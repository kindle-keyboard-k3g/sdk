import unittest
import sys
import tempfile
import zipfile
from pathlib import Path
from unittest.mock import MagicMock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from kindle_sdk.packaging.manifest import ManifestBuilder, ManifestSpec, ManifestValidationError
from kindle_sdk.packaging.azw2 import Azw2Packager
from kindle_sdk.signing.keystore import KeystoreGenerator, KeystoreError
from kindle_sdk.signing.jarsigner import JarSigner, JarSignerError

class TestPackagingAndSigning(unittest.TestCase):

    def test_manifest_builder_success(self):
        builder = ManifestBuilder()
        spec = ManifestSpec(
            main_class="com.example.MyAppKindlet",
            implementation_title="My App",
            toolbar_mode="persistent"
        )
        data = builder.build(spec).decode("utf-8")
        self.assertIn("Main-Class: com.example.MyAppKindlet", data)
        self.assertIn("Implementation-Title: My App", data)
        self.assertIn("Extension-List: SDK", data)
        self.assertIn("SDK-Extension-Name: com.amazon.kindle.kindlet", data)
        self.assertIn("SDK-Specification-Version: 2.1", data)
        self.assertTrue(data.endswith("\r\n\r\n"))

    def test_manifest_builder_validation_failure(self):
        builder = ManifestBuilder()
        with self.assertRaises(ManifestValidationError):
            builder.build(ManifestSpec(main_class="", implementation_title="Title"))

        with self.assertRaises(ManifestValidationError):
            builder.build(ManifestSpec(main_class="com.example.App", implementation_title="", toolbar_mode="invalid"))

    def test_azw2_packaging(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmppath = Path(tmpdir)
            classes_dir = tmppath / "classes"
            classes_dir.mkdir()
            (classes_dir / "App.class").write_bytes(b"\xca\xfe\xba\xbe")

            out_jar = tmppath / "app.azw2"
            packager = Azw2Packager()
            spec = ManifestSpec(main_class="App", implementation_title="App")
            packager.package(out_jar, classes_dir, spec)

            self.assertTrue(out_jar.exists())
            with zipfile.ZipFile(out_jar, "r") as zf:
                names = zf.namelist()
                self.assertIn("META-INF/MANIFEST.MF", names)
                self.assertIn("App.class", names)

    @patch("subprocess.run")
    def test_keystore_generator_invokes_keytool(self, mock_run):
        mock_run.return_value = MagicMock(returncode=0)
        gen = KeystoreGenerator(keytool_bin="keytool")
        with tempfile.TemporaryDirectory() as tmpdir:
            out_ks = Path(tmpdir) / "test.keystore"
            gen.generate(out_ks, "password123")
            # Should have invoked keytool 3 times for dk, di, dn
            self.assertEqual(mock_run.call_count, 3)

    @patch("subprocess.run")
    def test_jarsigner_invokes_sign_and_verify(self, mock_run):
        mock_run.return_value = MagicMock(returncode=0, stdout="jar verified.")
        signer = JarSigner(jarsigner_bin="jarsigner")
        with tempfile.TemporaryDirectory() as tmpdir:
            jar = Path(tmpdir) / "test.jar"
            jar.write_bytes(b"dummy")
            ks = Path(tmpdir) / "test.keystore"
            ks.write_bytes(b"dummy")

            signer.sign(jar, ks, "password123", ["dkDeveloper", "diDeveloper", "dnDeveloper"])
            self.assertEqual(mock_run.call_count, 3)

            verified = signer.verify(jar)
            self.assertTrue(verified)

if __name__ == "__main__":
    unittest.main()
