import unittest
from pathlib import Path

from PIL import Image, ImageChops

from draw_bikecomp import SCENARIOS, format_frame
from render_frames import render_raw

HERE = Path(__file__).resolve().parents[1]


class DisplaySimulatorTest(unittest.TestCase):
    def test_frame_format_matches_firmware_examples(self):
        self.assertEqual(("0.0", "IDLE  TRIP 0.00"), format_frame(SCENARIOS["idle"]))
        self.assertEqual(("24.8", "MOV  TRIP 18.42"), format_frame(SCENARIOS["moving"]))
        self.assertEqual(("0.0", "PAUSE  TRIP 18.42"), format_frame(SCENARIOS["paused"]))

    def test_all_scenarios_match_golden_pixels(self):
        for scenario in SCENARIOS:
            with self.subTest(scenario=scenario):
                actual = render_raw(scenario).convert("L")
                expected = Image.open(HERE / "golden" / f"{scenario}.png").convert("L")
                self.assertEqual((128, 32), actual.size)
                self.assertIsNone(ImageChops.difference(actual, expected).getbbox())

    def test_frames_are_visible_and_distinct(self):
        frames = [render_raw(name).convert("L") for name in SCENARIOS]
        for frame in frames:
            self.assertGreater(sum(1 for pixel in frame.getdata() if pixel), 50)
        self.assertEqual(len(frames), len({frame.tobytes() for frame in frames}))


if __name__ == "__main__":
    unittest.main()
