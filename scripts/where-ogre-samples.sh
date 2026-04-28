#!/usr/bin/env bash
# Hol vannak az OGRE minták Arch / tipikus Linux telepítésen — egy futtatásban kilistázza.
set +e
echo "=== /opt/ogre/samples (bináris minták) ==="
ls -la /opt/ogre/samples 2>/dev/null || echo "(nincs — nem Arch csomag / más prefix)"

echo ""
echo "=== /usr/share/OGRE-* (cfg + Media) ==="
ls -d /usr/share/OGRE-* 2>/dev/null || echo "(nincs)"

for d in /usr/share/OGRE-*; do
  [[ -d "$d/Media" ]] || continue
  echo ""
  echo "=== $d/Media (rövid) ==="
  ls "$d/Media" 2>/dev/null | head -25
done

if [[ -x /opt/ogre/samples/SampleBrowser ]]; then
  echo ""
  echo "SampleBrowser: /opt/ogre/samples/SampleBrowser"
fi
