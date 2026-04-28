# assoc-ogre

Kis Ogre3D 14 (Bites) + RTShader demó: 3D jelenet, szöveg a jelenetben betűnként `ManualObject` + `SdkTrays/Caption` font, nem overlay.

## Függőségek

- Ogre3D 14.x telepítve (CMake `find_package(OGRE)` megtalálja a rendszeren vagy egy saját prefixben)
- SDL2 (Arch: `sudo pacman -S ogre sdl2 cmake ninja gcc`)

**Linux / Arch** — egy lépésben (pacman + cmake + build):

```bash
./scripts/arch-setup-build.sh
```

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

A futtatáshoz a generált `build/plugins.cfg` és `build/resources.cfg` kell; indítás a `build/` könyvtárból: `./assoc` vagy `./meadow`. macOS-en továbbra is használható a `run-mac.sh`, ha az útvonalak passzolnak.

## Tailscale-kapcsolat (távoli gép ↔ laptop)

Ha a **laptopodon** fordítasz és futtatsz, de máshonnan (másik gépről) akarsz **SSH-t vagy fájlmásolást** ugyanazon a magánhálón:

1. **Arch laptop:** telepítés és felkapcsolás (egyszer):
   ```bash
   sudo pacman -S tailscale
   sudo systemctl enable --now tailscaled
   sudo tailscale up
   ```
   A `tailscale status` kiírja a laptop **Tailscale IP**-jét (pl. `100.x.y.z`).

2. **Másik gépről SSH** a projekthez / terminálhoz (ha az SSH szerver fut a laptopon):
   ```bash
   ssh felhasználó@100.x.y.z
   ```
   Innen ugyanúgy `git pull`, `./scripts/arch-setup-build.sh`, majd `cd build && ./meadow` — de az **ablak a laptop kijelzőjén** nyílik meg (helyi X/Wayland), hacsak nem állítasz be **X11 továbbítást** (`ssh -Y`) vagy más távoli megjelenítést.

3. **Repó / kód szinkron** Tailscale-en keresztül nem kötelező: elég a **GitHub** (`git pull` / `git push`). A Tailscale inkább a **biztonságos elérést** adja (SSH, scp, rsync) a két gép között ugyanazon a TS-hálón.

## GitHub (új repó + push)

1. Egy olyan terminálban, ahol tudod használni a `gh` böngészős bejelentkezését, egyszer: `gh auth login`  
   (PAT-vel: `echo "$GITHUB_TOKEN" | gh auth login --with-token` — a tokennek kell joga repót hozni és pusholni.)
2. A repó gyökeréből: `./scripts/github-create-and-push.sh`  
   A szkript már nincs `origin` esetén létrehozza a **publikus** `assoc-ogre` repót a fiókodon és feltolja a `main`-t.  
   Szervezet alá: `export GITHUB_ORG=szervezet_neve` (és szükséges a jog a repóhoz), majd ugyanígy a szkript.

## Licenc

A projektkód a szerzőé; az Ogre3D / minta módik külön licencelésűek (lásd az Ogre SDK / Samples dokumentációját).
