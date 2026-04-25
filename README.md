# assoc-ogre

Kis Ogre3D 14 (Bites) + RTShader demó: 3D jelenet, szöveg a jelenetben betűnként `ManualObject` + `SdkTrays/Caption` font, nem overlay.

## Függőségek

- Ogre3D 14.x telepítve (CMake `OGREConfig.cmake` elérhető)
- SDL2
- A minta módokhoz/anyagokhoz: OGRE forráskönyvtár `Samples/Media` (hálók, `Examples/*` anyagok)

A `CMakeLists.txt` alapértelmezett elérési utakat használ (`OGRE_SDK`, `OGRE_SAMPLES_MEDIA`); módosítsd, ha nálad máshol vannak:

```bash
cmake -S . -B build \
  -DOGRE_SDK=/path/to/ogre-sdk \
  -DOGRE_SAMPLES_MEDIA=/path/to/ogre/Samples/Media
cmake --build build
```

A futtatáshoz a generált `build/plugins.cfg` és `build/resources.cfg` kell; indítás: `build/assoc` a build könyvtárból, vagy a projekt `run-mac.sh` (ha a gépen passzol az út).

## GitHub (új repó + push)

1. Egy olyan terminálban, ahol tudod használni a `gh` böngészős bejelentkezését, egyszer: `gh auth login`  
   (PAT-vel: `echo "$GITHUB_TOKEN" | gh auth login --with-token` — a tokennek kell joga repót hozni és pusholni.)
2. A repó gyökeréből: `./scripts/github-create-and-push.sh`  
   A szkript már nincs `origin` esetén létrehozza a **publikus** `assoc-ogre` repót a fiókodon és feltolja a `main`-t.  
   Szervezet alá: `export GITHUB_ORG=szervezet_neve` (és szükséges a jog a repóhoz), majd ugyanígy a szkript.

## Licenc

A projektkód a szerzőé; az Ogre3D / minta módik külön licencelésűek (lásd az Ogre SDK / Samples dokumentációját).
