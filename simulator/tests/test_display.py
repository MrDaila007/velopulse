import unittest
from pathlib import Path

from PIL import Image, ImageChops

from draw_bikecomp import GOLDEN_SCENARIOS, firmware_commands
from render_frames import render_raw

HERE = Path(__file__).resolve().parents[1]
DISPLAY_HEIGHTS = (32, 64)


class DisplaySimulatorTest(unittest.TestCase):
    def test_128x32_commands_remain_pixel_compatible(self):
        commands = firmware_commands("average", 32)
        texts = [command[3] for command in commands if command[0] == "TEXT"]
        self.assertEqual(["24.8", "82%", "3.9V", "km/h", "MOV AVG 19.7 km/h"], texts)
        self.assertIn(["FRAME", "107", "0", "12", "8"], commands)

        unknown_texts = [
            command[3]
            for command in firmware_commands("battery_unknown", 32)
            if command[0] == "TEXT"
        ]
        self.assertIn("--V", unknown_texts)
        self.assertIn("--%", unknown_texts)

        low_commands = firmware_commands("low_battery", 32)
        self.assertIn(["TEXT", "2", "30", "LOW BATT"], low_commands)
        self.assertIn(["COLOR", "0"], low_commands)
        self.assertIn(["COLOR", "1"], low_commands)

    def test_128x64_uses_large_speed_and_metric_zones(self):
        commands = firmware_commands("average", 64)
        self.assertIn(["TEXT", "0", "7", "km/h"], commands)
        self.assertIn(["FONT", "SPEED_LARGE"], commands)
        self.assertIn(["TEXT", "0", "47", "24.8"], commands)
        self.assertIn(["FONT", "METRIC_LARGE"], commands)
        self.assertIn(["TEXT", "0", "63", "MOV AVG 19.7 km/h"], commands)

        low_commands = firmware_commands("low_battery", 64)
        self.assertIn(["BOX", "0", "50", "128", "14"], low_commands)
        self.assertIn(["TEXT", "2", "63", "LOW BATT"], low_commands)
        self.assertIn(["COLOR", "0"], low_commands)
        self.assertIn(["COLOR", "1"], low_commands)

    def test_128x64_boundary_values_and_font_fallback(self):
        expected_speeds = {
            "speed_0": "0.0",
            "speed_99_9": "99.9",
            "speed_100": "100.0",
            "speed_200": "200.0",
        }
        for scenario, speed in expected_speeds.items():
            with self.subTest(scenario=scenario):
                commands = firmware_commands(scenario, 64)
                self.assertIn(["TEXT", "0", "47", speed], commands)
                self.assertIn(["FONT", "SPEED_LARGE"], commands)

        empty_commands = firmware_commands("battery_empty", 64)
        self.assertIn(["TEXT", "91", "7", "0%"], empty_commands)
        self.assertIn(["TEXT", "91", "17", "3.0V"], empty_commands)
        self.assertNotIn(["BOX", "109", "2", "1", "4"], empty_commands)

        full_commands = firmware_commands("battery_full", 64)
        self.assertIn(["TEXT", "91", "7", "100%"], full_commands)
        self.assertIn(["TEXT", "91", "17", "4.2V"], full_commands)
        self.assertIn(["BOX", "109", "2", "8", "4"], full_commands)

        long_commands = firmware_commands("long_metric", 64)
        font_commands = [command for command in long_commands if command[0] == "FONT"]
        self.assertEqual(["FONT", "SMALL"], font_commands[-1])

    def test_burn_in_offset_moves_shared_layout_for_both_profiles(self):
        for display_height, speed_y in ((32, 21), (64, 47)):
            with self.subTest(display_height=display_height):
                commands = firmware_commands(
                    "average", display_height, x_offset=1, y_offset=1
                )
                self.assertIn(["TEXT", "1", str(speed_y + 1), "24.8"], commands)
                self.assertIn(["FRAME", "108", "1", "12", "8"], commands)

    def test_all_scenarios_match_golden_pixels_for_both_profiles(self):
        for display_height in DISPLAY_HEIGHTS:
            for scenario in GOLDEN_SCENARIOS:
                with self.subTest(display_height=display_height, scenario=scenario):
                    actual = render_raw(scenario, display_height).convert("L")
                    expected = Image.open(
                        HERE
                        / "golden"
                        / f"128x{display_height}"
                        / f"{scenario}.png"
                    ).convert("L")
                    self.assertEqual((128, display_height), actual.size)
                    self.assertIsNone(
                        ImageChops.difference(actual, expected).getbbox()
                    )

    def test_frames_are_visible_and_distinct_for_both_profiles(self):
        for display_height in DISPLAY_HEIGHTS:
            frames = [
                render_raw(name, display_height).convert("L")
                for name in GOLDEN_SCENARIOS
            ]
            for frame in frames:
                self.assertGreater(sum(1 for pixel in frame.getdata() if pixel), 50)
            self.assertEqual(len(frames), len({frame.tobytes() for frame in frames}))


if __name__ == "__main__":
    unittest.main()
