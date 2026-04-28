#!/usr/bin/env bash
# Ugyanaz mint run-meadow-x11.sh, bamboo_gap-hoz.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build/bamboo_gap"
if [[ ! -x "$BIN" ]]; then
  echo "Nincs futtatható: $BIN — cmake --build build --target bamboo_gap"
  exit 1
fi
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 "$BIN" "$@"
