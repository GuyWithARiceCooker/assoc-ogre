#!/usr/bin/env bash
# Arch Linux: OGRE build + opcionálisan Tailscale ugyanazon a gépen (tailscaled).
# Futtatás a repo gyökérből:
#   ./scripts/arch-setup-build.sh
#   ./scripts/arch-setup-build.sh --tailscale   # build után TS + sshd előkészítés
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

WITH_TS=0
for a in "$@"; do
  case "$a" in
    --tailscale) WITH_TS=1 ;;
  esac
done

if ! command -v pacman >/dev/null 2>&1; then
  echo "pacman nem található — nem Arch? Fordíts kézzel: cmake -S . -B build && cmake --build build"
  exit 1
fi

echo ">>> Pacman: ogre, sdl2, cmake, ninja, gcc"
sudo pacman -S --needed --noconfirm ogre sdl2 cmake ninja gcc

echo ">>> CMake configure"
cmake -S . -B build

echo ">>> Build"
cmake --build build

echo ""
echo ">>> Kész (OGRE build). Futtatás:"
echo "    cd build && ./meadow"
echo "    cd build && ./assoc"

if [[ "$WITH_TS" -eq 1 ]]; then
  echo ""
  echo ">>> Ugyanezen a gépen: Tailscale (tailscaled) + openssh"
  sudo pacman -S --needed --noconfirm tailscale openssh
  sudo systemctl enable --now tailscaled
  echo ">>> Felkapcsolás (belépés — kövesd a kimenetet / böngészőt):"
  sudo tailscale up
  echo ""
  tailscale status || true
  echo ""
  echo "SSH távolról (másik TS-es gépről): sudo systemctl enable --now sshd"
  echo "  ssh \$USER@$(tailscale ip -4 2>/dev/null || echo TS_IP)"
fi
