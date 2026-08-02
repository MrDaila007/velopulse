#!/usr/bin/env python3
"""Offline checks for USB regression fixture parsing."""

from __future__ import annotations

import tempfile
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from usb_regression import DEFAULT_FIXTURE_DIR, extract_results, load_fixture


class UsbRegressionOfflineTest(unittest.TestCase):
    def test_load_fixture_skips_comments_and_blanks(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.txt"
            path.write_text(
                "# comment\n\nreset\n\npulse 0\n# tail\n",
                encoding="utf-8",
            )
            self.assertEqual(load_fixture(path), ["reset", "pulse 0"])

    def test_extract_results_parses_status_lines(self) -> None:
        output = "noise\nOK reset\nFAIL expect speed_x100 got=0 want=3600\n"
        self.assertEqual(
            extract_results(output),
            [
                ("OK", "reset"),
                ("FAIL", "expect speed_x100 got=0 want=3600"),
            ],
        )

    def test_default_fixtures_exist(self) -> None:
        fixtures = sorted(DEFAULT_FIXTURE_DIR.glob("*.txt"))
        self.assertGreaterEqual(len(fixtures), 3)


if __name__ == "__main__":
    unittest.main()
