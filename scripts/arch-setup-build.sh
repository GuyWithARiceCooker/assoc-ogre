#!/usr/bin/env bash
# Arch Linux: ogre + SDL2 + toolchain, majd cmake build (assoc + meadow).
# Futtatás a repo gyökeréből: ./scripts/arch-setup-build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v pacman >/dev/null 2>&1; then
  echo "pacman nem található — nem Arch? Fordíts kézzel: cmake -S . -B build && cmake --build build"
  exit 1
fi

echo ">>> Pacman: ogre, sdl2, cmake, ninja, gcc"
sudo pacman -S --needed --noconfirm ogre sdl2 cmake ninja gcc

echo ">>> CMake configure"
cmake -S . -B build

echo ">>> Build"
cmake --build build

echo ">>> Kész. Futtatás (a build könyvtárból, ott vannak a .cfg fájlok):"
echo "    cd build && ./meadow"
echo "    cd build && ./assoc"
