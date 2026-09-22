#!/bin/bash
set -euo pipefail

echo "=== Running Security Scanner: Checking for Committed Private Keys or Credentials ==="

LEAK_FOUND=0

# Scan for keystore or private key extensions
for ext in "*.keystore" "*.p12" "*.key" "*.pem"; do
    FOUND=$(git ls-files "${ext}")
    if [ -n "${FOUND}" ]; then
        echo "SECURITY ALERT: Found tracked key/keystore file: ${FOUND}"
        LEAK_FOUND=1
    fi
done

# Scan tracked files for private key headers
if git grep -I -i "BEGIN RSA PRIVATE KEY" || git grep -I -i "BEGIN PRIVATE KEY"; then
    echo "SECURITY ALERT: Found private key contents in tracked files!"
    LEAK_FOUND=1
fi

if [ "${LEAK_FOUND}" -eq 0 ]; then
    echo "PASS: Zero private keys or keystores committed. Security check passed!"
    exit 0
else
    echo "FAIL: Private keys detected in git index!"
    exit 1
fi
