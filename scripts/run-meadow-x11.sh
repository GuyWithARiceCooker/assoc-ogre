#!/usr/bin/env bash
# Biztos XWayland / x11 SDL — teljes Wayland env eltávolítása (sdl2-compat workaround).
# Futtatás a repo gyökeréből: ./scripts/run-meadow-x11.sh
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build/meadow"
if [[ ! -x "$BIN" ]]; then
  echo "Nincs futtatható: $BIN — fordíts: cmake --build build --target meadow"
  exit 1
fi
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 "$BIN" "$@"
