# assoc-ogre

Kis Ogre3D 14 (Bites) + RTShader demó: 3D jelenet, szöveg a jelenetben betűnként `ManualObject` + `SdkTrays/Caption` font, nem overlay.

## Arch — egy copy-paste (függőség + repo + meadow futtatás)

**Gyorsút:** bash shellben másold be **egyetlen blokkban** (sudo kérhet jelszót):

```bash
curl -fsSL https://raw.githubusercontent.com/GuyWithARiceCooker/assoc-ogre/cursor/meadow-scene-4764/scripts/paste-run-arch.sh | bash
```

Ez telepíti (`git`, `ogre`, `sdl2`, `cmake`, `ninja`, `gcc`), klónozza a repót `~/assoc-ogre`-ba (ha még nincs), checkout `cursor/meadow-scene-4764`, majd build + `./meadow`.

**Ha már klónoztál máshova:**

```bash
export ASSOC_OGRE_HOME=/útvonal/amire/tetted
curl -fsSL https://raw.githubusercontent.com/GuyWithARiceCooker/assoc-ogre/cursor/meadow-scene-4764/scripts/paste-run-arch.sh | bash
```

Ha CMake hiányolja a mintamédiát: előtte `export OGRE_SAMPLES_MEDIA=/útvonal/ogre/Samples/Media`.

---

## Részletes (kézi lépések)

### Függőségek

- Ogre3D 14.x telepítve (CMake `find_package(OGRE)` megtalálja a rendszeren vagy egy saját prefixben)
- SDL2 (Arch: `sudo pacman -S ogre sdl2 cmake ninja gcc`)

**Linux / Arch** — egy lépésben (pacman + cmake + build), **és ha akarod ugyanazon a gépen Tailscale-t is** (`tailscaled`):

```bash
./scripts/arch-setup-build.sh --tailscale
```

Csak fordítás (Tailscale nélkül):

```bash
./scripts/arch-setup-build.sh
```

Régi csak-Tailscale szkript: `./scripts/tailscale-arch-setup.sh`

**Egyben: pull → build → meadow jelenet** (nálad):

```bash
chmod +x scripts/get-build-run-meadow.sh   # egyszer
./scripts/get-build-run-meadow.sh
```

Első alkalom (clone + build + futtatás egy könyvtárban):

```bash
./scripts/get-build-run-meadow.sh https://github.com/GuyWithARiceCooker/assoc-ogre.git
```

**Megjegyzés:** Cursor/felhős VM-ekben gyakran **nincs TUN** (`/dev/net/tun`) — ott a Tailscale daemon **nem indul**. Valódi Arch laptopon / fizikai gépen általában oké minden.

**Ubuntu / Debian** (Tailscale csomag):

```bash
curl -fsSL https://tailscale.com/install.sh | sudo sh
sudo systemctl enable --now tailscaled
sudo tailscale up
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

## Tailscale (laptop ↔ másik gép)

**Alapelv:** mindkét gépen telepítve a Tailscale, **ugyanazzal a fiókkal** belépve ([login.tailscale.com](https://login.tailscale.com)) — így minden **magán IP-n** (`100.x.y.z`) elérhető egymástól, NAT/portnyitás nélkül.

### Arch — egy szkript

```bash
./scripts/tailscale-arch-setup.sh
```

Ez telepíti a `tailscale`-et és az `openssh`-t, elindítja a daemont, majd `tailscale up`-pal felkapcsol (kövesd a böngészőt / utasításokat). SSH távolról:

```bash
sudo systemctl enable --now sshd
ssh felhasználó@$(tailscale ip -4)
```

A másik gépen is: Tailscale telepítés + `tailscale up` ugyanazzal a fiókkal. Utána onnan: `ssh felhasználó@<laptop Tailscale IP>`.

### Mit ad meg a Tailscale itt

| Cél | Hogyan |
|-----|--------|
| **SSH** a laptop termináljára | TS IP + `sshd` (lásd fent) |
| **scp/rsync** projekt / fájl | `scp -r . felhasználó@100.x.y.z:assoc-ogre/` |
| **Kód szinkron** | GitHub (`git pull`) — TS nélkül is megy; TS = közvetlen SSH/rsync TS IP-n |

### OGRE ablak

Ha SSH-n futtatod `./meadow`-t, az **alapból a laptop kijelzőjén** nyílik (nem a távoli gépen). Távoli megjelenítéshez külön kell **X11 forward** (`ssh -Y`) vagy más (pl. waypipe), ez nem része ennek a projektnek.

Opciók az admin felületen: **MagicDNS** (hostname `laptopnév.tailnet…`), **ACL** / subnet route — lásd [Tailscale docs](https://tailscale.com/kb/).

## GitHub (új repó + push)

1. Egy olyan terminálban, ahol tudod használni a `gh` böngészős bejelentkezését, egyszer: `gh auth login`  
   (PAT-vel: `echo "$GITHUB_TOKEN" | gh auth login --with-token` — a tokennek kell joga repót hozni és pusholni.)
2. A repó gyökeréből: `./scripts/github-create-and-push.sh`  
   A szkript már nincs `origin` esetén létrehozza a **publikus** `assoc-ogre` repót a fiókodon és feltolja a `main`-t.  
   Szervezet alá: `export GITHUB_ORG=szervezet_neve` (és szükséges a jog a repóhoz), majd ugyanígy a szkript.

## Licenc

A projektkód a szerzőé; az Ogre3D / minta módik külön licencelésűek (lásd az Ogre SDK / Samples dokumentációját).
