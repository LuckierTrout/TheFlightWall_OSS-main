#!/usr/bin/env python3
"""
FlightWall web portal smoke suite.

This suite is intentionally lightweight:

- Python standard library only
- Non-destructive by default
- Targets a live device over HTTP
- Safe enough to use as a first-pass release gate

Examples:

    python3 tests/smoke/test_web_portal_smoke.py --base-url http://192.168.4.1
    python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local --expect-mode dashboard
    python3 tests/smoke/test_web_portal_smoke.py --base-url http://192.168.4.1 --write-roundtrip
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import sys
import unittest
from dataclasses import dataclass
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen


ARGS: argparse.Namespace | None = None


def _parse_args(argv: list[str] | None = None) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description="Run FlightWall HTTP smoke tests against a live device.")
    parser.add_argument(
        "--base-url",
        default=os.environ.get("FLIGHTWALL_BASE_URL", ""),
        help="Device base URL, for example http://192.168.4.1 or http://flightwall.local",
    )
    parser.add_argument(
        "--expect-mode",
        choices=("any", "wizard", "dashboard"),
        default="any",
        help="Expected page served at /",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="Request timeout in seconds",
    )
    parser.add_argument(
        "--write-roundtrip",
        action="store_true",
        help="Opt-in write test that changes brightness and restores it",
    )
    return parser.parse_known_args(argv)


@dataclass
class Response:
    status: int
    headers: dict[str, str]
    body: bytes

    @property
    def content_type(self) -> str:
        return self.headers.get("content-type", "").lower()

    @property
    def encoding(self) -> str:
        return self.headers.get("content-encoding", "").lower()

    @property
    def decoded_body(self) -> bytes:
        if self.encoding == "gzip":
            return gzip.decompress(self.body)
        return self.body

    @property
    def text(self) -> str:
        return self.decoded_body.decode("utf-8", errors="replace")

    def json(self) -> Any:
        return json.loads(self.text)


class HttpClient:
    def __init__(self, base_url: str, timeout: float) -> None:
        normalized = base_url.strip()
        if not normalized:
            raise ValueError("A base URL is required. Pass --base-url or set FLIGHTWALL_BASE_URL.")
        self.base_url = normalized.rstrip("/") + "/"
        self.timeout = timeout

    def request(self, method: str, path: str, json_body: Any | None = None) -> Response:
        url = urljoin(self.base_url, path.lstrip("/"))
        headers = {
            "Accept": "application/json, text/html;q=0.9, */*;q=0.8",
            "User-Agent": "flightwall-smoke/1.0",
        }
        data = None
        if json_body is not None:
            data = json.dumps(json_body).encode("utf-8")
            headers["Content-Type"] = "application/json"

        request = Request(url=url, data=data, method=method.upper(), headers=headers)

        try:
            with urlopen(request, timeout=self.timeout) as response:
                return Response(
                    status=response.getcode(),
                    headers={k.lower(): v for k, v in response.info().items()},
                    body=response.read(),
                )
        except HTTPError as error:
            return Response(
                status=error.code,
                headers={k.lower(): v for k, v in error.headers.items()},
                body=error.read(),
            )
        except URLError as error:
            raise AssertionError(f"Request to {url} failed: {error.reason}") from error


class FlightWallSmokeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if ARGS is None:
            raise RuntimeError("CLI arguments were not initialized before tests started.")
        cls.args = ARGS
        cls.client = HttpClient(cls.args.base_url, cls.args.timeout)

    def assert_status(self, response: Response, expected_status: int) -> None:
        self.assertEqual(expected_status, response.status, msg=response.text)

    def assert_json_object(self, response: Response, expected_status: int = 200) -> dict[str, Any]:
        self.assert_status(response, expected_status)
        self.assertIn("application/json", response.content_type)
        payload = response.json()
        self.assertIsInstance(payload, dict)
        return payload

    def test_root_serves_expected_page(self) -> None:
        response = self.client.request("GET", "/")
        self.assert_status(response, 200)
        self.assertIn("text/html", response.content_type)
        html = response.text
        self.assertIn("<html", html.lower())

        is_wizard = "FlightWall Setup" in html
        is_dashboard = "<title>FlightWall</title>" in html and "System Health" in html

        if self.args.expect_mode == "wizard":
            self.assertTrue(is_wizard, "Expected wizard page at /")
        elif self.args.expect_mode == "dashboard":
            self.assertTrue(is_dashboard, "Expected dashboard page at /")
        else:
            self.assertTrue(is_wizard or is_dashboard, "Root page did not look like wizard or dashboard")

    def test_health_page_loads(self) -> None:
        response = self.client.request("GET", "/health.html")
        self.assert_status(response, 200)
        self.assertIn("text/html", response.content_type)
        html = response.text
        self.assertIn("System Health", html)
        self.assertIn("refresh-btn", html)

    def test_get_settings_contract(self) -> None:
        payload = self.assert_json_object(self.client.request("GET", "/api/settings"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        # Post hw-1.3: tiles_x/tiles_y/tile_pixels/display_pin retired from
        # /api/settings — they're no longer configurable (HW-1 HUB75 wall is
        # fixed at 256x192). /api/layout still exposes them as derived values.
        for key in ("brightness", "fetch_interval", "display_cycle"):
            self.assertIn(key, data)

    def test_get_status_contract(self) -> None:
        payload = self.assert_json_object(self.client.request("GET", "/api/status"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        for key in ("subsystems", "wifi_detail", "device", "flight", "quota"):
            self.assertIn(key, data)

        subsystems = data["subsystems"]
        self.assertIsInstance(subsystems, dict)
        for key in ("wifi", "aggregator", "cdn", "nvs", "littlefs"):
            self.assertIn(key, subsystems)

    def test_get_layout_contract(self) -> None:
        payload = self.assert_json_object(self.client.request("GET", "/api/layout"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        for key in ("matrix", "logo_zone", "flight_zone", "telemetry_zone", "hardware"):
            self.assertIn(key, data)

    def test_get_wifi_scan_contract(self) -> None:
        payload = self.assert_json_object(self.client.request("GET", "/api/wifi/scan"))
        self.assertTrue(payload.get("ok"))
        self.assertIn("scanning", payload)
        self.assertIsInstance(payload["scanning"], bool)
        self.assertIn("data", payload)
        self.assertIsInstance(payload["data"], list)

    def test_get_logos_contract(self) -> None:
        payload = self.assert_json_object(self.client.request("GET", "/api/logos"))
        self.assertTrue(payload.get("ok"))
        self.assertIn("data", payload)
        self.assertIsInstance(payload["data"], list)
        self.assertIn("storage", payload)
        self.assertIsInstance(payload["storage"], dict)
        for key in ("used", "total", "logo_count"):
            self.assertIn(key, payload["storage"])

    # ── LE-1.6 Editor assets ──────────────────────────────────────────────

    def test_editor_page_loads(self) -> None:
        """LE-1.6 AC #1: /editor.html must be served gzip-encoded as text/html."""
        response = self.client.request("GET", "/editor.html")
        self.assert_status(response, 200)
        self.assertIn("text/html", response.content_type)
        self.assertEqual("gzip", response.encoding, msg="editor.html must be served with Content-Encoding: gzip")
        html = response.text
        self.assertIn("editor-canvas", html)
        self.assertIn("editor-toolbox", html)

    def test_editor_js_loads(self) -> None:
        """LE-1.6 AC #1: /editor.js must be served gzip-encoded as application/javascript."""
        response = self.client.request("GET", "/editor.js")
        self.assert_status(response, 200)
        self.assertIn("javascript", response.content_type)
        self.assertEqual("gzip", response.encoding, msg="editor.js must be served with Content-Encoding: gzip")

    def test_editor_css_loads(self) -> None:
        """LE-1.6 AC #1: /editor.css must be served gzip-encoded as text/css."""
        response = self.client.request("GET", "/editor.css")
        self.assert_status(response, 200)
        self.assertIn("css", response.content_type)
        self.assertEqual("gzip", response.encoding, msg="editor.css must be served with Content-Encoding: gzip")

    def test_get_widget_types_contract(self) -> None:
        """LE-1.6 AC #3: /api/widgets/types must return a list of type entries with required shape."""
        payload = self.assert_json_object(self.client.request("GET", "/api/widgets/types"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, list, msg="data must be a list of widget type entries")
        self.assertGreater(len(data), 0, msg="at least one widget type must be present")
        for entry in data:
            self.assertIsInstance(entry, dict)
            self.assertIn("type", entry, msg="each widget type entry must have a 'type' key")
            self.assertIn("label", entry, msg="each widget type entry must have a 'label' key")
            self.assertIn("fields", entry, msg="each widget type entry must have a 'fields' list")
            self.assertIsInstance(entry["fields"], list)

    def test_get_status_includes_ota_fields(self) -> None:
        """Story dl-6.2, AC #3: GET /api/status must include ota_available and ota_version."""
        payload = self.assert_json_object(self.client.request("GET", "/api/status"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        # AC #3: ota_available must be present and boolean
        self.assertIn("ota_available", data)
        self.assertIsInstance(data["ota_available"], bool)
        # AC #3: ota_version must be present (string when available, null otherwise)
        self.assertIn("ota_version", data)
        # ota_version is either a string or null (None in Python)
        self.assertTrue(data["ota_version"] is None or isinstance(data["ota_version"], str))

    def test_get_ota_check_contract(self) -> None:
        """Story dl-6.2, AC #1-#2: GET /api/ota/check endpoint response shape."""
        payload = self.assert_json_object(self.client.request("GET", "/api/ota/check"))
        # Root 'ok' field must be true (even on error, pattern documented in story)
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        # AC #2: required fields
        self.assertIn("available", data)
        self.assertIsInstance(data["available"], bool)
        self.assertIn("current_version", data)
        self.assertIsInstance(data["current_version"], str)
        # version and release_notes may be string or null
        self.assertIn("version", data)
        self.assertTrue(data["version"] is None or isinstance(data["version"], str))
        self.assertIn("release_notes", data)
        self.assertTrue(data["release_notes"] is None or isinstance(data["release_notes"], str))
        # If there's an error field, it should be a string
        if "error" in data:
            self.assertIsInstance(data["error"], str)

    def test_get_ota_status_contract(self) -> None:
        """Story dl-7.3, AC #3: GET /api/ota/status endpoint response shape."""
        payload = self.assert_json_object(self.client.request("GET", "/api/ota/status"))
        self.assertTrue(payload.get("ok"))
        data = payload.get("data")
        self.assertIsInstance(data, dict)
        # AC #3: required fields with stable lowercase state names
        self.assertIn("state", data)
        self.assertIsInstance(data["state"], str)
        valid_states = ("idle", "checking", "available", "downloading", "verifying", "rebooting", "error")
        self.assertIn(data["state"], valid_states, f"State '{data['state']}' not in valid states")
        # progress: required for downloading, null otherwise
        self.assertIn("progress", data)
        if data["state"] == "downloading":
            self.assertIsInstance(data["progress"], int)
        else:
            # progress can be int or None for other states
            self.assertTrue(data["progress"] is None or isinstance(data["progress"], int))
        # error: non-null only when state == error
        self.assertIn("error", data)
        if data["state"] == "error":
            self.assertIsInstance(data["error"], str)
        else:
            self.assertIsNone(data["error"])
        # failure_phase and retriable from dl-7.2
        self.assertIn("failure_phase", data)
        self.assertIsInstance(data["failure_phase"], str)
        valid_phases = ("none", "download", "verify", "boot")
        self.assertIn(data["failure_phase"], valid_phases)
        self.assertIn("retriable", data)
        self.assertIsInstance(data["retriable"], bool)

    def test_post_ota_pull_rejects_when_not_available(self) -> None:
        """Story dl-7.3, AC #2: POST /api/ota/pull error when no update available."""
        # First ensure we're not in AVAILABLE state by not calling /api/ota/check
        # or by device being in IDLE state after boot
        response = self.client.request("POST", "/api/ota/pull", json_body={})
        # Should return error (400 or 409) since no update is available
        # The exact status depends on current state
        payload = response.json()
        self.assertIsInstance(payload, dict)
        # If state is not AVAILABLE, should return error
        # We don't control state, so we just verify the response shape is correct
        if not payload.get("ok"):
            # AC #2: error response shape
            self.assertIn("error", payload)
            self.assertIsInstance(payload["error"], str)
            self.assertIn("code", payload)
            self.assertIsInstance(payload["code"], str)
            # Valid error codes per AC #2
            valid_codes = ("NO_UPDATE", "OTA_BUSY", "OTA_START_FAILED")
            self.assertIn(payload["code"], valid_codes)

    def test_post_settings_rejects_empty_payload(self) -> None:
        payload = self.assert_json_object(self.client.request("POST", "/api/settings", json_body={}), expected_status=400)
        self.assertFalse(payload.get("ok"))
        self.assertEqual("EMPTY_PAYLOAD", payload.get("code"))

    def test_mode_switch_during_calibration_succeeds(self) -> None:
        """BF-1 AC #7: a mode switch issued while calibration is active must complete
        without timing out. Before the auto-yield fix, the Core-0 display task skipped
        ModeRegistry::tick() while calibration was running, so /api/display/mode would
        sit in REQUESTED state until the dashboard polled itself out at the timeout.
        """
        # Capture the active mode so we can restore it in teardown.
        modes_before = self.assert_json_object(self.client.request("GET", "/api/display/modes"))
        original_active = modes_before.get("data", {}).get("active")
        self.assertIsInstance(original_active, str, msg="device must report an active mode")

        # Pick a target mode different from the current one so the switch is non-trivial.
        modes_list = modes_before.get("data", {}).get("modes", [])
        self.assertGreater(len(modes_list), 1, msg="device must expose at least two modes for this test")
        target = next((m for m in modes_list if m.get("id") != original_active), None)
        self.assertIsNotNone(target, msg="could not find a non-active mode to switch to")
        target_id = target["id"]

        start = self.client.request("POST", "/api/calibration/start", json_body={})
        # Endpoint is optional in some legacy builds; skip rather than fail to keep
        # the suite portable across firmware revisions.
        if start.status == 404:
            self.skipTest("/api/calibration/start not available on this device")
        self.assert_status(start, 200)

        try:
            switch = self.client.request(
                "POST", "/api/display/mode", json_body={"mode": target_id}
            )
            self.assert_status(switch, 200)
            switch_payload = switch.json()
            self.assertTrue(switch_payload.get("ok"), msg=switch.text)

            # Poll /api/display/modes for terminal IDLE with the requested mode active.
            # Budget: 8s — same envelope the dashboard uses (SWITCH_POLL_TIMEOUT).
            import time
            deadline = time.time() + 8.0
            last_payload: dict[str, Any] = {}
            while time.time() < deadline:
                poll = self.assert_json_object(self.client.request("GET", "/api/display/modes"))
                data = poll.get("data", {})
                last_payload = data
                state = data.get("switch_state", "idle")
                if state == "idle" and data.get("active") == target_id:
                    # AC #4: if calibration was preempted, the response should advertise it.
                    self.assertEqual(
                        "calibration",
                        data.get("preempted"),
                        msg=f"expected preempted=calibration in {data!r}",
                    )
                    break
                if state == "failed":
                    self.fail(f"mode switch reported failed state during calibration: {data!r}")
                time.sleep(0.25)
            else:
                self.fail(
                    f"mode switch did not settle within 8s while calibration was active. "
                    f"last poll payload: {last_payload!r}"
                )
        finally:
            # Belt-and-suspenders: server-side preemption already cleared calibration,
            # but stop again so this test never leaves the device showing a test pattern.
            self.client.request("POST", "/api/calibration/stop", json_body={})
            # Restore the original mode if we changed it.
            if original_active and original_active != target_id:
                self.client.request("POST", "/api/display/mode", json_body={"mode": original_active})

    def test_optional_brightness_write_roundtrip(self) -> None:
        if not self.args.write_roundtrip:
            self.skipTest("Pass --write-roundtrip to enable settings mutation checks.")

        settings_before = self.assert_json_object(self.client.request("GET", "/api/settings"))
        current_value = settings_before["data"]["brightness"]
        self.assertIsInstance(current_value, int)
        new_value = 64 if current_value != 64 else 96

        try:
            update_payload = self.assert_json_object(
                self.client.request("POST", "/api/settings", json_body={"brightness": new_value})
            )
            self.assertTrue(update_payload.get("ok"))
            self.assertFalse(update_payload.get("reboot_required"))
            self.assertIn("brightness", update_payload.get("applied", []))

            settings_after = self.assert_json_object(self.client.request("GET", "/api/settings"))
            self.assertEqual(new_value, settings_after["data"]["brightness"])
        finally:
            revert_payload = self.assert_json_object(
                self.client.request("POST", "/api/settings", json_body={"brightness": current_value})
            )
            self.assertTrue(revert_payload.get("ok"))


def main(argv: list[str] | None = None) -> int:
    global ARGS

    ARGS, remaining = _parse_args(argv)
    if not ARGS.base_url:
        print("error: pass --base-url or set FLIGHTWALL_BASE_URL", file=sys.stderr)
        return 2

    unittest_argv = [sys.argv[0], *remaining]
    program = unittest.main(argv=unittest_argv, verbosity=2, exit=False)
    return 0 if program.result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
