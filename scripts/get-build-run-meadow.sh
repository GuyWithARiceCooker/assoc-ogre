#!/usr/bin/env bash
# Letöltés (git pull vagy clone), cmake build, majd meadow jelenet indítása.
#
# Már klónozott repóban (gyökérből vagy bárhonnan):
#   ./scripts/get-build-run-meadow.sh
#
# Pull nélkül:
#   ./scripts/get-build-run-meadow.sh --no-pull
#
# Első alkalom — klónozás + build + futtatás:
#   ./scripts/get-build-run-meadow.sh https://github.com/GuyWithARiceCooker/assoc-ogre.git
#   ./scripts/get-build-run-meadow.sh https://github.com/GuyWithARiceCooker/assoc-ogre.git ../assoc-ogre
#
# Ha csak az Arch `ogre` csomagod van (nem Waylandes), és assert jön (régi bináris):
#   SDL_VIDEODRIVER=x11 ./meadow
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
        echo "Már létezik: $TARGET — lépj be: cd \"$TARGET\" && ./scripts/get-build-run-meadow.sh --no-pull"
        exit 1
      fi
      echo ">>> git clone $CLONE_URL -> $TARGET"
      git clone "$CLONE_URL" "$TARGET"
      exec bash "$TARGET/scripts/get-build-run-meadow.sh" --no-pull
      ;;
  esac
fi

cd "$ROOT"
if [[ ! -f CMakeLists.txt ]]; then
  echo "Nem a repo gyökér: $ROOT — futtasd a projekt assoc-ogre mappájából, vagy adj meg clone URL-t első argumentumként."
  exit 1
fi

if [[ "$DO_PULL" -eq 1 ]] && [[ -d .git ]]; then
  echo ">>> git pull"
  git pull --ff-only 2>/dev/null || git pull origin HEAD --ff-only 2>/dev/null || true
fi

echo ">>> cmake configure"
CMAKE_EXTRA=()
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
  CMAKE_EXTRA+=("-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
fi
if [[ -n "${OGRE_SAMPLES_MEDIA:-}" ]]; then
  CMAKE_EXTRA+=("-DOGRE_SAMPLES_MEDIA=${OGRE_SAMPLES_MEDIA}")
fi
if [[ -n "${OGRE_SDK:-}" ]]; then
  CMAKE_EXTRA+=("-DOGRE_SDK=${OGRE_SDK}")
fi
cmake -S . -B build "${CMAKE_EXTRA[@]}"

echo ">>> cmake build (meadow)"
cmake --build build --target meadow

echo ">>> ./meadow"
if [[ "${ASSOC_OGRE_WAYLAND_NATIVE:-}" != "1" ]]; then
  echo ">>> pacman Ogre: Wayland env eltávolítva (exec env); distro + SDL compat workaround"
  exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
    SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}" \
    SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-x11}" \
    "$ROOT/build/meadow"
fi
exec "$ROOT/build/meadow"
