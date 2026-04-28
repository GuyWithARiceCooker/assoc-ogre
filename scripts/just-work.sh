#!/usr/bin/env bash
# „Csak menjen”: Cursor VM / Arch — tun próba, pacman/ cmake, meadow X11.
# Futtatás a repo gyökérből: bash scripts/just-work.sh
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo ">>> [1/4] /dev/net/tun (Tailscale / VPN ha kell)"
if [[ ! -c /dev/net/tun ]] && command -v sudo >/dev/null 2>&1; then
  sudo mkdir -p /dev/net 2>/dev/null || true
  sudo mknod /dev/net/tun c 10 200 2>/dev/null || true
  sudo chmod 666 /dev/net/tun 2>/dev/null || sudo chmod 660 /dev/net/tun 2>/dev/null || true
fi
[[ -c /dev/net/tun ]] && echo "    OK: /dev/net/tun" || echo "    (nincs tun — Tailscale: tailscaled --tun=userspace-networking)"

echo ">>> [2/4] Függőségek (Arch)"
if command -v pacman >/dev/null 2>&1; then
  sudo pacman -S --needed --noconfirm ogre sdl2 cmake ninja gcc pkgconf git unzip curl 2>/dev/null || true
else
  echo "    nem Arch — hagyd ki vagy telepítsd kézzel: ogre sdl2 cmake ninja gcc pkgconf"
fi

echo ">>> [3/4] cmake + meadow"
cmake -S . -B build
cmake --build build --target meadow

echo ">>> [4/4] meadow (X11 / XWayland — NEM Wayland SDL)"
exec env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET -u WAYLAND_DEBUG \
  SDL_VIDEODRIVER=x11 SDL_VIDEO_DRIVER=x11 "$ROOT/build/meadow"
