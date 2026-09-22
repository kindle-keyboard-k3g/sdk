import unittest
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from kindle_sdk.project.init import init_project, TEMPLATES, ProjectInitError
from kindle_sdk.toolchains import check_toolchains

class TestCliAndTemplates(unittest.TestCase):

    def test_toolchain_check(self):
        status = check_toolchains()
        self.assertIn("docker", status)
        self.assertIn("python3", status)
        self.assertTrue(status["docker"])
        self.assertTrue(status["python3"])

    def test_init_pure_kindlet_template(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            dest = Path(tmpdir) / "test-app"
            init_project(dest, "pure-kindlet", "TestApp")
            self.assertTrue(dest.exists())
            self.assertTrue((dest / "kindle.toml").exists())
            self.assertTrue((dest / "src/com/example/SampleKindlet.java").exists())

    def test_init_native_process_template(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            dest = Path(tmpdir) / "test-native"
            init_project(dest, "native-process-kindlet", "TestNative")
            self.assertTrue(dest.exists())
            self.assertTrue((dest / "native/main.cpp").exists())

    def test_init_refuses_non_empty_dir(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            dest = Path(tmpdir)
            (dest / "existing.txt").write_text("exists")
            with self.assertRaises(ProjectInitError):
                init_project(dest, "pure-kindlet", "App")

if __name__ == "__main__":
    unittest.main()
