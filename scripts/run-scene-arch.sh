#!/usr/bin/env bash
# Arch: egy parancs = rendszer OGRE (pacman) + ez a projekt (clone/pull) + meadow scene fordítás + futtatás.
# Ugyanaz, mint paste-run-arch.sh — csak beszédesebb név.
exec "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/paste-run-arch.sh" "$@"
