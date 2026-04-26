# assoc-ogre

Kis Ogre3D 14 (Bites) + RTShader demo: konnyu, asset-mentes, vizen uszo
modularis napelemes katamaran. A hajotestek ManualObject geometriabol epulnek,
kulso mesh/textura nelkul, hogy Linuxon/Archon es regi, 4 GB RAM-os gepen is
visszafogottan fusson.

## Fuggosegek

- Ogre3D 14.x telepitve (CMake `OGREConfig.cmake` elerheto)
- SDL2

Ha nincs meg Ogre, a repo tud helyi, minimalis Ogre 14 SDK-t epiteni
`.deps/ogre-14` ala:

```bash
./scripts/install-ogre-local.sh
```

A `CMakeLists.txt` alapertelmezett Ogre prefixe `/usr`, de megadhatod kezzel:

```bash
cmake -S . -B build \
  -DOGRE_SDK=/path/to/ogre-sdk
cmake --build build
```

Vagy Arch/sajat Ogre prefix eseten:

```bash
./scripts/build-arch.sh /usr
```

Helyi Ogre install utan:

```bash
OGRE_PLUGIN_DIR=.deps/ogre-14/lib/OGRE ./scripts/build-arch.sh .deps/ogre-14
./scripts/run-linux.sh .deps/ogre-14
```

A script az elso argumentumot `OGRE_SDK` prefixkent hasznalja, es ha nem adsz meg
`OGRE_PLUGIN_DIR` valtozot, megprobalja a szokasos `lib/OGRE`, `lib64/OGRE`,
`lib/OGRE-14` konyvtarakat.

A futtatashoz a generalt `build/plugins.cfg` es `build/resources.cfg` kell:

```bash
cd build
./assoc
```

Ha helyi `.deps/ogre-14` SDK-val forditottad, hasznald inkabb:

```bash
./scripts/run-linux.sh .deps/ogre-14
```

Billentyuk:

- `Space`: a harom hajotest szet-/osszedokkolasa
- `Esc`: kilepes

## Arch Linux / regi MacBook 2009 late cel

Archon tipikus csomagok:

```bash
sudo pacman -S --needed base-devel cmake sdl2
# Ogre3D 14 telepitesedtol fuggoen: disztro/AUR/sajat build,
# vagy a repo helyi installer scriptje:
./scripts/install-ogre-local.sh
```

Ha az Ogre sajat prefixbe kerult:

```bash
./scripts/build-arch.sh /opt/ogre-14
cd build
./assoc
```

Kimeletes beallitasok a regi, 4 GB RAM-os gephez:

- OpenGL 3+ render system, 1024x640 ablak
- FSAA=0, VSync=Yes
- nincs arnyek, nincs post-process, nincs textura/mesh sample media
- a viz egyszeru plane, a hajo es egbolt alacsony poligonszamu ManualObject

Ha gyenge az iGPU, inditas utan az `build/ogre.cfg` fajlban meg lejjebb veheted
a `Video Mode` sort peldaul `800 x 600`-ra.

## GitHub (uj repo + push)

1. Egy olyan terminalban, ahol tudod hasznalni a `gh` bongeszos bejelentkezeset, egyszer: `gh auth login`
   (PAT-vel: `echo "$GITHUB_TOKEN" | gh auth login --with-token` - a tokennek kell joga repot hozni es pusholni.)
2. A repo gyokerebol: `./scripts/github-create-and-push.sh`
   A szkript mar nincs `origin` eseten letrehozza a **publikus** `assoc-ogre` repot a fiokodon es feltolja a `main`-t.
   Szervezet ala: `export GITHUB_ORG=szervezet_neve` (es szukseges a jog a repohoz), majd ugyanigy a szkript.

## Licenc

A projektkod a szerzoe; az Ogre3D kulon licencelesu.
