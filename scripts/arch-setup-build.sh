#!/usr/bin/env bash
# Arch Linux: OGRE + toolchain, cmake build (assoc ha van mintamédia, meadow, bamboo_gap).
# Futtatás a repo gyökérből: ./scripts/arch-setup-build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v pacman >/dev/null 2>&1; then
  echo "pacman nem található — nem Arch? Fordíts kézzel: cmake -S . -B build && cmake --build build"
  exit 1
fi

echo ">>> Pacman: ogre, sdl2, cmake, ninja, gcc, pkgconf"
sudo pacman -S --needed --noconfirm ogre sdl2 cmake ninja gcc pkgconf

echo ">>> CMake configure"
cmake -S . -B build

echo ">>> Build"
cmake --build build

echo ""
echo ">>> Kész. Futtatás:"
echo "    cd build && ./meadow"
echo "    cd build && ./bamboo_gap"
echo "    cd build && ./assoc   # ha fordult"
