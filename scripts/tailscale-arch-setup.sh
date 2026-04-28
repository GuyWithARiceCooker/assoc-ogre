#!/usr/bin/env bash
# Arch Linux: Tailscale telepítés, daemon, felkapcsolás (egyszeri beállítás).
# Mindkét gépen ugyanazzal a Tailscale-fiókkal kell belépni (login.tailscale.com).
# Használat: ./scripts/tailscale-arch-setup.sh
set -euo pipefail

if ! command -v pacman >/dev/null 2>&1; then
  echo "Ez a szkript Arch Linuxra készült (pacman). Más disztribúció: lásd README Tailscale szekció."
  exit 1
fi

echo ">>> Tailscale + openssh (SSH a TS IP-re másik gépről)"
sudo pacman -S --needed --noconfirm tailscale openssh

echo ">>> tailscaled szolgáltatás"
sudo systemctl enable --now tailscaled

echo ">>> Tailscale bejelentkezés / felkapcsolás (böngésző vagy kulcs — kövesd a kimenetet)"
sudo tailscale up

echo ""
echo ">>> Állapot (Tailscale IP — erre SSH másik gépről ugyanazon a TS-hálón):"
tailscale status || true

echo ""
echo "SSH szerver (opcionális, ha távolról terminált akarsz):"
echo "  sudo systemctl enable --now sshd"
echo "  ssh \$USER@$(tailscale ip -4 2>/dev/null || echo 'TS_IP')"
echo ""
echo "Mindkét gépen futtasd ezt a szkriptet (vagy más OS-en: tailscale.com/download),"
echo "és ugyanaz a Tailscale-fiók / ugyanaz a tailnet legyen."
