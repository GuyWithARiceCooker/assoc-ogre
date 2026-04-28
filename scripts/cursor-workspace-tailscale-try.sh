#!/usr/bin/env bash
# Cursor / minimális VM: nincs /dev/net/tun → tailscaled elhasal. Két lehetőség:
#   (A) /dev/net létrehozása + tun node (ha van sudo, major 10 minor 200)
#   (B) tailscaled --tun=userspace-networking (TUN modul nélkül, lassabb de működik)
#
# Futtatás: bash scripts/cursor-workspace-tailscale-try.sh
# Utána:    sudo TAILSCALE_SOCKET=/tmp/tailscale.sock tailscale up
set -euo pipefail

if [[ -c /dev/net/tun ]]; then
  echo "OK: /dev/net/tun már létezik"
  exit 0
fi

echo ">>> Nincs /dev/net/tun — próbálom létrehozni (sudo kell)"
sudo mkdir -p /dev/net
if [[ ! -c /dev/net/tun ]]; then
  sudo mknod /dev/net/tun c 10 200
  sudo chmod 666 /dev/net/tun 2>/dev/null || sudo chmod 660 /dev/net/tun
fi
ls -la /dev/net/tun

echo ""
echo ">>> Ha innentől systemctl tailscaled megy, kész."
echo ">>> Ha még mindig hiba: userspace mód (nincs kernel TUN):"
echo "    sudo tailscaled --state=/var/lib/tailscale/tailscaled.state --socket=/run/tailscale/tailscale.sock --tun=userspace-networking &"
echo "    sudo tailscale up"
echo ""
echo "(Felhő IDE-ben teljes Tailscale gyakran nem kell — SSH/git a laptopodon.)"
