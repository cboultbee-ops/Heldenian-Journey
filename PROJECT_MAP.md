# PROJECT_MAP.md — Helbreath International All-In-One

Comprehensive project directory reference for offline guidance. Generated 2026-02-22.

---

## 1. Full Directory Tree

```
C:\Helbreath Project\
│
├── .vscode/                          # VS Code settings
├── Auto Updater Server/              # Auto-update server binary
├── asset_pipeline/                   # Sprite upscaling & processing pipeline
│   ├── __pycache__/
│   ├── defringe_test/                # Defringe visual test outputs
│   ├── pipeline_out/
│   │   ├── comparisons/             # Before/after sprite comparisons
│   │   └── ready/                   # 336 folders of processed sprites
│   ├── realesrgan/                  # Real-ESRGAN upscaler binary + models
│   └── steps/                       # Pipeline step modules
│
├── build/                            # PyInstaller build intermediate
│
├── Client/                           # CLIENT RUNTIME (Play_Me.exe lives here)
│   ├── CONTENTS/                    # Game data text files
│   │   ├── friends/
│   │   ├── mutes/
│   │   └── Shop/                    # 6 shop content files
│   ├── FONTS/                       # 7 font files (.FNT)
│   ├── MAPDATA/                     # 96 map data files
│   ├── MUSIC/                       # 11 music files
│   ├── shaders/                     # GLSL shaders (sprite.vert, sprite.frag)
│   ├── SOUNDS/                      # 228 sound files
│   ├── SPRITES/                     # 685 files (336 .pak + 336 .pak.bak + extras)
│   └── SPRITES(original)/           # 336 sprite PAK files (original backups)
│
├── dist/                             # PyInstaller output
│
├── gigapixel_input/                  # Topaz Gigapixel input (325 category folders)
├── gigapixel_output/                 # Topaz Gigapixel 2x output (327 entries)
├── gigapixel_staging/                # Staging area for Gigapixel batches
├── gigapixel_staging_out/
│
├── Login/                            # LOGIN SERVER RUNTIME
│   ├── Config/                      # Server configuration files (22 files)
│   │   └── Items/                   # 15 item category config files
│   └── Logs/
│       ├── GM/
│       └── Item/
│
├── Maps/                             # GAME SERVER RUNTIME (4 servers)
│   ├── configs/                     # Shared configs (Crusade, Schedule, droplists)
│   ├── MAPDATA/                     # 35 shared map files
│   ├── Logs/
│   ├── Events/                      # Events game server instance (port 3001)
│   │   ├── GameData/
│   │   ├── Logs/
│   │   └── MAPDATA/                 # 10 map files
│   ├── Middleland/                  # Middleland game server instance (port 3007)
│   │   ├── GameData/
│   │   ├── Logs/
│   │   └── MAPDATA/                 # 10 map files
│   ├── Neutrals/                    # Neutrals game server instance (port 3008)
│   │   ├── GameData/
│   │   ├── Logs/
│   │   └── MAPDATA/                 # 17 map files
│   └── Towns/                       # Towns game server instance (port 3002)
│       ├── GameData/
│       ├── Logs/
│       └── MAPDATA/                 # 84 map files (42 .amd + 42 .txt)
│
└── Sources/                          # ALL SOURCE CODE (~130K lines total)
    ├── All In One.sln               # Master solution (Client + HG + Login)
    ├── All In One.vcxproj           # Stub/placeholder project
    ├── Client/                      # Game client source
    │   ├── Client.sln / Client.vcxproj
    │   ├── char/                    # Character types
    │   │   └── item/               # Item types
    │   ├── DirectX/                 # Rendering, input, sound engine
    │   ├── include/                 # Vendored headers (GL/, GLFW/)
    │   ├── Map/                     # Map/tile classes
    │   ├── Net/                     # Network socket/message classes
    │   ├── Res/                     # Win32 resources
    │   └── UI/                      # Window management + GLEW/GLFW libs
    ├── HG/                          # Game server source
    │   ├── HGserver.sln / HGserver.vcxproj
    │   ├── char/                    # Server character/NPC classes
    │   │   └── item/               # Server item classes
    │   ├── droplist/               # XML drop list system
    │   ├── Map/                     # Server map/tile classes
    │   ├── Net/                     # Server network classes
    │   ├── ui/                      # Server window UI
    │   └── xsd-include/            # Vendored Xerces-C + XSD headers
    ├── Login/                       # Login server source
    │   ├── Login.vcxproj
    │   ├── char/                    # Login character classes
    │   │   └── item/
    │   ├── mysql/                   # Vendored MySQL C API headers
    │   ├── net/                     # Login network classes
    │   └── res/                     # Win32 resources
    └── Shared/                      # Shared headers + libraries
        └── lib/                     # 23 static libs + DLLs
```

---

## 2. Key Executables & Their Locations

| Full Path | Description | Built or Pre-existing |
|-----------|-------------|----------------------|
| `Client\Play_Me.exe` | Client (Release build) | Built from Client.vcxproj |
| `Client\Client_d.exe` | Client (Debug build) | Built from Client.vcxproj |
| `Client\Updater.exe` | Client auto-updater | Pre-existing |
| `Login\Login.exe` | Login server | Built from Login.vcxproj |
| `Maps\HGserver.exe` | Game server (root/spare copy) | Built from HGserver.vcxproj |
| `Maps\Towns\HGserver.exe` | Game server — Towns | Manually copied from Maps\ |
| `Maps\Neutrals\HGserver.exe` | Game server — Neutrals | Manually copied from Maps\ |
| `Maps\Middleland\HGserver.exe` | Game server — Middleland | Manually copied from Maps\ |
| `Maps\Events\HGserver.exe` | Game server — Events | Manually copied from Maps\ |
| `Helbreath Server Manager.exe` | Server manager GUI | PyInstaller from server_manager.py |
| `dist\Helbreath Server Manager.exe` | Server manager (dist copy) | PyInstaller output |
| `Auto Updater Server\Helbreath Update Server.exe` | Update server | Pre-existing |
| `asset_pipeline\realesrgan\realesrgan-ncnn-vulkan.exe` | Real-ESRGAN upscaler | Pre-existing |

### Build Output Paths

| Project | Debug OutDir | Debug TargetName | Release OutDir | Release TargetName |
|---------|-------------|------------------|----------------|-------------------|
| Client | `..\..\Client` | `Client_d` | `..\..\Client` | `Play_Me` |
| HGserver | `..\..\Maps` | `HGserver` | `..\..\Maps` | `HGserver` |
| Login | `..\..\Login` | `Login` | `..\..\Login\` | `Login` |

**Note:** HGserver builds to `Maps\HGserver.exe` (root). The 4 server subfolders (Towns, Neutrals, Middleland, Events) each need a manual copy of HGserver.exe after building.

---

## 3. Source Code Layout

### Solution Files

| Solution | Path | Contains |
|----------|------|----------|
| All In One | `Sources\All In One.sln` | Client + HG + Login |
| Client | `Sources\Client\Client.sln` | Client only |
| HG Server | `Sources\HG\HGserver.sln` | Game server only |

### Project Files

| vcxproj | Path |
|---------|------|
| Client | `Sources\Client\Client.vcxproj` |
| HGserver | `Sources\HG\HGserver.vcxproj` |
| Login | `Sources\Login\Login.vcxproj` |
| All In One (stub) | `Sources\All In One.vcxproj` |

### Key Source Files with Line Counts

#### Client — Core (~55K lines of game logic)

| File | Full Path | Lines |
|------|-----------|------:|
| Game.cpp | `Sources/Client/Game.cpp` | 39,016 |
| Game.h | `Sources/Client/Game.h` | 1,050 |
| MessageHandler.cpp | `Sources/Client/MessageHandler.cpp` | 6,332 |
| MessageHandler.h | `Sources/Client/MessageHandler.h` | 132 |
| SoundManager.cpp | `Sources/Client/SoundManager.cpp` | 191 |
| SoundManager.h | `Sources/Client/SoundManager.h` | 63 |
| Misc.cpp | `Sources/Client/Misc.cpp` | 408 |
| lan_eng.h | `Sources/Client/lan_eng.h` | 2,226 |

#### Client — DirectX / Rendering

| File | Full Path | Lines |
|------|-----------|------:|
| GPURenderer.cpp | `Sources/Client/DirectX/GPURenderer.cpp` | 1,291 |
| GPURenderer.h | `Sources/Client/DirectX/GPURenderer.h` | 215 |
| ShaderManager.cpp | `Sources/Client/DirectX/ShaderManager.cpp` | 149 |
| DXC_ddraw.cpp | `Sources/Client/DirectX/DXC_ddraw.cpp` | 1,022 |
| DXC_ddraw.h | `Sources/Client/DirectX/DXC_ddraw.h` | 95 |
| Sprite.cpp | `Sources/Client/DirectX/Sprite.cpp` | 4,071 |
| Sprite.h | `Sources/Client/DirectX/Sprite.h` | 121 |
| DXC_dinput.cpp | `Sources/Client/DirectX/DXC_dinput.cpp` | 187 |
| DXC_dinput.h | `Sources/Client/DirectX/DXC_dinput.h` | 45 |
| SoundBuffer.cpp | `Sources/Client/DirectX/SoundBuffer.cpp` | 266 |
| YWSound.cpp | `Sources/Client/DirectX/YWSound.cpp` | 57 |
| Effect.cpp | `Sources/Client/DirectX/Effect.cpp` | 23 |

#### Client — Other Subsystems

| File | Full Path | Lines |
|------|-----------|------:|
| MapData.cpp | `Sources/Client/Map/MapData.cpp` | 4,091 |
| Tile.cpp | `Sources/Client/Map/Tile.cpp` | 89 |
| XSocket.cpp | `Sources/Client/Net/XSocket.cpp` | 588 |
| Wmain.cpp | `Sources/Client/UI/Wmain.cpp` | 592 |

#### HG Server (~52K lines)

| File | Full Path | Lines |
|------|-----------|------:|
| HG.cpp | `Sources/HG/HG.cpp` | 42,182 |
| HG.h | `Sources/HG/HG.h` | 1,016 |
| Client.cpp | `Sources/HG/char/Client.cpp` | 1,212 |
| Client.h | `Sources/HG/char/Client.h` | 403 |
| combat.cpp | `Sources/HG/char/combat.cpp` | 1,816 |
| Npc.cpp | `Sources/HG/char/Npc.cpp` | 1,597 |
| Map.cpp | `Sources/HG/Map/Map.cpp` | 889 |
| XSocket.cpp | `Sources/HG/Net/XSocket.cpp` | 577 |
| droplist.cxx | `Sources/HG/droplist.cxx` | 918 |
| DropManager.cpp | `Sources/HG/DropManager.cpp` | 226 |
| Astoria.cpp | `Sources/HG/Astoria.cpp` | 162 |

#### Login Server (~6.3K lines)

| File | Full Path | Lines |
|------|-----------|------:|
| LoginServer.cpp | `Sources/Login/LoginServer.cpp` | 4,258 |
| LoginServer.h | `Sources/Login/LoginServer.h` | 154 |
| main.cpp | `Sources/Login/main.cpp` | 610 |
| GameServer.cpp | `Sources/Login/GameServer.cpp` | 93 |
| XSocket.cpp | `Sources/Login/net/XSocket.cpp` | 548 |
| PartyManager.cpp | `Sources/Login/char/PartyManager.cpp` | 290 |

#### Shared (~2.2K lines)

| File | Full Path | Lines |
|------|-----------|------:|
| NetMessages.h | `Sources/Shared/NetMessages.h` | 601 |
| items.h | `Sources/Shared/items.h` | 750 |
| common.h | `Sources/Shared/common.h` | 207 |
| maps.h | `Sources/Shared/maps.h` | 103 |
| npcType.h | `Sources/Shared/npcType.h` | 125 |
| magicID.h | `Sources/Shared/magicID.h` | 88 |

### Grand Total

| Component | Lines (approx) |
|-----------|---------------:|
| Client (all source) | ~69,000 |
| HG Server (all source) | ~52,000 |
| Login Server (all source) | ~6,300 |
| Shared headers | ~2,200 |
| **Total project source** | **~130,000** |

The two largest files — `Game.cpp` (39K) and `HG.cpp` (42K) — together account for ~62% of all project source code.

---

## 4. Asset Pipeline

### Scripts in `asset_pipeline/`

| Script | Description |
|--------|-------------|
| `run_pipeline.py` | Main pipeline runner: unpack → colorkey → upscale → repack |
| `pak_unpack.py` | Unpack PAK files: extract per-sprite BMPs and metadata |
| `pak_repack.py` | Repack sprites into PAK with brush scaling |
| `pak_utils.py` | PAK format constants and shared types |
| `repack_all.py` | Batch repack ALL 325 game categories (2x) + 11 UI (1x originals) |
| `upscale_batch.py` | Batch 2x upscale using Real-ESRGAN ncnn-vulkan |
| `defringe.py` | Defringe v1 — original color-key fringe removal |
| `defringe_v3.py` | Defringe v3 — interior color propagation + erosion (aggressive) |
| `defringe_v4.py` | Defringe v4 — safe edge cleanup, no erosion, size-aware |
| `gigapixel_auto.py` | Full GUI automation of Topaz Gigapixel |
| `gigapixel_batcher.py` | Semi-manual Gigapixel batch processing |
| `gigapixel_fixup.py` | Fix categories that exceeded MAX_BATCH_SIZE |
| `gigapixel_test.py` | Gigapixel automation test (v7) |
| `estimate_pipeline_cost.py` | Estimate total pipeline cost (fal.ai pricing) |
| `check_cuda_rembg.py` | Check CUDA/cuDNN availability for rembg GPU |
| `fix_cudnn_path.py` | Copy cuDNN DLLs for ONNX Runtime GPU |

#### Step Modules (`asset_pipeline/steps/`)

| Script | Description |
|--------|-------------|
| `colorkey_alpha.py` | Color-key → alpha: convert BMP sprites to RGBA PNGs |
| `upscale.py` | Upscale step (Pillow, fal.ai ESRGAN, or Replicate) |
| `bg_remove.py` | Background removal (rembg, Bria, fal.ai, etc.) |
| `png_to_bmp32.py` | Convert PNG with alpha to 32-bit BMP for PAK repack |

### Pipeline Workflow

```
Client/SPRITES/*.pak (336 originals)
    │
    ▼  [pak_unpack.py]
pipeline_out/unpacked/<Category>/sprite_N.bmp + manifest.json
    │
    ▼  [steps/colorkey_alpha.py]
pipeline_out/with_alpha/<Category>/sprite_N.png  (RGBA, color-key → alpha=0)
    │
    ▼  [copied to gigapixel_input/]
gigapixel_input/<Category>/sprite_N.png  (325 game categories)
    │
    ▼  [Topaz Gigapixel 2x upscale via gigapixel_auto.py]
gigapixel_output/<Category>/sprite_N.png  (2x resolution)
    │
    ▼  [defringe_v4.py — edge cleanup, no pixel deletion]
    ▼  [repack_all.py — postprocess + repack]
pipeline_out/repacked/<Category>.pak
    │
    ▼  [deploy: backup original as .pak.bak, copy new .pak]
Client/SPRITES/<Category>.pak      (336 repacked 2x PAKs)
Client/SPRITES/<Category>.pak.bak  (336 original backups)
```

### Input/Output Folder Paths

| Folder | Purpose |
|--------|---------|
| `gigapixel_input/` | 325 category folders of RGBA PNGs ready for Gigapixel |
| `gigapixel_output/` | 327 entries (325 folders + 2 stray .pak files) — 2x upscaled PNGs |
| `gigapixel_staging/` | Temp staging for Gigapixel batches |
| `gigapixel_staging_out/` | Temp staging output |
| `asset_pipeline/pipeline_out/` | Full pipeline intermediate outputs |

---

## 5. Server Architecture

### Server Processes

| Server | Executable | Working Directory | Config Files | Port |
|--------|-----------|-------------------|-------------|------|
| Login | `Login\Login.exe` | `Login\` | `LServer.cfg` | 4000 |
| Towns | `Maps\Towns\HGserver.exe` | `Maps\Towns\` | `GServer.cfg`, `Settings.cfg` | 3002 |
| Neutrals | `Maps\Neutrals\HGserver.exe` | `Maps\Neutrals\` | `GServer.cfg`, `Settings.cfg` | 3008 |
| Middleland | `Maps\Middleland\HGserver.exe` | `Maps\Middleland\` | `GServer.cfg`, `Settings.cfg` | 3007 |
| Events | `Maps\Events\HGserver.exe` | `Maps\Events\` | `GServer.cfg`, `Settings.cfg` | 3001 |
| Client | `Client\Play_Me.exe` | `Client\` | `GM.cfg` | — |

### Complete Port Map

| Port | Protocol | Service | Direction |
|------|----------|---------|-----------|
| 4000 | TCP | Login Server (client login) | Clients → Login |
| 5656 | TCP | GateServer (HG↔Login) | Game servers → Login |
| 3001 | TCP | Events game server | Clients → Events |
| 3002 | TCP | Towns game server | Clients → Towns |
| 3007 | TCP | Middleland game server | Clients → Middleland |
| 3008 | TCP | Neutrals game server | Clients → Neutrals |
| 3306 | TCP | MySQL 8.0 | Login → MySQL |
| 4825 | TCP | Client updater | Client → Updater |

**Note:** Port 5656 (GateServer) is hardcoded in both `Sources/Login/LoginServer.cpp` and `Sources/HG/HG.cpp` — not configurable via .cfg files.

### Network Flow

1. Client connects to Login server on port **4000**
2. Login authenticates via MySQL on port **3306**
3. Login tells client which game server to connect to (port 3001/3002/3007/3008)
4. Game servers register with Login via GateServer port **5656**
5. All services bind to `172.16.0.1` (with port proxies from `10.0.0.168` → `172.16.0.1`)

### Database

- **Engine:** MySQL 8.0
- **Host:** `127.0.0.1:3306`
- **User:** `root` / `alpha123`
- **Database:** `helbreath`
- **Binary path:** `C:\Program Files\MySQL\MySQL Server 8.0\bin`
- **Only the Login server connects to MySQL directly.** Game servers communicate with Login via GateServer socket protocol.

### Server Manager Launch Logic

`server_manager.py` is a tkinter GUI that:
- Launches Login first, then 1-second delay, then all 4 game servers simultaneously
- Each process spawns with `subprocess.Popen` using `CREATE_NEW_CONSOLE` (own console window)
- Stop uses `taskkill /F /IM`
- Reads/writes `Settings.cfg` for all 4 game servers simultaneously
- Client settings go to `GM.cfg`
- Database backup via `mysqldump` with hardcoded credentials

### Maps Hosted Per Server

**Towns (port 3002)** — 42 maps:
- Elvine: `elvine`, `elvfarm`, `elvbrk11/12/21/22`, `elvined1`, `elvjail`, `elvwrhus`, `gldhall_2`, `gshop_2/2f`, `resurr2`, `wrhus_2/2f`, `wzdtwr_2`, `bsmith_2/2f`, `cath_2`, `cityhall_2`, `CmdHall_2`
- Aresden: `aresden`, `arefarm`, `aresdend1`, `arebrk11/12/21/22`, `wrhus_1`, `cityhall_1`, `arewrhus`, `resurr1`, `gshop_1/1f`, `arejail`, `cath_1`, `wzdtwr_1`, `wrhus_1f`, `bsmith_1/1f`, `gldhall_1`, `CmdHall_1`

**Neutrals (port 3008)** — 17 maps:
- `bisle`, `default`, `areuni`, `elvuni`
- Fight zones: `fightzone1`–`fightzone9`
- Hunt zones: `huntzone1`–`huntzone4`

**Middleland (port 3007)** — 10 maps:
- `middleland`, `2ndmiddle`, `middled1n`, `middled1x`
- Tower of Hell: `toh1`, `toh2`, `toh3`
- Deep dungeons: `dglv2`, `dglv3`, `dglv4`

**Events (port 3001)** — 9 maps (10 in MAPDATA):
- `inferniaA`, `inferniaB`, `maze`, `druncncity`, `procella`, `abaddon`
- Battleground: `BTField`, `GodH`, `HRampart`
- **Note:** `icebound` has map data files but is NOT listed in GServer.cfg (disabled)

---

## 6. Sprite/PAK Structure

### Deployed PAKs: `Client\SPRITES\`
- **336 `.pak` files** (active, repacked at 2x resolution)
- **336 `.pak.bak` files** (1x original backups, created by repack_all.py before deploying)
- **10 `(1).PAK` files** (Windows copy artifacts — duplicates)
- **Total: ~685 files**

### Original Backups: `Client\SPRITES(original)\`
- **336 `.pak` files** (pristine originals, separate from .pak.bak)

### Category Folders in gigapixel_input/: 325 categories

Categories break down into:
- **Player characters** (8): `Helb`, `Bm`, `Bw`, `Ym`, `Yw`, `Wm`, `Ww`, `Cla`
- **Male equipment** (~60): `MAxe1`–`MAxe6`, `MHelm1`–`MHelm4`, `MHauberk`, `MRobe1`, `MStaff1`–`3`, `MSw`–`MSw3`, `MLarmor`, `MLeggings`, etc.
- **Female equipment** (~55): `WAxe1`–`WAxe6`, `WHelm1`/`WHelm4`, `WHauberk`, `WRobe1`, `WBodice1`–`2`, `WChemiss`, `WSkirt`, etc.
- **Hero equipment**: `MHHelm1`–`MHHelm3`, `MHHauberk1`–`3`, `WHRobe1`, etc.
- **Monsters** (~40): `Orc`, `Zom`, `Troll`, `Demon`, `Barlog`, `Cyc`, `Ettin`, `Liche`, `Beholder`, `DarkElf`, `DarkKnight`, `Wyvern`, `Hellclaw`, `Stalker`, `Minotaurs`, `Babarian`, `Bunny`, `Cat`, etc.
- **Map tiles** (~50): `maptiles1`–`6`, `Tile223-225` through `Tile541-545`, `Objects1`–`7`, `Structures1`
- **Effects** (~20): `EFFECT`, `EFFECT2`, `EFFECT3`, `effect4`–`14`, `yseffect2`–`4`, `CruEffect1`
- **NPCs** (~15): `Guard`, `Gandlf`, `Howard`, `Kennedy`, `Gail`, `Perry`, `Tom`, `William`, `McGaffin`, `SHOPKPR`
- **Items**: `item-dynamic`, `item-equipM`, `item-equipW`, `item-ground`, `item-pack`
- **Misc**: `BG`, `Crop`, `Gate`, `TREES1`, `TreeShadows`, `frost`, `yspro`, etc.

### 11 UI Categories (Skipped from Upscaling)

These use original 1x PAKs because they contain hardcoded pixel coordinates:

```
interface        — Mouse cursor, status indicators, main HUD
interface2       — Extended UI: crafting, help windows, fonts
GameDialog       — Inventory, skills, magic, trading, crusade panels
New-Dialog       — Loading screens, main menu, quit dialog
LoginDialog      — Login, account creation, agreement screens
DialogText       — Dialog text rendering and button sprites
SPRFONTS         — Font/character sprites for text rendering
newmaps          — Minimap and guide map overlays
Telescope        — World map display (32 frames)
Telescope2       — Heldenian map display
exch             — Exchange/trading UI
```

### PAK Naming Convention

| Pattern | Examples | Contents |
|---------|----------|----------|
| Character base | `Helb.pak`, `Bm.pak`, `Ww.pak` | Player body/animation sprites |
| Male equip `M*` | `MAxe1.pak`, `MHelm2.pak`, `MHauberk.pak` | Male character with equipment |
| Female equip `W*` | `WAxe1.pak`, `WHelm1.pak`, `WBodice1.pak` | Female character with equipment |
| Hero equip `MH*/WH*` | `MHHelm1.pak`, `MHHauberk1.pak` | Hero-tier equipment sprites |
| Neutral helm `NM*/NW*` | `NMHelm1.pak`, `NWHelm4.pak` | Neutral faction helms |
| Monster names | `Orc.pak`, `Demon.pak`, `Wyvern.pak` | Monster animation sprites |
| Effects `EFFECT*` | `EFFECT.pak`, `effect4.pak` | Spell/skill visual effects |
| Map tiles | `maptiles1.pak`, `Tile382-387.pak` | Ground tile textures (number = tile ID range) |
| World objects | `Objects1.pak`, `TREES1.pak` | Static world objects, buildings, trees |
| Items | `item-pack.pak`, `item-ground.pak` | Item sprites (inventory, ground views) |
| NPCs | `Guard.pak`, `SHOPKPR.pak` | NPC character sprites |
| UI (protected) | `interface.pak`, `GameDialog.pak` | UI elements (not upscaled) |
| Mantles | `Mmantle01.pak`, `Wmantle06.pak` | Cape/mantle equipment |

---

## 7. Config Files

### Client: `Client\GM.cfg`

```
log-server-address       = 172.16.0.1
log-server-port          = 4000
game-server-mode         = LAN
logout-timer             = 1
walk-speed               = 70
run-speed                = 20
dash-speed               = 50
attack-speed-multiplier  = 100
display-mode             = windowed
resolution               = 2560x1440
```

Other client configs:
- `Client\config.ini` — hotkey bindings (customized, includes GM shortcuts)
- `Client\classicconfig.ini` — default hotkey layout
- `Client\equipsets.cfg` — 3 saved equipment loadout sets
- `Client\Updater.dat` — `IP=127.0.0.1, Port=4825, Start=client`
- `Client\CONTENTS\ItemName.cfg` — item display names

### Login Server: `Login\LServer.cfg`

```
login-server-port    = 4000
external-address     = 172.16.0.1
game-server-port     = 3008       (NOTE: only Neutrals port — likely legacy)
mysql-address        = 127.0.0.1
mysql-user           = root
mysql-password       = alpha123
mysql-server-port    = 3306
mysql-database       = helbreath
permitted-address    = 172.16.0.1
permitted-address    = 127.0.0.1
```

### Login Server Config Files: `Login\Config\`

| File | Contents |
|------|----------|
| `Item.cfg`, `Item2.cfg`, `Item3.cfg`, `Item4.cfg` | ~320 items total, tab-delimited, 25+ fields |
| `Npc.cfg` | 142 NPCs/creatures, tab-delimited, 25 fields |
| `Magic.cfg` | Magic/spell definitions |
| `Skill.cfg` | Skill definitions |
| `Quest.cfg` | Quest definitions |
| `Potion.cfg` | Potion definitions |
| `BuildItem.cfg` | Crafting recipes |
| `CraftItem.cfg` | Craft item recipes |
| `DupItemID.cfg` | Duplicate item ID mapping |
| `Teleport.cfg` | Teleport definitions |
| `Items/Armor.cfg` | Armor items (sub-category) |
| `Items/Weapons.cfg` | Weapon items |
| `Items/Bows.cfg` | Bow items |
| `Items/Rings.cfg` | Ring items |
| `Items/Necklaces.cfg` | Necklace items |
| `Items/Scrolls.cfg` | Scroll items |
| `Items/Potions.cfg` | Potion items |
| `Items/Wands.cfg` | Wand items |
| `Items/Manuals.cfg` | Manual items |
| `Items/Minerals.cfg` | Mineral items |
| `Items/Food.cfg` | Food items |
| `Items/Dyes.cfg` | Dye items |
| `Items/Farming.cfg` | Farming items |
| `Items/Parts.cfg` | Part items |
| `Items/Misc.cfg` | Miscellaneous items |

### Game Server Configs (per server)

Each server subfolder has:
- **`GServer.cfg`** — server identity, map list, gate server address, port
- **`Settings.cfg`** — gameplay tuning (XP rates, drop rates, stat limits, etc.)

#### Settings.cfg Key Values (differences highlighted)

| Setting | Towns | Neutrals | Middleland | Events |
|---------|-------|----------|------------|--------|
| enemy-kill-adjust | **10** | **1** | **10** | **10** |
| secondary-drop-rate | 1 | **5** | 1 | 1 |
| slate-success-rate | 99 | 99 | 99 | 99 |
| character-stat-limit | 1000 | 1000 | 1000 | 1000 |
| character-skill-limit | 2500 | 2500 | 2500 | 2500 |
| max-player-level | 180 | 180 | 180 | 180 |
| exp-modifier | 1000 | 1000 | 1000 | 1000 |
| recall-damage-timer | 1 | 1 | 1 | 1 |

### Shared Configs: `Maps\configs\`

| File | Purpose |
|------|---------|
| `Crusade.cfg` | Crusade event structure positions |
| `Schedule.cfg` | Scheduled event timing |
| `droplist.xml` | Monster drop tables (XML, parsed with Xerces-C) |
| `droplist.xsd` | XML schema for drop tables |

---

## 8. Build Instructions

### Build All (Client + HG + Login)

```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\All In One.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

### Build Client Only

```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\Client\Client.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

Output: `Client\Play_Me.exe` (Release) or `Client\Client_d.exe` (Debug)

### Build HG Server Only

```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\HG\HGserver.sln' /p:Configuration=Debug /p:Platform=Win32 /m /v:minimal"
```

Output: `Maps\HGserver.exe` — then manually copy to `Maps\Towns\`, `Maps\Neutrals\`, `Maps\Middleland\`, `Maps\Events\`

### Build Login Server Only

```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\Login\Login.vcxproj' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

Output: `Login\Login.exe`

### Build Quirks

- **Always use `powershell.exe -Command`** — `cmd.exe /c` fails with spaces in paths
- **IDE:** Visual Studio 2022, MSBuild 17.14, Toolset v143, Win32 (x86)
- **SAFESEH:NO** required for old DirectX libs (DDRAW, DINPUT, DXGUID)
- **Link libs:** opengl32.lib, glew32.lib, glu32.lib (XAudio2 auto-links via SDK header)
- **PDB lock error (C1041)** on parallel builds: use `/t:Rebuild` to clear stale locks
- **HG server** outputs to `Maps\` root — must manually copy exe to 4 server subfolders after each build
- **Expect C4244 warnings** in Sprite.cpp — pre-existing, harmless

---

## 9. Root-Level Files

| File | Type | Purpose |
|------|------|---------|
| `CLAUDE.md` | docs | Project instructions for Claude Code |
| `PROJECT_MAP.md` | docs | This file |
| `PROJECT_SUMMARY.md` | docs | Project summary |
| `README.md` | docs | Project README |
| `server_manager.py` | script | Server manager GUI (Python/tkinter) |
| `clean_old.py` | script | Cleanup utility |
| `extract_new.py` | script | Extraction utility |
| `remap_skills.ps1` | script | Skill remapping (PowerShell) |
| `setup_firewall.bat` | script | Firewall rule setup |
| `START_ALL_SERVERS.bat` | script | Start all game servers |
| `STOP_ALL_SERVERS.bat` | script | Stop all game servers |
| `database_export.sql` | data | Database export/schema |
| `Helbreath Server Manager.exe` | binary | Server manager GUI (compiled PyInstaller) |
| `files.zip` | archive | Miscellaneous archive |

---

## 10. Data File Counts

| Directory | Count | Contents |
|-----------|------:|---------|
| `Client/SPRITES/` | 336 | Active sprite .pak files |
| `Client/SPRITES/` | 336 | Original .pak.bak backups |
| `Client/SPRITES(original)/` | 336 | Pristine original PAKs |
| `Client/SOUNDS/` | 228 | WAV sound files |
| `Client/MAPDATA/` | 96 | Client map data files |
| `Client/MUSIC/` | 11 | Music files |
| `Client/FONTS/` | 7 | Font files (.FNT) |
| `Client/shaders/` | 2 | GLSL shaders |
| `Maps/MAPDATA/` | 35 | Shared server map data |
| `Maps/Towns/MAPDATA/` | 84 | Towns map data (42 .amd + 42 .txt) |
| `Login/Config/` | 22+ | Server config files |
| `Login/Config/Items/` | 15 | Per-category item configs |
| `gigapixel_input/` | 325 | Category folders (PNGs for upscaling) |
| `gigapixel_output/` | 327 | Upscaled output (325 folders + 2 stray .pak) |
| `asset_pipeline/pipeline_out/ready/` | 336 | Processed sprite folders |
| `Sources/Shared/lib/` | 23 | Static libs + DLLs |
