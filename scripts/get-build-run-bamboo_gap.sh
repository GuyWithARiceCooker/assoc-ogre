#!/usr/bin/env bash
# Mint get-build-run-meadow.sh, de bamboo_gap fordítás + futtatás (pacman Ogre: exec env x11).
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DO_PULL=1
ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-pull) DO_PULL=0 ;;
    *) ARGS+=("$1") ;;
  esac
  shift
done
set -- "${ARGS[@]}"

if [[ $# -ge 1 ]]; then
  case "$1" in
    https://*|git@*|ssh://*)
      CLONE_URL="$1"
      CLONE_DIR="${2:-assoc-ogre}"
      PARENT="$(pwd)"
      TARGET="${PARENT%/}/$(basename "$CLONE_DIR")"
      if [[ -e "$TARGET" ]]; then
        echo "Már létezik: $TARGET"
        exit 1
      fi
      git clone "$CLONE_URL" "$TARGET"
      exec bash "$TARGET/scripts/get-build-run-bamboo_gap.sh" --no-pull
      ;;
  esac
fi

cd "$ROOT"
if [[ ! -f CMakeLists.txt ]]; then
  echo "Repo gyökér kell (CMakeLists.txt)."
  exit 1
fi

if [[ "$DO_PULL" -eq 1 ]] && [[ -d .git ]]; then
  git pull --ff-only 2>/dev/null || git pull origin HEAD --ff-only 2>/dev/null || true
fi

CMAKE_EXTRA=()
[[ -n "${CMAKE_PREFIX_PATH:-}" ]] && CMAKE_EXTRA+=("-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
[[ -n "${OGRE_SAMPLES_MEDIA:-}" ]] && CMAKE_EXTRA+=("-DOGRE_SAMPLES_MEDIA=${OGRE_SAMPLES_MEDIA}")
[[ -n "${OGRE_SDK:-}" ]] && CMAKE_EXTRA+=("-DOGRE_SDK=${OGRE_SDK}")

cmake -S . -B build "${CMAKE_EXTRA[@]}"
cmake --build build --target bamboo_gap

if [[ "${ASSOC_OGRE_WAYLAND_NATIVE:-}" != "1" ]]; then
  exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
    SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}" \
    SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-x11}" \
    "$ROOT/build/bamboo_gap"
fi
exec "$ROOT/build/bamboo_gap"
