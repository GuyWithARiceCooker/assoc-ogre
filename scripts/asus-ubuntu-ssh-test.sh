#!/usr/bin/env bash
# Gyors health-check: van-e asus-ubuntu, mennyi erő, Android SDK, disk.
# Nem módosít semmit, csak olvas.
set -euo pipefail
H="${1:-asus-ubuntu}"
exec ssh -o BatchMode=yes -o ConnectTimeout=10 "$H" '
  echo "◆ host:      $(hostname)"
  echo "◆ load:     $(nproc) mag, uptime: $(uptime -p 2>/dev/null || true)"
  echo "◆ lemez:    $(df -h / | tail -1)"
  if test -d "$HOME/Android/Sdk"; then
    echo "◆ Android:   ~/Android/Sdk (megvan)"
  else
    echo "◆ Android:   (nincs ~/Android/Sdk – igény szerint install)"
  fi
'
