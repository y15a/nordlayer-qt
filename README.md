# nordlayer-qt

A lightweight Qt6 system-tray GUI for the [NordLayer](https://nordlayer.com/) CLI on Linux/KDE Plasma.

> **Disclaimer:** This is an unofficial, community project. It is not affiliated with, endorsed by, or supported by Nord Security. "NordLayer" is a trademark of Nord Security.

## Features

- Connection status display with auto-refresh
- Gateway list (private + shared) with search/filter
- Connect/disconnect with progress indication
- System tray with colored status icon and quick actions
- Read-only settings view
- KDE Plasma native look via Qt6 Widgets

## Requirements

- Qt6 (Widgets) — `qt6-qtbase-devel` on Fedora
- NordLayer CLI — `/usr/bin/nordlayer`
- CMake 3.20+

## Build

```bash
# Install dependencies (Fedora)
sudo dnf install -y qt6-qtbase-devel qt6-qtsvg-devel cmake gcc-c++

# Build
./scripts/build.sh

# Run tests
cd build && ctest --output-on-failure

# Run
./build/nordlayer-qt
```

## Install

```bash
# Build + install to ~/.local (no sudo required)
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build
cmake --install build

# Enable autostart (current user)
cp ~/.local/share/applications/nordlayer-qt-autostart.desktop ~/.config/autostart/
```

## License

MIT — see [LICENSE](LICENSE).
