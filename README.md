# assoc-ogre

Kis Ogre3D 14 (Bites) + RTShader demó: 3D jelenet, szöveg a jelenetben betűnként `ManualObject` + `SdkTrays/Caption` font, nem overlay.

## bamboo_gap — bambusz félhengerek (~90 egység nyílás)

Procedurális **félhengerek**, több **elrendezési minta** (lamella, dupla sor, rács, hullám, váltásos „lépcső”, ív magasságban, szórt). **Tab** vagy **]** következő, **[** előző, **Esc** kilépés. Futtatás: `build/bamboo_gap` (ugyanúgy mint `meadow`, `plugins.cfg` / `resources.cfg` mellett).

```bash
cmake --build build --target bamboo_gap
cd build && ./bamboo_gap
```

## Arch — **egy parancs**: rendszer **OGRE** + projekt **fordítás** + **meadow jelenet** futtatás

*(Ha **`git pull` / clone nem megy**, ugyanez a szkript **automatikusan ZIP-et tölt** GitHub-ról. Kötelező kényszerítés ZIP-re: `ASSOC_OGRE_FROM_ZIP=1`.)*

```bash
curl -fsSL https://raw.githubusercontent.com/GuyWithARiceCooker/assoc-ogre/cursor/meadow-scene-4764/scripts/paste-run-arch.sh | bash
```

**Git nélkül / elölről (csak curl + bash):**

```bash
export ASSOC_OGRE_FROM_ZIP=1
curl -fsSL https://raw.githubusercontent.com/GuyWithARiceCooker/assoc-ogre/cursor/meadow-scene-4764/scripts/paste-run-arch.sh | bash
```

Kézzel csak a forrás: `bash scripts/fetch-repo-zip.sh ~/assoc-ogre` (majd `cd ~/assoc-ogre && bash scripts/get-build-run-meadow.sh --no-pull` ha már fent van az OGRE).

Ugyanez, ha már klónoztál:

```bash
cd ~/assoc-ogre && bash scripts/paste-run-arch.sh
```

Alias név: `scripts/run-scene-arch.sh` → ugyanaz.

Ha CMake hiányolja a mintamédiát (`assoc` demóhoz): **`assoc`** nélkül is lefordul **`meadow`** és **`bamboo_gap`**. Az **`assoc`**-hoz kell `models/ogrehead.mesh` — adj meg útvonalat:

```bash
cmake -S . -B build -DOGRE_SAMPLES_MEDIA=/útvonal/ogre/Samples/Media
```

**Más klónozási könyvtár:**

```bash
export ASSOC_OGRE_HOME=/útvonal/amire/tetted
curl -fsSL https://raw.githubusercontent.com/GuyWithARiceCooker/assoc-ogre/cursor/meadow-scene-4764/scripts/paste-run-arch.sh | bash
```

**OGRE minták hol (Arch / Linux) — egy parancs:**

```bash
ls -la /opt/ogre/samples 2>/dev/null; ls -d /usr/share/OGRE-* 2>/dev/null; for d in /usr/share/OGRE-*/Media; do [[ -d "$d" ]] && echo "== $d ==" && ls "$d" | head -30; done; [[ -x /opt/ogre/samples/SampleBrowser ]] && echo "Futtatás: /opt/ogre/samples/SampleBrowser"
```

Vagy a repóban: `./scripts/where-ogre-samples.sh`

**Megjegyzés (Wayland + pacman Ogre):** a futtathatók **`SDL_HINT_VIDEODRIVER=x11`** erős prioritással állítják be az SDL-t az első init előtt (`AssocLinuxWaylandEnv.cpp`), nem csak env változó — így nem nyílik natív Wayland ablak a distro Ogre ellen.

1. **Saját Ogre Wayland prefix** (Arch; egyszer):

   ```bash
   PREFIX=$HOME/ogre-wayland bash scripts/build-ogre-wayland-prefix.sh
   ```

2. **Ez a projekt** abba a prefixbe kötve:

   ```bash
   export CMAKE_PREFIX_PATH=$HOME/ogre-wayland
   cmake -S . -B build && cmake --build build --target meadow
   cd build && ./meadow
   ```

   A bináris és `get-build-run-meadow.sh` Linuxon **alapból `SDL_VIDEODRIVER=x11`** (XWayland), ha nincs **`ASSOC_OGRE_WAYLAND_NATIVE=1`** — így a **pacman `ogre`** csomag stabil.

3. **Natív Wayland Ogre** (`OGRE_USE_WAYLAND=ON`): **`ASSOC_OGRE_WAYLAND_NATIVE=1`** + `CMAKE_PREFIX_PATH` a saját prefixre, majd `./meadow`.

---

## Részletes (kézi lépések)

### Függőségek

- Ogre3D 14.x telepítve (CMake `find_package(OGRE)` megtalálja a rendszeren vagy egy saját prefixben)
- SDL2 (Arch: `sudo pacman -S ogre sdl2 cmake ninja gcc pkgconf`)

**Linux / Arch** — egy lépésben (pacman + cmake + build):

```bash
./scripts/arch-setup-build.sh
```

**Egyben: pull → build → meadow jelenet** (nálad):

```bash
chmod +x scripts/get-build-run-meadow.sh   # egyszer
./scripts/get-build-run-meadow.sh
```

Első alkalom (clone + build + futtatás egy könyvtárban):

```bash
./scripts/get-build-run-meadow.sh https://github.com/GuyWithARiceCooker/assoc-ogre.git
```

**Megjegyzés:** Cursor/felhős VM-ekben gyakran **nincs kijelző** — ott helyben nem biztos, hogy fut az ablak.

Vagy kézzel (ha az `ogre` csomagban megvan a mintamédia az OGRE `Media` könyvtárában — gyakran így van):

```bash
cmake -S . -B build
cmake --build build
```

Ha CMake nem találja automatikusan a `Samples/Media`-t az `assoc` demóhoz (`models/ogrehead.mesh`), add meg kézzel vagy töltsd le az Ogre forrást:

```bash
cmake -S . -B build -DOGRE_SAMPLES_MEDIA=/path/to/ogre/Samples/Media
```

**Saját SDK-prefix** (opcionális): `-DOGRE_SDK=/path/to/prefix` (ahol van `CMake/OGREConfig.cmake` vagy `lib/cmake/OGRE/`).

A futtatáshoz a generált `build/plugins.cfg` és `build/resources.cfg` kell; indítás a `build/` könyvtárból: `./meadow`, `./bamboo_gap`, esetleg `./assoc`. macOS-en továbbra is használható a `run-mac.sh`, ha az útvonalak passzolnak.

Opcionális (SSH másik gépről): lásd `scripts/tailscale-arch-setup.sh` — **nem kell** az OGRE-hez.

## GitHub (új repó + push)

1. Egy olyan terminálban, ahol tudod használni a `gh` böngészős bejelentkezését, egyszer: `gh auth login`  
   (PAT-vel: `echo "$GITHUB_TOKEN" | gh auth login --with-token` — a tokennek kell joga repót hozni és pusholni.)
2. A repó gyökeréből: `./scripts/github-create-and-push.sh`  
   A szkript már nincs `origin` esetén létrehozza a **publikus** `assoc-ogre` repót a fiókodon és feltolja a `main`-t.  
   Szervezet alá: `export GITHUB_ORG=szervezet_neve` (és szükséges a jog a repóhoz), majd ugyanígy a szkript.

## Licenc

A projektkód a szerzőé; az Ogre3D / minta módik külön licencelésűek (lásd az Ogre SDK / Samples dokumentációját).
