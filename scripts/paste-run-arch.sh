#!/usr/bin/env bash
# Arch Linux: OGRE csomag + SDL/toolchain, repo (git vagy ZIP), meadow build + futtatás.
# curl | bash esetén nincs BASH_SOURCE — a ZIP logika be van ágyazva (nem külső útvonal).
#
# Ha git nem megy: ASSOC_OGRE_FROM_ZIP=1 vagy automatikus ZIP fallback.
set -euo pipefail

REPO="${ASSOC_OGRE_HOME:-$HOME/assoc-ogre}"
BRANCH="${ASSOC_OGRE_BRANCH:-cursor/meadow-scene-4764}"
URL="${ASSOC_OGRE_URL:-https://github.com/GuyWithARiceCooker/assoc-ogre.git}"
OWNER_REPO="${ASSOC_OGRE_GITHUB:-GuyWithARiceCooker/assoc-ogre}"

fetch_via_zip() {
  echo ">>> Forrás: GitHub ZIP ($BRANCH) — git nélkül / curl-bash kompatibilis"
  local BRANCH_ESC="${BRANCH//\//%2F}"
  local ZIP_URL="https://github.com/${OWNER_REPO}/archive/refs/heads/${BRANCH_ESC}.zip"
  local TMP
  TMP="$(mktemp -d)"
  cleanup() { rm -rf "$TMP"; }
  trap cleanup EXIT
  curl -fsSL "$ZIP_URL" -o "$TMP/archive.zip"
  unzip -q "$TMP/archive.zip" -d "$TMP"
  local EXTRACTED
  EXTRACTED="$(find "$TMP" -mindepth 1 -maxdepth 1 -type d ! -name '*.zip' | head -1)"
  if [[ -z "$EXTRACTED" || ! -d "$EXTRACTED" ]]; then
    echo "Hiba: üres ZIP kicsomagolás"
    exit 1
  fi
  if [[ -e "$REPO" ]]; then
    echo ">>> Régi könyvtár törlése: $REPO"
    rm -rf "$REPO"
  fi
  mv "$EXTRACTED" "$REPO"
  trap - EXIT
  cleanup
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
    echo ">>> checkout / fetch gond — ZIP felülírás"
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
echo ">>> 3/4 projekt: cmake + meadow; Linux: SDL x11 alapból (ASSOC_OGRE_WAYLAND_NATIVE=1 natív WL Ogre)"
echo ">>> 4/4 ./meadow"
exec bash "$REPO/scripts/get-build-run-meadow.sh" --no-pull
