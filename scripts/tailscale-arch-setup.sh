#!/usr/bin/env bash
# Arch: Tailscale csomag már fent? — daemon + felkapcsolás + (opcionális) sshd
#   ./scripts/tailscale-arch-setup.sh
#   ./scripts/tailscale-arch-setup.sh --with-sshd
set -euo pipefail

WITH_SSHD=0
for a in "$@"; do
  case "$a" in
    --with-sshd) WITH_SSHD=1 ;;
  esac
done

if ! command -v pacman >/dev/null 2>&1; then
  echo "Arch (pacman) kell. Más OS: https://tailscale.com/download"
  exit 1
fi

echo ">>> Pacman: tailscale (ha még nincs), openssh"
sudo pacman -S --needed --noconfirm tailscale openssh

if systemctl is-system-running &>/dev/null; then
  echo ">>> tailscaled (systemd)"
  sudo systemctl enable --now tailscaled
else
  echo ">>> FIGYELEM: nincs futó systemd — manuálisan: sudo tailscaled &  vagy  sudo systemctl (helyi gépen)"
fi

echo ">>> tailscale up (böngésző / bejelentkezés — kövesd a kimenetet)"
sudo tailscale up

echo ""
echo ">>> Te Tailscale IP-d (másik TS-es gépről: ssh user@<ez>):"
tailscale ip -4 2>/dev/null || true
echo ""
tailscale status || true

if [[ "$WITH_SSHD" -eq 1 ]]; then
  if systemctl is-system-running &>/dev/null; then
    echo ">>> sshd (távoli SSH)"
    sudo systemctl enable --now sshd
  else
    echo ">>> sshd: helyi gépen: sudo systemctl enable --now sshd"
  fi
else
  echo ">>> Távoli SSH-hoz: ./scripts/tailscale-arch-setup.sh --with-sshd  vagy  sudo systemctl enable --now sshd"
fi
