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
# Ha az OGRE nem standard helyen van (opcionális):
#   export CMAKE_PREFIX_PATH=/usr
#   export OGRE_SAMPLES_MEDIA=/path/to/ogre/Samples/Media
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

# Arch `ogre` package is often built without OGRE_USE_WAYLAND; SDL+Wayland then hits
# RuntimeAssertionException externalWlDisplay in OgreX11EGLWindow. Force X11 (XWayland).
if [[ -z "${SDL_VIDEODRIVER:-}" ]]; then
  if [[ "${XDG_SESSION_TYPE:-}" == wayland ]] || [[ -n "${WAYLAND_DISPLAY:-}" ]]; then
    export SDL_VIDEODRIVER=x11
    echo ">>> Wayland session: SDL_VIDEODRIVER=x11 (XWayland; avoids OGRE externalWlDisplay assert with distro ogre)"
  fi
fi

echo ">>> ./meadow (working dir: build/)"
cd "$ROOT/build"
exec ./meadow
