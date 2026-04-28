#!/usr/bin/env bash
# Saját OGRE prefix OGRE_USE_WAYLAND=ON — ehhez kösd a projektet (CMAKE_PREFIX_PATH).
# Arch példa függőségek; más OS-en igazítsd.
# Használat: PREFIX=$HOME/ogre-wayland bash scripts/build-ogre-wayland-prefix.sh
set -euo pipefail

PREFIX="${PREFIX:-$HOME/ogre-wayland}"
SRC="${OGRE_SRC:-$(pwd)/ogre-src-wayland}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

if ! command -v pacman >/dev/null 2>&1; then
  echo "Ez a szkript pacman-t feltételez (Arch). Más OS-en telepítsd kézzel: wayland, wayland-protocols, mesa, libxkbcommon, ..."
else
  echo ">>> Pacman build deps (Arch)"
  sudo pacman -S --needed --noconfirm base-devel git cmake ninja \
    wayland wayland-protocols libxkbcommon mesa libgl libxi libxrandr libxaw libsm freetype2 zlib rapidjson \
    sdl2 freeimage pugixml zzip libxcursor libxinerama
fi

if [[ ! -d "$SRC/.git" ]]; then
  echo ">>> git clone OGRECave/ogre -> $SRC"
  git clone --depth 1 https://github.com/OGRECave/ogre.git "$SRC"
fi

echo ">>> cmake OGRE USE_WAYLAND -> $PREFIX"
cmake -S "$SRC" -B "$SRC/build-wayland" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DOGRE_USE_WAYLAND=ON \
  -DOGRE_BUILD_SAMPLES=OFF \
  -DOGRE_INSTALL_SAMPLES=OFF

echo ">>> build + install ($JOBS jobs)"
cmake --build "$SRC/build-wayland" -j"$JOBS"
cmake --install "$SRC/build-wayland"

echo ""
echo ">>> Kész. Projekt így:"
echo "    export CMAKE_PREFIX_PATH=$PREFIX"
echo "    cmake -S . -B build && cmake --build build --target meadow"
echo "    cd build && ./meadow"
