# Helbreath Private Server — Complete Project Briefing for Claude

**Last updated:** 2026-02-20
**Owner:** Cameron (account: CamB, character: toke, Level 180, Traveler, map "default")

> This document gives you everything you need to work on this project. Read it fully before making changes.

---

## 1. Project Overview

This is a Helbreath private server running on Cameron's Windows 11 machine. It consists of:
- **Login Server** — authenticates clients, manages character data, routes to game servers
- **4 Game Servers** — Towns, Neutrals, Middleland, Events (each runs its own HGserver.exe)
- **Game Client** — the player-facing game executable
- **Server Manager** — Python/tkinter GUI tool for managing all of the above
- **MySQL Database** — stores accounts, characters, items, guilds

The source code is C++ (Visual Studio 2022, v143 toolset, Win32/x86). The client and server are separate VS projects.

---

## 2. Directory Layout

```
C:\Helbreath Project\                    ← Project root (git repo)
├── Client\                              ← Client runtime directory
│   ├── Client_d.exe                     ← Original client (4.2MB, debug)
│   ├── Play_Me.exe                      ← Client built from source (1.5MB, release)
│   └── GM.cfg                           ← Client config (server address, movement, logout)
│
├── Login\                               ← Login server runtime directory
│   ├── Login.exe                        ← Login server binary (~240KB, debug build)
│   └── LServer.cfg                      ← Login server config (ports, MySQL credentials)
│
├── Maps\                                ← Game server runtime directories
│   ├── HGserver.exe                     ← Master copy (Debug build output lands here)
│   ├── Towns\                           ← Port 3002
│   │   ├── HGserver.exe                 ← Copy of Maps\HGserver.exe
│   │   ├── GServer.cfg                  ← Network config (port, maps, login address)
│   │   └── Settings.cfg                 ← Gameplay settings (exp, drops, speeds, etc.)
│   ├── Neutrals\                        ← Port 3008 (same structure)
│   ├── Middleland\                      ← Port 3007 (same structure)
│   └── Events\                          ← Port 3001 (same structure)
│
├── Sources\                             ← All source code
│   ├── HG\                              ← Game server source
│   │   ├── HGserver.vcxproj             ← VS project (MUST BUILD DEBUG - see Build section)
│   │   ├── HG.cpp                       ← Main game logic (~41,000 lines)
│   │   ├── HG.h                         ← CGame class header (member variables, settings)
│   │   ├── char\combat.cpp              ← Combat/damage calculations
│   │   ├── ui\Wmain.cpp                 ← Server GUI, utility functions (bGetOffsetValue, etc.)
│   │   └── xsd-include\                 ← XML schema headers
│   │
│   ├── Login\                           ← Login server source
│   │   ├── Login.vcxproj                ← VS project (created from Login.vcxproj.xml)
│   │   ├── LoginServer.cpp              ← Main login logic (~3000 lines)
│   │   ├── LoginServer.h                ← Login server header
│   │   ├── main.cpp                     ← Entry point, window proc
│   │   └── res\Resources.rc             ← Resource file (uses windows.h, NOT afxres.h)
│   │
│   ├── Client\                          ← Client source
│   │   ├── Client.vcxproj               ← VS project (Release builds to Client\Play_Me.exe)
│   │   ├── Game.cpp                     ← Main client logic (~40,000 lines)
│   │   ├── Game.h                       ← Client game class header
│   │   ├── GlobalDef.h                  ← Defines (_DEBUG is commented out)
│   │   ├── UI\Wmain.cpp                 ← Client window proc, WM_CLOSE handler
│   │   └── lan_eng.h                    ← English language strings
│   │
│   └── Shared\lib\                      ← Static libraries (.lib files)
│       ├── xerces-c_3D.lib              ← XML parser (Debug)
│       ├── xerces-c_3.lib               ← XML parser (Release)
│       ├── libmysql.lib                 ← MySQL client
│       └── (DirectX, GameServerStrTok, etc.)
│
├── server_manager.py                    ← Server Manager GUI (tkinter)
├── MAGIC_FIX.md                         ← Documents the spell mastery encoding bug
├── CODE_BOT_TASKS.md                    ← Task list for Claude code bot
├── CLAUDE_BRIEFING.md                   ← THIS FILE
└── _ref_source\                         ← Reference: original Helbreath International source
```

---

## 3. Build System — CRITICAL INFORMATION

### HGserver (Game Server)
| Setting | Value |
|---------|-------|
| **Project** | `Sources\HG\HGserver.vcxproj` |
| **Configuration** | **DEBUG** (MUST be Debug, NOT Release) |
| **Platform** | Win32 (x86) |
| **Toolset** | v143 (VS2022) |
| **Debug OutputDir** | `..\..\Maps` → `C:\Helbreath Project\Maps\HGserver.exe` |
| **Release OutputDir** | `..\..\..\Helbreath-International-Server\...` (WRONG — stale path, DO NOT USE) |
| **Expected size** | ~2.9MB (Debug). If you get ~1.4MB, you built Release — it will CRASH |

**Build command (PowerShell ONLY — bash mangles /p: switches):**
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'C:\Helbreath Project\Sources\HG\HGserver.vcxproj' /p:Configuration=Debug /p:Platform=Win32 /verbosity:minimal
```

**After building:** Copy `Maps\HGserver.exe` to all 4 subdirectories:
```
copy Maps\HGserver.exe Maps\Towns\HGserver.exe
copy Maps\HGserver.exe Maps\Neutrals\HGserver.exe
copy Maps\HGserver.exe Maps\Middleland\HGserver.exe
copy Maps\HGserver.exe Maps\Events\HGserver.exe
```

### Login Server
| Setting | Value |
|---------|-------|
| **Project** | `Sources\Login\Login.vcxproj` (copied from Login.vcxproj.xml) |
| **Configuration** | Debug |
| **Platform** | Win32 |
| **OutputDir** | `..\..\Login\` → `C:\Helbreath Project\Login\Login.exe` |
| **Expected size** | ~240KB |
| **Note** | `res\Resources.rc` must use `#include <windows.h>` not `afxres.h` |

### Client
| Setting | Value |
|---------|-------|
| **Project** | `Sources\Client\Client.vcxproj` |
| **Configuration** | Release (or Debug) |
| **Platform** | Win32 |
| **OutputDir** | `..\..\Client` → `C:\Helbreath Project\Client\Play_Me.exe` |
| **Note** | The original client is `Client_d.exe` (4.2MB). Rebuilt client is `Play_Me.exe` (1.5MB) |

### Build gotchas
- **ALWAYS use PowerShell** for MSBuild. Bash/sh mangles `/p:Configuration=Debug` into a path.
- **Windows Defender** may block exe writes. Add exclusions if needed.
- **SAFESEH is disabled** in all projects (required for legacy code).
- The `_DEBUG` preprocessor define in `GlobalDef.h` is commented out — the client runs in release-like mode even in Debug config.

---

## 4. Network Configuration

```
Client (10.0.0.168) → port proxy → Login (172.16.0.1:4000)
                                         ↓
                               Game Servers (172.16.0.1)
                               ├── Towns     :3002
                               ├── Neutrals  :3008
                               ├── Middleland:3007
                               └── Events    :3001
```

Port proxies (netsh) forward from 10.0.0.168 → 172.16.0.1 on all server ports.

---

## 5. Database

| Setting | Value |
|---------|-------|
| **Engine** | MySQL 8.0 |
| **Host** | 127.0.0.1:3306 |
| **Database** | `helbreath` |
| **User/Pass** | `root` / `alpha123` |
| **MySQL bin** | `C:\Program Files\MySQL\MySQL Server 8.0\bin\` |

### Key tables
| Table | Purpose |
|-------|---------|
| `account_database` | Login accounts |
| `char_database` | Character data (stats, mastery, location, etc.) |
| `item` | Character inventory items |
| `bank_item` | Bank storage items |
| `skill` | Character skill data |
| `guild` / `guild_member` | Guild system |

### Character data schema (char_database)
The `char_database` table stores character data in individual columns. Key fields:
- **MagicMastery** — `varchar(100)` of `'0'` and `'1'` characters (one per spell type, 100 total)
- Stats: Strenght, Vitality, Dexterity, Intelligence, Magic, Agility (tinyint unsigned)
- **AdminLevel** — 0=normal, higher=GM powers

---

## 6. Known Bug: Magic Mastery Encoding (THE SPELL BUG)

**This is the #1 recurring issue.** It causes ALL spells to fail.

### Root cause
The `MagicMastery` column in `char_database` is a `varchar(100)` storing `'1'` and `'0'` characters.
- `'1'` = ASCII value **49** (0x31)
- `'0'` = ASCII value **48** (0x30)

The **LoginServer** (`LoginServer.cpp:2364`) loads this string and copies it into a binary data buffer. The **game server** (`HG.cpp:12608`) then checks:
```cpp
if (caster->m_cMagicMastery[sType] != 1)  // expects integer 1, gets ASCII 49
```

### Fix applied (2026-02-20)
**LoginServer load** (`LoginServer.cpp:2364`) — converts ASCII to binary on load:
```cpp
else if(IsSame(field[f]->name, "MagicMastery")) {
    int mlen = strlen(myRow[f]);
    if (mlen > 100) mlen = 100;
    for (int m = 0; m < mlen; m++)
        Data[243 + m] = (myRow[f][m] == '1') ? 1 : 0;
}
```

**LoginServer save** (`LoginServer.cpp:2839`) — converts binary back to ASCII for varchar storage:
```cpp
for (int m = 0; m < 100; m++)
    MagicMastery[m] = ((BYTE)cp[243 + m] == 1) ? '1' : '0';
MagicMastery[100] = '\0';
```

### If spells break again
1. Check the LoginServer code at line ~2364 — is the ASCII→binary conversion present?
2. Check the game server code at HG.cpp line ~12608 — what value is mastery showing in the debug log?
3. Check the database directly: `SELECT char_name, MagicMastery FROM char_database WHERE char_name='toke';` — should show '1'/'0' characters
4. Make sure the REBUILT Login.exe is actually deployed to `Login\Login.exe`

---

## 7. Gameplay Settings

### Settings.cfg (per game server — all 4 should match unless intentional)
| Setting | Value | Notes |
|---------|-------|-------|
| exp-modifier | 1000 | 1000x experience |
| max-player-level | 180 | Level cap |
| character-skill-limit | 2500 | Max skill points |
| character-stat-limit | 1000 | Max stat points |
| slate-success-rate | 99 | Near-guaranteed slate success |
| primary-drop-rate | 1 | Normal drops |
| secondary-drop-rate | 1 (5 on Neutrals) | Neutrals has higher |
| enemy-kill-adjust | 10 (1 on Neutrals) | Kill difficulty |
| super-attack-multiplier | 100 | 100% = normal |
| recall-damage-timer | 1 | 1=enabled, 0=instant recall (new) |
| min-speed-axe/longsword/etc. | 0-3 | Weapon attack speed minimums |
| attack-frequency-min | 20 | Anti-speedhack threshold |

### GM.cfg (client)
| Setting | Value | Notes |
|---------|-------|-------|
| log-server-address | 172.16.0.1 | Login server IP |
| log-server-port | 4000 | Login server port |
| game-server-mode | LAN | LAN mode for local testing |
| logout-timer | 1 | 1 = near-instant logout |
| walk-speed | 70 | ms per frame |
| run-speed | 20 | ms per frame |
| dash-speed | 50 | ms per frame |

---

## 8. Changes Made This Session (2026-02-20)

### A. Server Manager GUI — Scrollable Layout (server_manager.py)
- Added `ScrollableFrame` helper class for vertical scrolling
- Refactored Gameplay Tuning tab: buttons pinned at top, settings in scrollable area
- Added "Dev Toggles" section with Recall Damage Timer and Instant Logout toggles
- Toggle buttons read/write Settings.cfg (recall) and GM.cfg (logout)

### B. Recall Damage Timer — Server Side (HG.cpp, HG.h)
- **HG.h line 940**: Added `int m_bRecallDamageTimer;` member variable
- **HG.cpp line 178**: Initialized `m_bRecallDamageTimer = 1;` (enabled by default)
- **HG.cpp line ~4435**: Added case 26 config parser for `recall-damage-timer` setting
- **HG.cpp line ~4466**: Added token match `recall-damage-timer` → cReadMode = 26
- **HG.cpp line ~13301**: Modified recall check to bypass timer when disabled:
  ```cpp
  if(!m_bRecallDamageTimer || (dwTime - caster->m_lastDamageTime) > 10000 || caster->IsGM())
  ```
- **All 4 Settings.cfg**: Added `recall-damage-timer = 1`

### C. Logout Timer Fix (Client: UI/Wmain.cpp)
- **Wmain.cpp line ~74**: Fixed WM_CLOSE handler to use `m_iLogOutTimer` from config instead of hardcoded 11 seconds. Was:
  ```cpp
  #ifdef _DEBUG
      G_pGame->m_cLogOutCount = 1;
  #else
      G_pGame->m_cLogOutCount = 11;
  #endif
  ```
  Now uses: `G_pGame->m_cLogOutCount = (char)G_pGame->m_iLogOutTimer;`

### D. Bag Inventory Display (Client: Game.cpp)
- Added item count and weight display at bottom of inventory dialog
- Uses `PutString2` (not SprFont3) in yellow text
- Item count: loops through `m_pItemList[0..MAXITEMS-1]`, counts non-null
- Weight formula: current = `_iCalcTotalWeight()/100`, max = `STR*5 + Level*5`
- Inserted at end of `DrawDialogBox_Inventory()`, after the existing button regions

### E. Magic Mastery Encoding Fix (LoginServer.cpp)
- **Load (line ~2364)**: Converts varchar '1'/'0' to binary 1/0 when packing character data
- **Save (line ~2839)**: Converts binary 1/0 back to varchar '1'/'0' for database storage
- This fixes the root cause of "all spells fail" — the game server expects integer 1 for learned spells, but was receiving ASCII 49

### F. Login Server Build Fix
- Copied `Login.vcxproj.xml` → `Login.vcxproj` (the .vcxproj file was missing)
- Fixed `res\Resources.rc`: replaced `#include "afxres.h"` with `#include <windows.h>`

---

## 9. Source Code Key Locations

### Game Server (HG.cpp) — ~41,000 lines
| What | Location |
|------|----------|
| Constructor / variable init | Line ~160-185 |
| Settings.cfg parser (`bReadSettingsConfigFile`) | Lines ~4380-4475 |
| Character data load from LoginServer | Lines ~4770-4785 |
| Character data save to LoginServer | Lines ~5155-5165 |
| Magic mastery check (spell gate) | Line ~12608 |
| Recall spell logic (MAGICTYPE_TELEPORT) | Lines ~13295-13318 |
| Recall damage timer check | Line ~13301 |
| Force recall / map level limits | Lines ~2670-2742 |
| "Above max level" check | Line ~17366-17369 |
| Max level config parse | Lines ~4340-4344 |

### Game Server Header (HG.h)
| What | Location |
|------|----------|
| MAXMAGICTYPE, MAXITEMS, etc. | Near top |
| Settings member variables | Lines ~935-945 (m_iAttackFreqMin, m_iSuperAttackMultiplier, m_bRecallDamageTimer, etc.) |
| m_sMaxPlayerLevel | Line ~941 |
| m_iPlayerMaxLevel (UNINITIALIZED — causes log spam) | Line ~917 |

### Login Server (LoginServer.cpp)
| What | Location |
|------|----------|
| Character data load from MySQL | Lines ~2300-2370 |
| MagicMastery load (ASCII→binary fix) | Line ~2364 |
| Character data save to MySQL | Lines ~2715-2960 |
| MagicMastery save (binary→ASCII fix) | Line ~2839 |
| Enter game handler | Lines ~2005-2086 |

### Client (Game.cpp) — ~40,000 lines
| What | Location |
|------|----------|
| Variable initialization | Lines ~260-470 |
| GM.cfg parser (logout-timer, speeds) | Lines ~13735-13783 |
| Inventory drawing (DrawDialogBox_Inventory) | Line ~18856 |
| Item count + weight display (NEW) | After line ~18916 |
| _iCalcTotalWeight() | Line ~20377 |
| Logout countdown logic | Lines ~30494-30503 |
| F12 logout trigger | Line ~24793 |
| Weight display formula reference | Line ~32446 |

### Client Header (Game.h)
| What | Location |
|------|----------|
| MAXITEMS = 50 | Line 84 |
| STAT_STR = 0 | Line 141 |
| m_pItemList[MAXITEMS] | Line 744 |
| m_iLevel, m_stat[6] | Lines 850-851 |
| m_cLogOutCount, m_iLogOutTimer | Lines 929-930 |

---

## 10. Previous Session Fixes (Day 1 commit: 22cebd4)

The Day 1 commit fixed many issues. Key changes:
- Damage calculations in `combat.cpp`
- Spell system debugging (added mastery debug logging)
- Skill system fixes
- Super attack multiplier
- Dash/movement speed configuration
- HP bar display
- Attack speed configuration
- Server manager GUI creation
- Database export

---

## 11. Known Remaining Issues

1. **m_iPlayerMaxLevel uninitialized** (HG.h:917) → causes log spam. Fix: set `= m_sMaxPlayerLevel` after HG.cpp:4344
2. **Pixel/graphics issues in client** — not yet investigated
3. **Exp table overflows** at level 891+ (int32), not critical at max 180
4. **Settings.cfg slight differences** — Neutrals has different enemy-kill-adjust (1) and secondary-drop-rate (5) vs the other 3 servers (10 and 1)
5. **Client uses `Client_d.exe`** (original) — the rebuilt `Play_Me.exe` may need testing as a replacement

---

## 12. How to Test

1. Start Login Server: run `Login\Login.exe`
2. Wait 3 seconds
3. Start all 4 game servers: run `HGserver.exe` in each Maps subdirectory
4. Start client: run `Client\Client_d.exe` (or `Play_Me.exe`)
5. Login: account **CamB**, password **password**
6. Select character **toke** (Level 180)
7. Enter game world — should load without "Connection Lost"
8. Test spells: cast Berserk or Recall — should work
9. Test inventory: open bag, check for Items/Weight display at bottom

Or use the Server Manager: `python server_manager.py` → click "Start All"

---

## 13. Git Info

- **Branch:** master
- **Remote:** `cboultbee-ops/Helbreath-Project`
- **Last commit:** `22cebd4` — Day 1 fixes
- **Uncommitted changes:** All the changes from section 8 above
