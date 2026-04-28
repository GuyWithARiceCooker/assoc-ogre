#!/usr/bin/env bash
# Arch pacman Ogre SampleBrowser — NE így: /opt/ogre/samples/SampleBrowser (Wayland → externalWlDisplay assert).
# Használat (repo gyökérből):
#   ./scripts/run-ogre-samplebrowser-x11.sh
#
# Egy sor (másold be — Wayland env törlése + SDL x11 kötelező):
#   env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 /opt/ogre/samples/SampleBrowser
set -euo pipefail

for CAND in \
  "/opt/ogre/samples/SampleBrowser" \
  "/usr/bin/SampleBrowser_Ogre" \
  "$(command -v SampleBrowser 2>/dev/null)" \
  "$(command -v SampleBrowser_Ogre 2>/dev/null)"
do
  if [[ -n "$CAND" && -x "$CAND" ]]; then
    BIN="$CAND"
    break
  fi
done

if [[ -z "${BIN:-}" ]]; then
  echo "SampleBrowser nem található. Arch: pacman -Ql ogre | grep -i sample"
  exit 1
fi

echo ">>> Indítás: $BIN"
echo ">>> (WAYLAND_* törölve, SDL → x11 / XWayland — ha még Wayland log: pacman -S sdl2, sdl2-compat levétele)"
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  HOME="${HOME:-}" USER="${USER:-}" PATH="${PATH:-}" \
  DISPLAY="${DISPLAY:-:0}" \
  XAUTHORITY="${XAUTHORITY:-}" \
  SDL_VIDEODRIVER=x11 \
  SDL_VIDEO_DRIVER=x11 \
  "$BIN" "$@"
