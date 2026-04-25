#!/usr/bin/env bash
# ==============================================================================
# assoc-ogre — forrásszinkron az ASUS-ubuntu gépnek (Ogre3D helyi munka / távoli build)
#
# Miért van ez: a Mac (8 GB, kétmagos) sokáig fordít; az ASUSon akár 8 mag,
# több tár, és sok Tomi-setupnál már fent van Android SDK. Ha ugyanaz a fájltörzs
# mindkét gépen kell, rsync a legjobb egyszerű eszköz.
#
# Feltételek: ssh-asus-ubuntu kulccsal működik; a távoli user home-ban legyen
# a célkönyvtár vagy a szkript hozza létre. Alap: REMOTE=asus-ubuntu, Remote dir:
#   ~/work/assoc-ogre  (a REMOTE_DIR változóval felülírható)
#
# A Mac kizárjai: .git, build, .DS_Store — a build helyben marad, a távoli gépen
# újra lehet futtatni cmake-et.
# ==============================================================================
set -euo pipefail
REMOTE="${REMOTE:-asus-ubuntu}"
REMOTE_DIR="${REMOTE_DIR:-work/assoc-ogre}"
LOCAL="${LOCAL:-$(cd "$(dirname "$0")/.." && pwd)}"
echo "▶ forrás (helyi):  $LOCAL"
echo "▶ távoli:          $REMOTE:~/\${REMOTE_DIR} → $REMOTE_DIR"
rsync -av --delete --exclude '.git' --exclude 'build' --exclude '.DS_Store' \
  "$LOCAL/" "$REMOTE:$REMOTE_DIR/"
echo "▶ kész. Távoli példa build (Ubuntu, NDK/CMake, ha fent vannak):"
echo "    ssh $REMOTE 'cd ~/$REMOTE_DIR && mkdir -p build && cd build && cmake .. && cmake --build . -j'"
