# Helbreath Server — Attack Speed Fix + GM Configuration

---

## Task 1: Fix Attack Speed (Too Fast)

### Problem
Attack speed is approximately 2x faster than standard Helbreath. Player is very familiar with the game and confirms this is not normal.

### Root Cause
In HG.cpp around line 8627, there is a `#ifdef WEAPONSPEEDLIMIT` block that enforces minimum weapon speeds per weapon type. This block is **not active** because `WEAPONSPEEDLIMIT` is not defined. The `#else` branch (line 8639-8641) allows weapon speed to go all the way down to 0:

```cpp
#else
    sSpeed -= (m_pClientList[iClientH]->GetStr() / 13);
    if (sSpeed < 0) sSpeed = 0;   // <-- No weapon-type minimum!
#endif
```

At high levels (130+) with high STR, the speed reduction makes every weapon swing at maximum speed (0), regardless of weapon type. A Battle Axe should have a minimum speed of 3, a Long Sword minimum 2, etc.

### Fix
**Option A (recommended):** Define `WEAPONSPEEDLIMIT` and uncomment the weapon speed minimums inside the `#ifdef` block. Find the header file or preprocessor definitions and add `#define WEAPONSPEEDLIMIT`. Then uncomment the switch statement inside the `#ifdef WEAPONSPEEDLIMIT` block at lines 8629-8638 so it enforces per-weapon minimums:

```cpp
switch (m_pClientList[iClientH]->m_pItemList[sItemIndex]->m_sRelatedSkill) {
    case SKILL_ARCHERY:     if (sSpeed < 1) sSpeed = 1; break;
    case SKILL_SHORTSWORD:  if (sSpeed < 0) sSpeed = 0; break;
    case SKILL_LONGSWORD:   if (sSpeed < 2) sSpeed = 2; break;
    case SKILL_FENCING:     if (sSpeed < 1) sSpeed = 1; break;
    case SKILL_AXE:         if (sSpeed < 3) sSpeed = 3; break;
    case SKILL_HAMMER:      if (sSpeed < 1) sSpeed = 1; break;
    default:                if (sSpeed < 0) sSpeed = 0; break;
}
```

Note: There are TWO copies of this block (around lines 8627 and 8682) — fix both. Also verify the SKILL_* constants match the new skill index mapping (SKILL_AXE=7, etc.).

**Option B (simpler):** Just change the minimum in the `#else` block from 0 to a reasonable value:
```cpp
if (sSpeed < 2) sSpeed = 2;  // Global minimum, prevents zero-speed attacks
```

### Also check
The `#ifndef NO_MSGSPEEDCHECK` blocks control server-side speed hack detection. Verify `NO_MSGSPEEDCHECK` is NOT defined — if it is, the server won't enforce attack or movement speed limits at all.

Rebuild the server and deploy to all 4 HGserver directories after changes.

---

## Task 2: Create Server Manager GUI

### Overview
Build a single Windows GUI application that replaces the separate startup/shutdown scripts and provides a control panel for all server settings. This should be a standalone `.exe` that Cameron runs instead of the batch files.

### Technology
Use Python with tkinter (ships with Python, no dependencies) and compile to .exe with PyInstaller. Or use C# WinForms if preferred. The GUI should be simple and functional — not fancy.

### Layout
The window should have these sections:

#### Server Control (top section)
- **Start All Servers** button — starts Login.exe, waits 3 seconds, then starts all 4 HGserver.exe instances. Show status indicators (green/red) for each process.
- **Stop All Servers** button — kills all server processes and closes their console windows (replaces the shutdown script).
- **Restart All** button — stop then start.
- **Status display** — show running/stopped status for: Login Server, Towns, Events, Middleland, Neutrals. Auto-refresh every few seconds.
- **Launch Client** button — starts Client_d.exe from the client directory.

#### Server Settings (middle section)
Read the current values from the server config files and display them as editable fields. Include ALL settings found in the config parser. At minimum:

| Setting | Config Key | Type | Range |
|---------|-----------|------|-------|
| EXP Modifier | `exp-modifier` | Number | 1-10000 |
| Max Player Level | `max-player-level` | Number | 1-800 |
| Primary Drop Rate | `primary-drop-rate` | Number | 1-10000 |
| Secondary Drop Rate | `secondary-drop-rate` | Number | 1-10000 |
| Skill Point Limit | `character-skill-limit` | Number | 1-5000 |
| Min Weapon Speed | `min-weapon-speed` | Number | 0-10 |

- **Save Settings** button — writes changes back to the config files.
- **Apply & Restart** button — saves settings and restarts all servers.
- Settings that are changed but not saved should be highlighted.

#### Quick Actions (bottom section)
- **Open Server Logs** — opens the log directory in File Explorer
- **Open Config Files** — opens the config directory in File Explorer
- **Database Backup** — runs mysqldump and saves to a timestamped .sql file

### File Paths (hardcoded or from a settings.ini)
```
Login.exe:     C:\Helbreath Project\Login\Login.exe
HGserver.exe:  C:\Helbreath Project\Maps\{Towns,Events,Middleland,Neutrals}\HGserver.exe
Client_d.exe:  C:\Helbreath Project\Client\Client_d.exe
Server Config: (find the config file that contains exp-modifier, etc.)
```

### Process Management
- Use `subprocess.Popen` (Python) or `Process.Start` (C#) to launch servers
- Track PIDs so shutdown can kill the correct processes
- Use `taskkill /F /PID` for reliable shutdown
- After killing server processes, also kill any orphaned cmd.exe windows associated with them

### Build
Compile to a standalone .exe so it can be run without Python installed:
```bash
pip install pyinstaller
pyinstaller --onefile --windowed --name "Helbreath Server Manager" server_manager.py
```
Place the compiled .exe in `C:\Helbreath Project\`

### Important
- The GUI should work without admin privileges if possible, but server processes may need elevation
- Don't block the GUI while servers are starting — use threading
- Save window position/size between sessions if possible
- The old startup and shutdown batch scripts can remain as backups but the GUI replaces them

---

## Task 3: Remove Debug Coordinate Display

In the CLIENT source Game.cpp, find and delete this line (around line 2806):
```cpp
{ char dbg[64]; wsprintf(dbg, "X:%d Y:%d", msX, msY); PutString2(300, 10, dbg, 255, 255, 0); }
```

Rebuild the client after removal.

---

## File Locations
- **Server source:** `C:\Helbreath Project\Sources\`
- **Server config:** Found in the Maps subdirectories (look for the config file that contains `exp-modifier`, `primary-drop-rate`, etc.)
- **Client source:** `C:\Helbreath Project\Sources\Client\` or similar
- **Deploy HGserver.exe to:** Towns, Events, Middleland, Neutrals directories under Maps
