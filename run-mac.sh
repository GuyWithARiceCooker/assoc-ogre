#!/usr/bin/env bash
# assoc Ogre3D — Mac: a build könyvtárból indul (plugins.cfg, resources.cfg, ogre.cfg itt vannak).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/build"
exec ./assoc
