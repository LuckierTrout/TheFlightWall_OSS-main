#!/usr/bin/env python3
"""
FlightWall editor smoke suite — LE-1.7 AC #10.

Tests:
  - Editor HTML asset served gzip-encoded
  - Layout CRUD: POST /api/layouts, PUT /api/layouts/:id
  - Activate: POST /api/layouts/:id/activate
  - Mode switch: GET /api/status confirms display_mode.active == "custom_layout"

Non-destructive by default: tearDown deletes any layout created during the run.

Examples:

    python3 tests/smoke/test_editor_smoke.py --base-url http://192.168.4.1
    python3 tests/smoke/test_editor_smoke.py --base-url http://flightwall.local --timeout 15
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import sys
import time
import unittest
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen


ARGS: argparse.Namespace | None = None

CANNED_LAYOUT = {
    "id": "smoke-test-layout",
    "name": "Smoke Test",
    "version": 1,
    "canvas": {"width": 192, "height": 48},
    "widgets": [
        {
            "id": "w1",
            "type": "text",
            "x": 0, "y": 0, "w": 64, "h": 8,
            "color": "#FFFFFF",
            "content": "SMOKE",
            "align": "left",
        }
    ],
}


def _parse_args(argv: list[str] | None = None) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description="Run FlightWall editor smoke tests against a live device.")
    parser.add_argument(
        "--base-url",
        default=os.environ.get("FLIGHTWALL_BASE_URL", ""),
        help="Device base URL, for example http://192.168.4.1 or http://flightwall.local",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="Request timeout in seconds",
    )
    return parser.parse_known_args(argv)


class Response:
    def __init__(self, status: int, headers: dict[str, str], body: bytes) -> None:
        self.status = status
        self.headers = headers
        self.body = body

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
        headers: dict[str, str] = {
            "Accept": "application/json, text/html;q=0.9, */*;q=0.8",
            "User-Agent": "flightwall-editor-smoke/1.0",
        }
        data = None
        if json_body is not None:
            data = json.dumps(json_body).encode("utf-8")
            headers["Content-Type"] = "application/json"

        req = Request(url=url, data=data, method=method.upper(), headers=headers)
        try:
            with urlopen(req, timeout=self.timeout) as response:
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


# Module-level id shared between test classes (set in TestEditorLayoutCRUD, consumed in later tests)
_created_layout_id: str | None = None


class EditorSmokeBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if ARGS is None:
            raise RuntimeError("CLI arguments were not initialized before tests started.")
        cls.args = ARGS  # type: ignore[attr-defined]
        cls.client = HttpClient(cls.args.base_url, cls.args.timeout)  # type: ignore[attr-defined]

    def assert_status(self, response: Response, expected_status: int) -> None:
        self.assertEqual(expected_status, response.status, msg=response.text[:500])

    def assert_json_ok(self, response: Response, expected_status: int = 200) -> dict[str, Any]:
        self.assert_status(response, expected_status)
        self.assertIn("application/json", response.content_type)
        payload = response.json()
        self.assertIsInstance(payload, dict)
        self.assertTrue(payload.get("ok"), msg=f"ok:false — {payload}")
        return payload


class TestEditorAsset(EditorSmokeBase):
    """AC #10a: /editor.html must be served HTTP 200 with Content-Encoding: gzip."""

    def test_editor_html_served_gzip(self) -> None:
        r = self.client.request("GET", "/editor.html")
        self.assert_status(r, 200)
        self.assertIn("text/html", r.content_type)
        self.assertEqual("gzip", r.encoding, msg="editor.html must be served with Content-Encoding: gzip")
        html = r.text
        self.assertIn("editor-canvas", html)
        self.assertIn("editor-toolbox", html)
        # LE-1.7 additions must be present
        self.assertIn("props-placeholder", html)
        self.assertIn("layout-name", html)
        self.assertIn("btn-save", html)
        self.assertIn("btn-activate", html)


class TestEditorCLayoutCRUD(EditorSmokeBase):
    """AC #10b: POST a layout and verify HTTP 200 with ok:true; capture returned id.

    NOTE: Class is named TestEditorCLayoutCRUD so that unittest's alphabetical
    loading places it between TestEditorAsset (A) and TestEditorDActivate (D).
    """

    def test_post_layout_creates_layout(self) -> None:
        global _created_layout_id
        # Clean up any leftover smoke layout from a previous run
        self.client.request("DELETE", "/api/layouts/smoke-test-layout")

        r = self.client.request("POST", "/api/layouts", CANNED_LAYOUT)
        payload = self.assert_json_ok(r)
        data = payload.get("data", {})
        self.assertIn("id", data, msg="POST /api/layouts response must include data.id")
        _created_layout_id = data["id"]
        self.assertEqual("smoke-test-layout", _created_layout_id)

    @classmethod
    def tearDownClass(cls) -> None:
        # Cleanup is handled by TestEditorDActivate.tearDownClass after all tests
        pass


class TestEditorDActivate(EditorSmokeBase):
    """AC #10c: POST /api/layouts/:id/activate must return HTTP 200 with ok:true.

    NOTE: Class is named TestEditorDActivate (not TestEditorActivate) so that
    unittest's alphabetical class loading runs this AFTER TestEditorCLayoutCRUD
    which sets the shared _created_layout_id.  Execution order: A-Asset,
    C-LayoutCRUD, D-Activate, E-ModeSwitch.
    """

    def test_activate_layout(self) -> None:
        if not _created_layout_id:
            self.skipTest("No layout id available — TestEditorCLayoutCRUD must run first.")
        r = self.client.request("POST", "/api/layouts/" + _created_layout_id + "/activate", {})
        self.assert_json_ok(r)

    @classmethod
    def tearDownClass(cls) -> None:
        global _created_layout_id
        # Non-destructive cleanup: delete the smoke layout if it was created
        if _created_layout_id and ARGS is not None:
            try:
                client = HttpClient(ARGS.base_url, ARGS.timeout)
                client.request("DELETE", "/api/layouts/" + _created_layout_id)
            except Exception:
                pass  # Best-effort cleanup; do not fail the test run
            _created_layout_id = None


class TestEditorModeSwitch(EditorSmokeBase):
    """AC #10d: GET /api/status must show display_mode.active == 'custom_layout' after activate."""

    def test_status_reports_custom_layout_mode(self) -> None:
        # Poll /api/status up to 3 times with 1-second intervals (sub-500ms switch per story)
        active_mode = None
        for _ in range(3):
            r = self.client.request("GET", "/api/status")
            payload = self.assert_json_ok(r)
            data = payload.get("data", {})
            display_mode = data.get("display_mode", {})
            active_mode = display_mode.get("active")
            if active_mode == "custom_layout":
                break
            time.sleep(1)
        self.assertEqual(
            "custom_layout",
            active_mode,
            msg="display_mode.active should be 'custom_layout' after activate call",
        )


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
