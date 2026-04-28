#!/usr/bin/env bash
# SDL x11 / XWayland — Wayland env eltávolítása (pacman Ogre + sdl2-compat).
# Használat (repo gyökérből):
#   ./scripts/run-demo-x11.sh meadow
#   ./scripts/run-demo-x11.sh bamboo_gap
#   ./scripts/run-demo-x11.sh assoc
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO="${1:?demo név: meadow | bamboo_gap | assoc}"
shift || true
BIN="${ROOT}/build/${DEMO}"
if [[ ! -x "$BIN" ]]; then
  echo "Nincs: $BIN — cmake --build build --target ${DEMO}"
  exit 1
fi
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 "$BIN" "$@"
