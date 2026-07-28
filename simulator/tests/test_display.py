import unittest
from pathlib import Path

from PIL import Image, ImageChops

from draw_bikecomp import GOLDEN_SCENARIOS, firmware_commands
from render_frames import render_raw

HERE = Path(__file__).resolve().parents[1]


class DisplaySimulatorTest(unittest.TestCase):
    def test_commands_come_from_firmware_renderer(self):
        texts = [command[3] for command in firmware_commands("average") if command[0] == "TEXT"]
        self.assertEqual(["24.8", "82%", "km/h", "MOV AVG 19.7 km/h"], texts)
        self.assertIn(["FRAME", "91", "0", "12", "8"], firmware_commands("trip"))
        unknown_texts = [command[3] for command in firmware_commands("battery_unknown") if command[0] == "TEXT"]
        self.assertIn("--%", unknown_texts)
        low_commands = firmware_commands("low_battery")
        self.assertIn(["TEXT", "2", "30", "LOW BATT"], low_commands)
        self.assertIn(["COLOR", "0"], low_commands)
        self.assertIn(["COLOR", "1"], low_commands)

    def test_all_scenarios_match_golden_pixels(self):
        for scenario in GOLDEN_SCENARIOS:
            with self.subTest(scenario=scenario):
                actual = render_raw(scenario).convert("L")
                expected = Image.open(HERE / "golden" / f"{scenario}.png").convert("L")
                self.assertEqual((128, 32), actual.size)
                self.assertIsNone(ImageChops.difference(actual, expected).getbbox())

    def test_frames_are_visible_and_distinct(self):
        frames = [render_raw(name).convert("L") for name in GOLDEN_SCENARIOS]
        for frame in frames:
            self.assertGreater(sum(1 for pixel in frame.getdata() if pixel), 50)
        self.assertEqual(len(frames), len({frame.tobytes() for frame in frames}))


if __name__ == "__main__":
    unittest.main()
