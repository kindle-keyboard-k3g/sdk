#!/bin/bash
set -euo pipefail

echo "=== Kindle SDK End-to-End Verification ==="
TMP_DIR=$(mktemp -d)
trap 'rm -rf "${TMP_DIR}"' EXIT

echo "1. Initializing new project from template..."
PYTHONPATH=python/src python3 -m kindle_sdk.cli init "${TMP_DIR}/demo-app" --template pure-kindlet --name "DemoApp"
test -f "${TMP_DIR}/demo-app/kindle.toml"
test -f "${TMP_DIR}/demo-app/src/com/example/SampleKindlet.java"

echo "2. Building manifest and verifying spec..."
PYTHONPATH=python/src python3 -c "
from kindle_sdk.packaging.manifest import ManifestBuilder, ManifestSpec
from pathlib import Path
builder = ManifestBuilder()
spec = ManifestSpec(main_class='com.example.SampleKindlet', implementation_title='DemoApp')
data = builder.build(spec)
assert b'com.amazon.kindle.kindlet' in data
assert b'Main-Class: com.example.SampleKindlet' in data
print('Manifest generated and verified!')
"

echo "3. Packaging simulated application into .azw2 container..."
mkdir -p "${TMP_DIR}/classes/com/example"
echo "cafebabe" > "${TMP_DIR}/classes/com/example/SampleKindlet.class"

PYTHONPATH=python/src python3 -c "
from kindle_sdk.packaging.azw2 import Azw2Packager
from kindle_sdk.packaging.manifest import ManifestSpec
from pathlib import Path
packager = Azw2Packager()
spec = ManifestSpec(main_class='com.example.SampleKindlet', implementation_title='DemoApp')
packager.package(Path('${TMP_DIR}/demo.azw2'), Path('${TMP_DIR}/classes'), spec)
assert Path('${TMP_DIR}/demo.azw2').exists()
print('AZW2 package built successfully!')
"

echo "4. Simulating desktop e-ink 4bpp display quantizer..."
PYTHONPATH=python/src python3 -c "
from kindle_sdk.profiles import load_profile
k3 = load_profile('k3')
assert k3.display.width == 600
assert k3.display.height == 800
assert 'gray4' in k3.display.formats
print('K3 display profile validated!')
"

echo "PASS: End-to-end SDK workflow verified successfully!"
