# Nounours Player

A free and open-source, cross-platform media player built on [libmpv](https://mpv.io/) and Qt6.

[![Build](https://github.com/Antidote1911/Nounours-Player/actions/workflows/build.yml/badge.svg)](https://github.com/Antidote1911/Nounours-Player/actions/workflows/build.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE)

## Features

- Plays virtually any audio/video format via libmpv/FFmpeg
- Playlist management with shuffle and repeat modes
- Gesture support (drag to seek, drag to adjust volume)
- Dim Lights mode (X11)
- Always-on-top window
- Screenshot capture
- Subtitle and audio track selection
- Configurable keyboard shortcuts
- Update checker
- 13 languages: German, Spanish, French, Croatian, Italian, Georgian, Korean, Dutch, Brazilian Portuguese, Russian, Turkish, Vietnamese, Chinese (Simplified)

## Requirements

- Qt 6.4+
- libmpv
- X11 (Linux)

## Build

```bash
cmake -B build -S src -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Linux dependencies (Ubuntu 24.04)

```bash
sudo apt-get install cmake ninja-build \
    qt6-base-dev qt6-base-private-dev libqt6svg6-dev \
    qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
    libmpv-dev libx11-dev libgl-dev pkg-config
```

### Windows (MSYS2 / MinGW-w64)

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-svg mingw-w64-x86_64-qt6-tools \
    mingw-w64-x86_64-mpv mingw-w64-x86_64-libzip
```

## Install (Linux)

```bash
cmake --install build
```

Installs to `/usr/local` by default. Use `-DCMAKE_INSTALL_PREFIX=/usr` for system-wide install.

## License

[GPL v2](LICENSE)
