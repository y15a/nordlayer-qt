#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Check dependencies
check_deps() {
    local missing=()
    command -v cmake >/dev/null || missing+=(cmake)
    if ! pkg-config --exists Qt6Widgets 2>/dev/null && \
       ! [ -f /usr/lib64/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake ]; then
        missing+=(qt6-qtbase-devel)
    fi
    if (( ${#missing[@]} > 0 )); then
        echo "Missing dependencies: ${missing[*]}"
        echo "Install with: sudo dnf install -y ${missing[*]}"
        exit 1
    fi
}

check_deps

cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE="${1:-Release}" \
    -DBUILD_TESTS=ON

cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo "Build complete: ${BUILD_DIR}/nordlayer-qt"
echo "Run tests:      cd ${BUILD_DIR} && ctest --output-on-failure"
echo "Run app:        ${BUILD_DIR}/nordlayer-qt"
echo ""
echo "Install (user-local):"
echo "  cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local -DAUTOSTART_INSTALL_DIR=~/.config/autostart"
echo "  cmake --install build"
echo ""
echo "Install (system-wide):"
echo "  cmake -B build -DCMAKE_INSTALL_PREFIX=/usr"
echo "  sudo cmake --install build"
