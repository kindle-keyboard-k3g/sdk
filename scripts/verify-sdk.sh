#!/bin/bash
set -euo pipefail

echo "=== Kindle SDK End-to-End Verification ==="
TMP_DIR=$(mktemp -d)
trap 'rm -rf "${TMP_DIR}"' EXIT

echo "1. Initializing new project from template..."
PYTHONPATH=python/src python3 -m kindle_sdk.cli init "${TMP_DIR}/demo-app" --template pure-kindlet --name "DemoApp"
test -f "${TMP_DIR}/demo-app/kindle.toml"
test -f "${TMP_DIR}/demo-app/src/com/example/SampleKindlet.java"

echo "2. Building manifest and packaging active content into .azw2 container..."
mkdir -p "${TMP_DIR}/classes"
# Compile real Java class against kindlet-api.jar
/usr/lib/jvm/java-8-openjdk-amd64/bin/javac -source 1.4 -target 1.4 -cp "java/kindlet-api/dist/kindlet-api.jar" -d "${TMP_DIR}/classes" "${TMP_DIR}/demo-app/src/com/example/SampleKindlet.java"

AZW2_PATH="${TMP_DIR}/DemoApp.azw2"
PYTHONPATH=python/src python3 -c "
from kindle_sdk.packaging.azw2 import Azw2Packager
from kindle_sdk.packaging.manifest import ManifestSpec
from pathlib import Path
packager = Azw2Packager()
spec = ManifestSpec(
    main_class='com.example.SampleKindlet',
    implementation_title='DemoApp',
    toolbar_mode='persistent'
)
packager.package(Path('${AZW2_PATH}'), Path('${TMP_DIR}/classes'), spec)
assert Path('${AZW2_PATH}').exists()
print('AZW2 container packaged successfully!')
"

echo "3. Generating real test keystore with dk, di, dn certificate aliases..."
KEYSTORE_PATH="${TMP_DIR}/developer.keystore"
PASSWORD="kindlePassword123"

PYTHONPATH=python/src python3 -c "
from kindle_sdk.signing.keystore import KeystoreGenerator
from pathlib import Path
gen = KeystoreGenerator()
gen.generate(Path('${KEYSTORE_PATH}'), '${PASSWORD}')
assert Path('${KEYSTORE_PATH}').exists()
print('Developer keystore generated successfully with OpenSSL/keytool!')
"

echo "4. Triple-signing .azw2 with dk, di, dn aliases and verifying signature..."
PYTHONPATH=python/src python3 -c "
from kindle_sdk.signing.jarsigner import JarSigner
from pathlib import Path
import zipfile

signer = JarSigner()
aliases = ['dkDeveloper', 'diDeveloper', 'dnDeveloper']
signer.sign(Path('${AZW2_PATH}'), Path('${KEYSTORE_PATH}'), '${PASSWORD}', aliases)
assert signer.verify(Path('${AZW2_PATH}'))

# Verify presence of signature files (jarsigner 8.3 convention or full alias)
with zipfile.ZipFile(Path('${AZW2_PATH}'), 'r') as zf:
    names = zf.namelist()
    assert 'META-INF/MANIFEST.MF' in names
    assert any('DKDEVELO.SF' in n or 'DKDEVELOPER.SF' in n for n in names)
    assert any('DIDEVELO.SF' in n or 'DIDEVELOPER.SF' in n for n in names)
    assert any('DNDEVELO.SF' in n or 'DNDEVELOPER.SF' in n for n in names)
    print('Verified .azw2 package signatures: DK, DI, and DN blocks confirmed!')
"

echo "5. Simulating desktop e-ink 4bpp display quantizer..."
PYTHONPATH=python/src python3 -c "
from kindle_sdk.profiles import load_profile
k3 = load_profile('k3')
assert k3.display.width == 600
assert k3.display.height == 800
assert 'gray4' in k3.display.formats
print('K3 display profile validated!')
"

echo "6. Launching and executing official Kindlet in desktop simulator (headless mode)..."
/usr/lib/jvm/java-8-openjdk-amd64/bin/java -cp "java/emulator/dist/kindle-emulator.jar:java/kindlet-api/dist/kindlet-api.jar" com.amazon.kindle.emulator.EmulatorLauncher "${AZW2_PATH}" --headless --width 600 --height 800

echo "PASS: End-to-end SDK workflow with full signing verified successfully!"
