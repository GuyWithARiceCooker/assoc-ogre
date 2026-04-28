#!/usr/bin/env bash
# SampleBrowser ugyanazzal a „csak menjen” env-el (pacman ogre).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec bash "$ROOT/scripts/run-ogre-samplebrowser-x11.sh" "$@"
