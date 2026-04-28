#!/usr/bin/env bash
# Arch Linux: egy paste — függőségek, repo (clone vagy pull), meadow build + futtatás.
# Használat:
#   curl -fsSL ... | bash
#   bash scripts/paste-run-arch.sh
set -euo pipefail

REPO="${ASSOC_OGRE_HOME:-$HOME/assoc-ogre}"
BRANCH="${ASSOC_OGRE_BRANCH:-cursor/meadow-scene-4764}"
URL="${ASSOC_OGRE_URL:-https://github.com/GuyWithARiceCooker/assoc-ogre.git}"

echo ">>> Pacman (sudo): git, ogre, sdl2, cmake, ninja, gcc"
sudo pacman -S --needed --noconfirm git ogre sdl2 cmake ninja gcc

export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-/usr}"

if [[ ! -d "$REPO/.git" ]]; then
  echo ">>> git clone $BRANCH -> $REPO"
  git clone --depth 1 -b "$BRANCH" "$URL" "$REPO"
else
  echo ">>> git pull ($REPO)"
  cd "$REPO"
  git fetch origin "$BRANCH" 2>/dev/null || git fetch origin
  git checkout "$BRANCH"
  git pull --ff-only origin "$BRANCH" 2>/dev/null || git pull --ff-only || true
fi

cd "$REPO"
echo ">>> meadow build + run"
exec bash "$REPO/scripts/get-build-run-meadow.sh" --no-pull
