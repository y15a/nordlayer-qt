#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
BINARY="${BUILD_DIR}/nordlayer-qt"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Building first..."
    "$SCRIPT_DIR/build.sh"
fi

exec "$BINARY" "$@"
