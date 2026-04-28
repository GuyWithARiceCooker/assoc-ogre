#!/usr/bin/env bash
# Ha git nem elérhető / clone-pull hibázik: GitHub ZIP a branchről, kicsomagolás DEST-be.
# Használat:
#   ASSOC_OGRE_BRANCH=cursor/meadow-scene-4764 bash scripts/fetch-repo-zip.sh ~/assoc-ogre
set -euo pipefail

DEST="${1:-$HOME/assoc-ogre}"
BRANCH="${ASSOC_OGRE_BRANCH:-cursor/meadow-scene-4764}"
OWNER_REPO="${ASSOC_OGRE_GITHUB:-GuyWithARiceCooker/assoc-ogre}"

BRANCH_ESC="${BRANCH//\//%2F}"
ZIP_URL="https://github.com/${OWNER_REPO}/archive/refs/heads/${BRANCH_ESC}.zip"

echo ">>> ZIP letöltés: $ZIP_URL"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
curl -fsSL "$ZIP_URL" -o "$TMP/archive.zip"

command -v unzip >/dev/null 2>&1 || { echo "Telepítsd: unzip (Arch: sudo pacman -S unzip)"; exit 1; }
unzip -q "$TMP/archive.zip" -d "$TMP"

EXTRACTED="$(find "$TMP" -mindepth 1 -maxdepth 1 -type d ! -name '*.zip' | head -1)"
if [[ -z "$EXTRACTED" || ! -d "$EXTRACTED" ]]; then
  echo "Hiba: üres kicsomagolás"
  exit 1
fi

if [[ -e "$DEST" ]]; then
  echo ">>> Régi mappa törlése: $DEST"
  rm -rf "$DEST"
fi
mv "$EXTRACTED" "$DEST"
echo ">>> Kész: $DEST"
