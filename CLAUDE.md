# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Qt6 Widgets system-tray GUI wrapping the NordLayer VPN CLI (`/usr/bin/nordlayer`) on Linux/KDE Plasma. C++17, MIT license.

## Build & Test

```bash
# Build (release)
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel $(nproc)

# Build with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON && cmake --build build --parallel $(nproc)

# Run tests (requires display or xvfb)
ctest --test-dir build --output-on-failure

# Run tests headless (CI-style)
xvfb-run ctest --test-dir build --output-on-failure

# Run app
./build/nordlayer-qt
```

**Dependencies (Fedora):** `qt6-qtbase-devel qt6-qtsvg-devel cmake gcc-c++`

Convenience scripts: `scripts/build.sh` (checks deps, builds), `scripts/run.sh` (builds if needed, runs).

## Architecture

Three-layer design with Qt signal/slot wiring:

```
UI Layer:  MainWindow (tabs: Gateways, Settings, About) + SystemTrayManager (tray icon/menu)
    ↕ signals/slots
Client:    NordLayerClient (QProcess wrapper, 30s timeout, one-command-at-a-time guard)
    ↓
Parser:    NordLayerParser (static functions: stripAnsi, parseStatus, parseGateways, parseSettings)
```

**Data flow:** 5-second QTimer → `NordLayerClient::refreshStatus()` → spawns `nordlayer status` via QProcess → raw output → `NordLayerParser::parseStatus()` strips ANSI/spinners and extracts fields → emits `statusUpdated(StatusInfo)` → MainWindow and SystemTrayManager update UI.

**Key types** (in `types.h`): `ConnectionState` enum, `StatusInfo`, `Gateway`, `SettingsInfo` structs.

**NordLayer CLI quirks** the parser handles:
- ANSI escape sequences and braille spinner characters in output
- Unicode box-drawing characters (│ U+2502, ─ U+2500) in gateway tables
- Gateway table has "Private gateways:" / "Shared gateways:" section headers

**Window behavior:** Close button minimizes to tray (doesn't exit). `--minimized` flag starts hidden.

**System tray icons:** Generated at runtime via QPainter (green #27ae60 = connected, gray #95a5a6 = disconnected, amber #f39c12 = connecting).

## Testing

Tests use **Qt Test** framework (`tests/test_parser.cpp`). Currently covers parser functions only (ANSI stripping, status/gateway/settings parsing). Qt Test requires a display server, hence xvfb in CI.

## CMake Notes

`CMAKE_AUTOMOC` and `CMAKE_AUTORCC` are enabled. Resources embedded via `resources/resources.qrc`. Adding new QObject subclasses or .qrc files is automatic.
