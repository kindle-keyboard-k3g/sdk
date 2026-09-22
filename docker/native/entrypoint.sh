#!/bin/bash
set -euo pipefail

if [ "$#" -eq 0 ]; then
    exec /verify-toolchain.sh
else
    exec "$@"
fi
