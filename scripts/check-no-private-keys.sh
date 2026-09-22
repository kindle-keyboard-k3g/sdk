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

# Scan tracked files for private key headers, excluding this scanner script
HEADER_MATCHES=$(git grep -I -i "BEGIN RSA PRIVATE KEY" -- ':!scripts/check-no-private-keys.sh' || true)
if [ -n "${HEADER_MATCHES}" ]; then
    echo "SECURITY ALERT: Found private key contents in tracked files: ${HEADER_MATCHES}"
    LEAK_FOUND=1
fi

HEADER_MATCHES2=$(git grep -I -i "BEGIN PRIVATE KEY" -- ':!scripts/check-no-private-keys.sh' || true)
if [ -n "${HEADER_MATCHES2}" ]; then
    echo "SECURITY ALERT: Found private key contents in tracked files: ${HEADER_MATCHES2}"
    LEAK_FOUND=1
fi

if [ "${LEAK_FOUND}" -eq 0 ]; then
    echo "PASS: Zero private keys or keystores committed. Security check passed!"
    exit 0
else
    echo "FAIL: Private keys detected in git index!"
    exit 1
fi
