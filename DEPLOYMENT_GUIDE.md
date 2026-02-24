# Helbreath Private Server — Deployment & Troubleshooting Guide

This guide documents the complete deployment process, known pitfalls, and solutions discovered during development. Written for AI assistants and developers working on this codebase.

---

## Table of Contents
1. [Build System](#build-system)
2. [Deployment Layout](#deployment-layout)
3. [Database Setup](#database-setup)
4. [Server Configuration](#server-configuration)
5. [Network Configuration](#network-configuration)
6. [Critical Build Gotcha — Stale Object Files](#critical-build-gotcha--stale-object-files)
7. [Common Bugs & Solutions](#common-bugs--solutions)
8. [Combat & Abilities System](#combat--abilities-system)
9. [Client Rendering (GPU + DirectDraw)](#client-rendering-gpu--directdraw)
10. [Sound System](#sound-system)
11. [Asset Pipeline](#asset-pipeline)
12. [GM Commands & Testing](#gm-commands--testing)

---

## Build System

### Prerequisites
- **Visual Studio 2022** (Community edition works)
- **MSBuild 17.x**, Toolset **v143**, Platform **Win32 (x86)**
- **MySQL 8.0** — database `helbreath`, user `root`/`alpha123`

### Build Commands

**IMPORTANT**: Always use `powershell.exe -Command` — bash and `cmd.exe /c` mangle paths with spaces.

All three projects (Client + HG + Login):
```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\All In One.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

Client only:
```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\Client\Client.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

HG (game server) only — **MUST use Debug config** (Release outputs to wrong directory):
```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\HG\HGserver.sln' /p:Configuration=Debug /p:Platform=Win32 /m /v:minimal"
```

Login server only:
```powershell
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\Login\Login.vcxproj' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

### Build Outputs
| Project | Config | Output Path |
|---------|--------|-------------|
| Client | Release | `Client\Play_Me.exe` |
| Client | Debug | `Client\Client_d.exe` |
| HG Server | Debug | `Maps\HGserver.exe` |
| Login | Release | `Login\Login.exe` |

### Build Notes
- **SAFESEH:NO** is required for legacy DirectX libs (DDRAW, DINPUT, DXGUID)
- Client links: `opengl32.lib`, `glew32.lib`, `glu32.lib` (XAudio2 auto-links via SDK header)
- Expect C4244 warnings in `Sprite.cpp` — pre-existing, harmless
- PDB lock error (C1041): use `/t:Rebuild` to clear stale locks

---

## Deployment Layout

```
C:\Helbreath Project\
├── Client\                     # Client runtime directory
│   ├── Play_Me.exe             # Client executable
│   ├── GM.cfg                  # Client configuration
│   ├── SPRITES\                # Active sprite PAK files
│   ├── SPRITES(original)\      # Pristine 1x PAK backup
│   └── SAVE\                   # Screenshots (BMP)
├── Login\                      # Login server
│   ├── Login.exe
│   └── LServer.cfg
├── Maps\                       # Game servers (4 instances)
│   ├── HGserver.exe            # Master copy (build output)
│   ├── Towns\HGserver.exe      # Copy for Towns map
│   ├── Neutrals\HGserver.exe   # Copy for Neutrals map
│   ├── Middleland\HGserver.exe # Copy for Middleland map
│   └── Events\HGserver.exe     # Copy for Events map
├── Sources\                    # All source code
│   ├── All In One.sln          # Master solution
│   ├── Client\                 # Client source + vcxproj
│   ├── HG\                     # Game server source + vcxproj
│   ├── Login\                  # Login server source + vcxproj
│   └── Shared\                 # Shared headers (NetMessages.h, etc.)
└── server_manager.py           # Python server launcher GUI
```

### After Building HG Server — Deploy to All 4 Directories
```bash
cp Maps/HGserver.exe Maps/Towns/HGserver.exe
cp Maps/HGserver.exe Maps/Neutrals/HGserver.exe
cp Maps/HGserver.exe Maps/Middleland/HGserver.exe
cp Maps/HGserver.exe Maps/Events/HGserver.exe
```

### Startup Order
1. Start **Login server** (`Login\Login.exe`) first
2. Start all 4 **HG servers** (Towns, Neutrals, Middleland, Events)
3. Start **Client** (`Client\Play_Me.exe`)

The `server_manager.py` (or its compiled `Helbreath Server Manager.exe`) automates this.

---

## Database Setup

### MySQL 8.0
- Database: `helbreath`
- User: `root` / Password: `alpha123`
- Key tables: `char_database`, `skill`, `item_database`

### Character Stats Columns (`char_database`)
| Column | Type | Maps To |
|--------|------|---------|
| `Strenght` (sic) | tinyint unsigned | STR |
| `Vitality` | tinyint unsigned | VIT |
| `Dexterity` | tinyint unsigned | DEX |
| `Intelligence` | tinyint unsigned | INT |
| `Magic` | tinyint unsigned | MAG |
| `Agility` | tinyint unsigned | CHR |

Note: "Strenght" is misspelled in the DB schema — this is intentional/legacy. Do NOT rename it.

### Skill IDs (`skill` table)
| SkillID | Skill Name |
|---------|------------|
| 0 | Mining |
| 1 | Manufacturing |
| 2 | Alchemy |
| 3 | **Magic** |
| 4 | Short Sword |
| 5 | Long Sword |
| 6 | Fencing |
| 7 | Axe |
| 8 | Shield |
| 9 | Archery |

### Stat Budget Formula
Total stat points = `(Level - 1) * 3 + 70` (including CHR).

### Useful Queries
```sql
-- Check character stats
SELECT char_name, Level, Strenght, Vitality, Dexterity, Intelligence, Magic, Agility,
       Strenght+Vitality+Dexterity+Intelligence+Magic+Agility AS Total
FROM char_database WHERE char_name = 'CharName';

-- Check skills (join needed because skill table uses CharID, not char_name)
SELECT cd.char_name, s.SkillID, s.SkillMastery
FROM skill s JOIN char_database cd ON s.CharID = cd.CharID
WHERE cd.char_name = 'CharName' ORDER BY s.SkillID;

-- Set a skill to 100%
UPDATE skill s JOIN char_database cd ON s.CharID = cd.CharID
SET s.SkillMastery = 100
WHERE cd.char_name = 'CharName' AND s.SkillID = 3;
```

---

## Server Configuration

### Client Config (`Client\GM.cfg`)
```ini
[CONFIG]
log-server-address  = 172.16.0.1
log-server-port     = 4000
game-server-mode    = LAN
walk-speed          = 70
run-speed           = 20
attack-speed-multiplier = 100
display-mode        = fullscreen
resolution          = 2560x1440
```

### Login Server Config (`Login\LServer.cfg`)
- Login port: 4000
- GateServer port: 5656

### Game Server Ports
| Server | Port |
|--------|------|
| Towns | 3002 |
| Neutrals | 3008 |
| Middleland | 3007 |
| Events | 3001 |

---

## Network Configuration

For LAN play, port proxies may be needed:
```
10.0.0.168 → 172.16.0.1
```
The client connects to the login server at the address in `GM.cfg`.

---

## Critical Build Gotcha — Stale Object Files

### The Problem
**If you modify `Client.h` (the CClient class in `Sources/HG/char/Client.h`) and only do an incremental build, `Client.cpp` may not be recompiled.** This causes a **class layout mismatch** between translation units:

- `HG.cpp` (recompiled) sees the new class layout with inline getters (`GetStr()`, `GetInt()`, etc.)
- `Client.obj` (stale) has setters (`SetStr()`, `SetInt()`) compiled against the OLD layout
- **Result**: Setters write to one memory offset, getters read from a different offset
- **Symptom**: Character stats appear "rotated" — STR shows MAG's value, INT shows STR's value, etc.

### The Fix
**Always use `/t:Rebuild` when modifying any header file in the HG project:**
```powershell
powershell.exe -Command "& '...\MSBuild.exe' 'Sources\HG\HGserver.sln' /p:Configuration=Debug /p:Platform=Win32 /t:Rebuild /m /v:minimal"
```

### How to Diagnose
If stats appear rotated in-game but the database has correct values, this is almost certainly a stale object file. The telltale sign: `GetStr()` returns a value different from `GetBaseStr() + GetAngelStr()`, which is mathematically impossible in correctly compiled code.

### Root Cause Details
`CClient` in `Client.h` has private members at the END of the class:
```cpp
private:
    int _str, _int, _dex, _mag;
    int _angelStr, _angelInt, _angelDex, _angelMag;
```
Any member additions ABOVE these (even in the public section) shift the memory offsets of `_str`, `_int`, etc. Since the getters are inline (defined in the header), they get compiled into each `.cpp` that includes the header. But the setters are defined in `Client.cpp` and compiled once. If `Client.cpp` isn't recompiled, its setters use the old offsets.

---

## Common Bugs & Solutions

### 1. Stats Display Rotated In-Game
**Cause**: Stale object files (see section above).
**Fix**: Clean rebuild with `/t:Rebuild`.

### 2. Short Overflow on Super Berserk + Super Attack
**Location**: `Sources/HG/char/combat.cpp` lines ~878-879
**Cause**: `NpcKilledHandler`/`KilledHandler` take `short sDamage` but receive `int iDamage`. The 2.5x super berserk multiplier on super attacks overflows `short` (max 32767).
**Fix**: Clamp `iAP_SM` and `iAP_L` to 32000 after berserk multiplier application.

### 3. Speed Drop After Glacial Strike Expires
**Location**: `Sources/Client/MessageHandler.cpp` line ~4918
**Cause**: Server sends `NOTIFY_MAGICEFFECTOFF` (not `NOTIFY_SPEED_BUFF`) when speed expires. Client had no handler for `MAGICTYPE_SPEED` in that case.
**Fix**: Added `MAGICTYPE_SPEED` case in `NotifyMsg_MagicEffectOff` that calls `ApplySpeedBuff(false)`.

### 4. Skill Display Showing 0%
**Location**: `Sources/Client/MessageHandler.cpp` line ~3122
**Cause**: `InitSkillList` loaded `m_cSkillMastery` but never set `m_pSkillCfgList[i]->m_iLevel`, which the skill dialog reads.
**Fix**: Added `m_iLevel` update in the init loop.

### 5. `PlaySound` Name Conflict
**Cause**: Windows `#define PlaySound PlaySoundA` silently renames any method called `PlaySound`.
**Fix**: The sound method is named `PlaySfx()` instead. Never name a method `PlaySound`.

### 6. `DEX > 200` or Stats Exceeding Limits
**Cause**: If stats get rotated due to stale builds, level-up allocations go to the wrong stat. A stat might exceed the 200 cap because the server checks the REAL stat (correct) but the client displays a different one.
**Fix**: Fix the stale build, then correct the database values directly.

### 7. EXP Bar Not Showing
**Location**: `Sources/Client/Game.cpp` line ~17674
**Cause**: The EXP bar rendering code was commented out.
**Fix**: Uncommented. Also added EXP percentage tooltip and handled the 100% case with escaped `%%`.

---

## Combat & Abilities System

### Special Abilities (4 Diamond Skills)
| Ability | Cooldown | Effect |
|---------|----------|--------|
| Berserk | 5 min | Damage multiplier |
| Super Berserk | 10 min | 2.5x damage, purple character hue |
| Speed Burst | 10 min | Walk/run frame times reduced to 2/3 |
| Glacial Strike | 15 min | Ice-based attack |

### Speed Buff System
- `ApplySpeedBuff(true)`: walk/run frame times reduced to 2/3
- `ApplySpeedBuff(false)`: restores to `m_iWalkSpeed`/`m_iRunSpeed` from GM.cfg
- Dual-layer expiry: client-side timer + server `NOTIFY_MAGICEFFECTOFF` handler

### Spell Targeting
- **Corridor spells** (Blizzard, Meteor Strike, Earth Shockwave): draw targeting corridor guide after cast animation
- Targeting activates at the `OBJECTMAGIC` case handler (line ~29231), NOT at `UseMagic()` (line ~36368 is intentionally commented out)

### Server-Side Ability Handler
`NewAbilityActivationHandler` in `HG.cpp` handles activation with per-ability cooldowns.

---

## Client Rendering (GPU + DirectDraw)

### Dual Renderer Architecture
- `DXC_ddraw` manages both DirectDraw 7 and OpenGL 3.3 (`m_bUseGPU` flag)
- GPU renderer: `CGPURenderer` — OpenGL 3.3 context via WGL on Win32 HWND
- Sprite batching with up to 4096 sprites per batch
- Font atlas: 512x512 RGBA texture

### Resolution
- Virtual resolution: 640x480 (game logic)
- GPU scales to native resolution with aspect-ratio-preserving letterboxing
- `RenderConfig` struct tracks viewport offsets and uniform scale factor

### Screenshots
- `SaveScreenshot()` uses `glReadPixels(GL_FRONT)` to capture the OpenGL framebuffer
- Output: BMP files in `Client\SAVE\`

### Shutdown Order Matters
- `DXC_ddraw::~DXC_ddraw` calls `ShutdownGPURenderer()` which deletes the GPU renderer
- CSprite destructors run AFTER GPU renderer is gone — `m_pDDraw` becomes dangling
- `UnloadFromGPU` must NOT call `glDeleteTextures` — `wglDeleteContext` already frees all GL textures

---

## Sound System

- **CSoundManager** wraps XAudio2 (replaced DirectSound)
- `YWSound`: `IXAudio2` + `IXAudio2MasteringVoice`
- `CSoundBuffer`: `IXAudio2SourceVoice` with lazy WAV loading
- Sound containers: `std::unordered_map<int, CSoundBuffer*>`
- Volume conversion: DS decibels (-10000..0) → XAudio2 linear (0..1) via `powf(10, dB/2000)`
- Pan: `SetOutputMatrix()` with stereo L/R gains

---

## Asset Pipeline

### Sprite PAK Files
| Location | Contents |
|----------|----------|
| `Client\SPRITES(original)\` | 336 pristine original 1x PAKs |
| `Client\SPRITES\` | Currently deployed PAKs |

### Gigapixel Upscaling Workflow
```
gigapixel_input/ → Topaz Gigapixel 2x → gigapixel_output/ → defringe_v4.py → repack PAKs
```
- `gigapixel_input/` — original 1x PNGs (never modify)
- `gigapixel_output_backup/` — pristine Gigapixel output pre-defringe (never modify)

---

## GM Commands & Testing

### Available GM Commands
- `/setmag <value>` — Set character's MAG stat (only stat with a GM command)
- Other stats must be modified directly in the database

### GM Panel
- Input mode 13 for Amount field — click the amount number to type values up to 10M
- Useful for creating gold or items for testing

### Testing Characters
When setting up test characters, remember:
- Use the `skill` table (joined via `CharID`) to set skill masteries
- SkillID 3 = Magic, SkillID 4 = Short Sword (don't mix these up)
- Stats are `tinyint unsigned` (0-255) but game caps at 200 (`m_sCharStatLimit`)
- After modifying the database, the character must log out and back in to pick up changes

### Database Access (MySQL CLI)
```bash
"/c/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe" -u root -palpha123 helbreath
```
Note: `mysql` is not in PATH — use the full path.
