import unittest
import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from kindle_sdk.profiles import load_profile, validate_profile, ProfileValidationError

class TestProfiles(unittest.TestCase):
    def test_load_k3_profile(self):
        profile = load_profile("k3")
        self.assertEqual(profile.name, "k3")
        self.assertEqual(profile.soc, "imx353")
        self.assertEqual(profile.cpu, "armv6")
        self.assertEqual(profile.ram_bytes, 268435456)  # 256MB
        self.assertEqual(profile.display.width, 600)
        self.assertEqual(profile.display.height, 800)
        self.assertIn("gray4", profile.display.formats)
        self.assertIn("gray8", profile.display.formats)

    def test_load_dx_profile(self):
        profile = load_profile("dx")
        self.assertEqual(profile.name, "dx")
        self.assertEqual(profile.soc, "imx31")
        self.assertEqual(profile.cpu, "armv6")
        self.assertEqual(profile.ram_bytes, 134217728)  # 128MB
        self.assertEqual(profile.display.width, 824)
        self.assertEqual(profile.display.height, 1200)
        self.assertIn("gray4", profile.display.formats)

    def test_reject_invalid_profile_data(self):
        invalid_data = {
            "name": "invalid",
            "soc": "unknown_soc",
            "cpu": "x86",
            "ram_bytes": -1,
            "display": {
                "width": -100,
                "height": 0,
                "formats": ["unsupported_format"]
            }
        }
        with self.assertRaises(ProfileValidationError):
            validate_profile(invalid_data)

if __name__ == "__main__":
    unittest.main()
