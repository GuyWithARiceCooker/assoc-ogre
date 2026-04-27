#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_dir}/build"

: "${OGRE_SDK:=${1:-${repo_dir}/.deps/ogre-14}}"

if [[ ! -x "${build_dir}/assoc" ]]; then
  echo "Nincs build/assoc. Elobb futtasd:" >&2
  echo "  ./scripts/build-arch.sh \"${OGRE_SDK}\"" >&2
  exit 1
fi

export LD_LIBRARY_PATH="${OGRE_SDK}/lib:${LD_LIBRARY_PATH:-}"

# Ogre GL3Plus + SDL2 is more predictable through X11/XWayland on older Mesa
# drivers. GNOME Wayland can otherwise show a black window and close.
if [[ -n "${WAYLAND_DISPLAY:-}" && -n "${DISPLAY:-}" && -z "${SDL_VIDEODRIVER:-}" ]]; then
  export SDL_VIDEODRIVER=x11
fi

echo "assoc-ogre inditas"
echo "  OGRE_SDK=${OGRE_SDK}"
echo "  SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-auto}"
echo "  ASSOC_VIDEO_MODE=${ASSOC_VIDEO_MODE:-1024 x 640}"
echo "  log: ~/.cache/assoc/ogre.log"

cd "${build_dir}"
exec ./assoc "$@"
