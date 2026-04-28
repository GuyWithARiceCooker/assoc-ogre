#!/usr/bin/env bash
# Arch pacman Ogre SampleBrowser — ugyanaz az externalWlDisplay assert (SDL Wayland vs X11 EGL).
# Használat:
#   ./scripts/run-ogre-samplebrowser-x11.sh
# Vagy egy sor:
#   env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET SDL_VIDEODRIVER=x11 /opt/ogre/samples/SampleBrowser
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

echo ">>> $BIN (SDL x11 / XWayland, Wayland env nélkül)"
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 "$BIN" "$@"
