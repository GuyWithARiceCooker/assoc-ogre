#!/usr/bin/env bash
# Arch Linux: egy paste — **OGRE csomag + SDL/toolchain**, repo (**clone/pull**), **meadow jelenet** cmake build + **futtatás**.
# Használat:
#   curl -fsSL .../paste-run-arch.sh | bash
#   bash scripts/paste-run-arch.sh
set -euo pipefail

REPO="${ASSOC_OGRE_HOME:-$HOME/assoc-ogre}"
BRANCH="${ASSOC_OGRE_BRANCH:-cursor/meadow-scene-4764}"
URL="${ASSOC_OGRE_URL:-https://github.com/GuyWithARiceCooker/assoc-ogre.git}"

echo ">>> 1/4 Pacman: OGRE + git + SDL2 + cmake toolchain"
sudo pacman -S --needed --noconfirm git ogre sdl2 cmake ninja gcc

export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-/usr}"

if [[ ! -d "$REPO/.git" ]]; then
  echo ">>> 2/4 git clone $BRANCH -> $REPO"
  git clone --depth 1 -b "$BRANCH" "$URL" "$REPO"
else
  echo ">>> 2/4 git pull + checkout $BRANCH ($REPO)"
  cd "$REPO"
  git fetch origin "$BRANCH" 2>/dev/null || git fetch origin
  git checkout "$BRANCH"
  git pull --ff-only origin "$BRANCH" 2>/dev/null || git pull --ff-only || true
fi

cd "$REPO"
echo ">>> 3/4 projekt: cmake + fordítás (meadow); alapból Wayland SDL — distro ogre nélküle: ASSOC_OGRE_USE_XWAYLAND=1"
echo ">>> 4/4 Meadow scene: ./meadow (Esc kilépés)"
exec bash "$REPO/scripts/get-build-run-meadow.sh" --no-pull
