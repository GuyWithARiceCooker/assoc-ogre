#!/usr/bin/env bash
# Arch Linux: egy paste — **OGRE csomag + SDL/toolchain**, repo (**clone/pull vagy ZIP**), **meadow** build + futtatás.
#
# Ha git nem megy: ASSOC_OGRE_FROM_ZIP=1 vagy automatikus ZIP fallback clone hiba után.
# Használat:
#   curl -fsSL .../paste-run-arch.sh | bash
#   bash scripts/paste-run-arch.sh
set -euo pipefail

REPO="${ASSOC_OGRE_HOME:-$HOME/assoc-ogre}"
BRANCH="${ASSOC_OGRE_BRANCH:-cursor/meadow-scene-4764}"
URL="${ASSOC_OGRE_URL:-https://github.com/GuyWithARiceCooker/assoc-ogre.git}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

fetch_via_zip() {
  echo ">>> Forrás: GitHub ZIP ($BRANCH) — git nélkül / git hiba után"
  bash "$SCRIPT_DIR/fetch-repo-zip.sh" "$REPO"
}

echo ">>> 1/4 Pacman: OGRE + git + SDL2 + cmake + unzip (ZIP-hez)"
sudo pacman -S --needed --noconfirm git ogre sdl2 cmake ninja gcc unzip curl

export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-/usr}"

if [[ "${ASSOC_OGRE_FROM_ZIP:-}" == "1" ]]; then
  fetch_via_zip
elif [[ ! -d "$REPO/.git" ]]; then
  echo ">>> 2/4 git clone $BRANCH -> $REPO"
  if ! git clone --depth 1 -b "$BRANCH" "$URL" "$REPO" 2>/dev/null; then
    echo ">>> git clone sikertelen — ZIP fallback"
    fetch_via_zip
  fi
else
  echo ">>> 2/4 git pull + checkout $BRANCH ($REPO)"
  cd "$REPO"
  if ! git fetch origin "$BRANCH" 2>/dev/null; then
    git fetch origin || true
  fi
  if ! git checkout "$BRANCH" 2>/dev/null; then
    echo ">>> checkout / fetch gond — próbáld: ASSOC_OGRE_FROM_ZIP=1 , vagy ZIP felülírás"
    cd /
    fetch_via_zip
  else
    if ! git pull --ff-only origin "$BRANCH" 2>/dev/null && ! git pull --ff-only 2>/dev/null; then
      echo ">>> git pull sikertelen — ZIP frissítés"
      cd /
      fetch_via_zip
    fi
  fi
fi

cd "$REPO"
echo ">>> 3/4 projekt: cmake + fordítás (meadow); distro ogre assert: ASSOC_OGRE_USE_XWAYLAND=1"
echo ">>> 4/4 Meadow scene: ./meadow (Esc kilépés)"
exec bash "$REPO/scripts/get-build-run-meadow.sh" --no-pull
