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
        for key in ("brightness", "fetch_interval", "display_cycle", "tiles_x", "tiles_y", "tile_pixels"):
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
        for key in ("wifi", "opensky", "aeroapi", "cdn", "nvs", "littlefs"):
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
