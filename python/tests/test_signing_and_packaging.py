import unittest
import sys
import tempfile
import zipfile
from pathlib import Path

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

    def test_real_keystore_generation_and_signing(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmppath = Path(tmpdir)
            keystore_path = tmppath / "test.keystore"
            jar_path = tmppath / "test.jar"

            # Create sample jar
            with zipfile.ZipFile(jar_path, "w") as zf:
                zf.writestr("META-INF/MANIFEST.MF", "Manifest-Version: 1.0\r\n\r\n")
                zf.writestr("com/example/Test.class", b"\xca\xfe\xba\xbe")

            # 1. Real keystore generation (via OpenSSL or keytool)
            gen = KeystoreGenerator()
            gen.generate(keystore_path, "password123")
            self.assertTrue(keystore_path.exists())
            self.assertGreater(keystore_path.stat().st_size, 0)

            # 2. Real triple signing with dk, di, dn aliases
            signer = JarSigner()
            signer.sign(jar_path, keystore_path, "password123", ["dkDeveloper", "diDeveloper", "dnDeveloper"])

            # 3. Verify signed jar
            self.assertTrue(signer.verify(jar_path))

            # Inspect zip entries to ensure .SF and .RSA/.DSA signatures are present
            with zipfile.ZipFile(jar_path, "r") as zf:
                names = zf.namelist()
                has_dk_sf = any("DKDEVELOPER.SF" in n for n in names)
                has_di_sf = any("DIDEVELOPER.SF" in n for n in names)
                has_dn_sf = any("DNDEVELOPER.SF" in n for n in names)
                self.assertTrue(has_dk_sf)
                self.assertTrue(has_di_sf)
                self.assertTrue(has_dn_sf)

if __name__ == "__main__":
    unittest.main()
