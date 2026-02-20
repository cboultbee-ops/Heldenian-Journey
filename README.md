# HB

Helbreath private server files for Cam and Dallon.

## Architecture

- **Login Server** — `Login/Login.exe` (port 4000 for clients, port 5656 for game servers)
- **Game Servers** (4) — `Maps/<name>/HGserver.exe`
  - Towns (port 3002), Neutrals (port 3008), Middleland (port 3007), Events (port 3001)
- **Client** — `Client/Client_d.exe` (GPU/OpenGL renderer)
- **Database** — MySQL 8.0, database `helbreath`
- **Source Code** — `Sources/` (HG = game server, Login = login server, Client = client)

## Build

- Visual Studio 2022, MSBuild v143 toolset, Win32 (x86)
- Use PowerShell for MSBuild commands (bash mangles `/p:` switches)
- Build scripts: `build_all.ps1`, `build_login_only.ps1`

## Change Log

### 2026-02-19 — Client GPU Collision & Equipment Fixes

**Inventory drag bug (GPU mode)**
- Items in the inventory could not be clicked or dragged — the entire inventory window moved instead
- Root cause: `_bCheckCollison()` in `Sprite.cpp` requires a DirectDraw surface for pixel-level hit testing, but the GPU renderer (OpenGL/GLEW) never creates one, so `m_bIsSurfaceEmpty` stays TRUE and the function always returns FALSE
- Fix: Modified `_bCheckCollison()` in `Sprite.cpp` to handle GPU mode — lets the surface-empty guard pass when GPU is active, runs the bounding-box check using `m_stBrush` data (always available), and returns TRUE if the box passes (skipping the unavailable pixel check)
- This single fix covers all dialogs that use collision detection: inventory, character sheet (26 equipment slots), bank, trade, sell list

**Inventory-specific bounding box bypass (Game.cpp)**
- Added a GPU-mode fast path in `bDlgBoxPress_Inventory()` that skips the `_bCheckCollison` call when an outer bounding-box check already passed
- In DDraw mode, the original pixel-perfect collision is preserved

**Double-click unequip re-equip bug**
- Double-clicking an equipped item on the character sheet (F5) would unequip it, then immediately re-equip it
- Root cause: `DlbBoxDoubleClick_Character()` sent RELEASEITEM and set `m_bIsItemEquipped=FALSE`, but didn't clear the cursor selection state. The mouse-up handler then called `_bCheckDraggingItemRelease()` → `ItemEquipHandler()`, which saw the item was unequipped and re-equipped it
- Fix: Clear `m_stMCursor.cSelectedObjectType` and `sSelectedObjectID` after the unequip command, matching what the second code path already did

**Debug text removal**
- Removed `#ifdef _DEBUG` block that displayed "Press Inventory" text in the game window

**Build fix**
- Changed `#include "afxres.h"` to `#include <windows.h>` in `Sources/Client/Res/resource.rc` (MFC header not available; same fix previously applied to HG and Login builds)

### 2026-02-18 — Server Build & Network Setup

- Recovered missing source directories (char/, map/, net/, ui/, res/, droplist/, xsd-include/) from reference source
- Built HGserver.exe (Release, v143 toolset) and Login.exe (Release, v143 toolset)
- Fixed `afxres.h` → `windows.h` in HG and Login resource files
- Disabled SAFESEH for linker compatibility
- Configured network port proxies (netsh) for login and game server ports
- Set `max-player-level=180`, `exp-modifier=1000`, `character-skill-limit=2500`
- Raised `upper-level-limit` on default map to 180

## Known Issues

- **Windows Defender** blocks copying Login.exe to the `Login/` directory — needs an exclusion
- **`m_iPlayerMaxLevel` uninitialized** (HG.h:917) — causes log spam. Fix: set `= m_sMaxPlayerLevel` after HG.cpp:4344
- **Exp table overflow** at level 891+ (int32 overflow, not critical at max level 180)
- **Client source** uses VC9 project format, converted to VS2022 v143 for Debug builds
- **Item auto-placement** — new items of the same type stack at the same coordinates rather than spreading out
