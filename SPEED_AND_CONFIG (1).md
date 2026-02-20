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
Build a single Windows GUI application that replaces the separate startup/shutdown scripts and provides a tabbed control panel for all server settings and gameplay tuning. This should be a standalone `.exe` that Cameron runs instead of the batch files. It must be designed to be **expandable** — new settings and tabs can be added easily over time.

### Technology
Use Python with tkinter (ships with Python, no dependencies) and compile to .exe with PyInstaller. Or use C# WinForms if preferred. The GUI should be clean and functional.

### Layout — Tabbed Interface

#### Top Bar (always visible, above tabs)
- **Start All Servers** button (green)
- **Stop All Servers** button (red)
- **Restart All** button
- **Launch Client** button
- **Status indicators** — green/red dots for: Login Server, Towns, Events, Middleland, Neutrals. Auto-refresh every 3 seconds.

---

#### Tab 1: Server Settings
Core server configuration. Read current values from the server config files and display as editable fields.

| Setting | Config Key | Type | Range | Description |
|---------|-----------|------|-------|-------------|
| EXP Modifier | `exp-modifier` | Number | 1-10000 | Experience rate multiplier |
| Max Player Level | `max-player-level` | Number | 1-800 | Level cap |
| Primary Drop Rate | `primary-drop-rate` | Number | 1-10000 | Common item drop chance |
| Secondary Drop Rate | `secondary-drop-rate` | Number | 1-10000 | Rare item drop chance |
| Skill Point Limit | `character-skill-limit` | Number | 1-5000 | Max total skill points |
| Rep Drop Modifier | `rep<->drop-modifier` | Number | 0-100 | Reputation effect on drops |

- **Save Settings** button — writes changes to config files
- **Apply & Restart** button — saves and restarts all servers
- Changed but unsaved values highlighted in yellow

---

#### Tab 2: Gameplay Tuning
Fine-tuning combat and movement feel. These settings modify the server config or source-level constants. Each setting should have a label, current value, input field, and a brief description.

**Combat Speed:**
| Setting | Description | Default | Range |
|---------|-------------|---------|-------|
| Min Weapon Speed (Axe) | Minimum attack speed for axes | 3 | 0-10 |
| Min Weapon Speed (Long Sword) | Minimum attack speed for LS | 2 | 0-10 |
| Min Weapon Speed (Fencing) | Minimum attack speed for fencing | 1 | 0-10 |
| Min Weapon Speed (Short Sword) | Minimum attack speed for SS | 0 | 0-10 |
| Min Weapon Speed (Archery) | Minimum attack speed for bows | 1 | 0-10 |
| Min Weapon Speed (Hammer) | Minimum attack speed for hammers | 1 | 0-10 |
| Min Weapon Speed (Global) | Absolute minimum for all weapons | 0 | 0-10 |
| Attack Frequency Min (ms) | Minimum time between attacks | 500 | 100-2000 |

**Movement:**
| Setting | Description | Default | Range |
|---------|-------------|---------|-------|
| Move Frequency Min (ms) | Minimum time between moves | 250 | 100-1000 |
| Dash Attack Enabled | Allow dash attacks | ON | ON/OFF |
| Run Speed Multiplier | Movement speed modifier | 1.0 | 0.5-3.0 |

**Logout:**
| Setting | Description | Default | Range |
|---------|-------------|---------|-------|
| Logout Timer (seconds) | Countdown before logout completes | 10 | 0-30 |
| Instant Logout (Testing) | Skip logout timer entirely | OFF | ON/OFF |

When "Instant Logout" is ON, the logout timer is set to 1 second (or 0). When OFF, it uses the configured Logout Timer value. This is for testing convenience — turn it OFF before publishing the server.

**How to implement:** These values should be written to a custom config file (e.g., `C:\Helbreath Project\server_manager_settings.ini`) AND also written to the appropriate server config files where applicable. For settings that require source code constants (like weapon speed minimums), the server manager should modify the config file, and the server source should be updated to READ these values from config instead of being hardcoded.

For the logout timer specifically:
- The client-side logout countdown is in Game.cpp around line 28459: `m_cLogOutCount = 11` (11 seconds in Release, 1 second in Debug). 
- Make this value configurable by reading it from a config file the client loads at startup, or by having the server send the logout timer value to the client on login.

---

#### Tab 3: Quick Actions
Utility functions for server management.

- **Open Server Logs** — opens the log directory in File Explorer
- **Open Config Files** — opens the config directory in File Explorer
- **Open Source Code** — opens the Sources directory in File Explorer
- **Database Backup** — runs mysqldump and saves to a timestamped .sql file in a Backups folder
- **Git Commit & Push** — runs `git add . && git commit -m "[auto] date" && git push` for quick saves
- **Open GM Commands Reference** — opens GM_COMMANDS.md

---

### Config File Strategy
The server manager should maintain its own settings file (`server_manager_settings.ini`) with ALL tunable values. When "Save" or "Apply & Restart" is clicked:
1. Write values to `server_manager_settings.ini` (the manager's own state)
2. Write applicable values to the appropriate server config files (the ones HGserver.exe reads)
3. For values that need client changes (like logout timer), write to a client config file too

This way the server manager always knows the current state even if config files are edited externally.

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
- Save window position/size between sessions
- The old startup and shutdown batch scripts can remain as backups but the GUI replaces them
- **Design for expandability** — new tabs and settings will be added over time. Use a clean structure that makes adding new fields easy (e.g., define settings as a list of dicts that auto-generate the UI fields)

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
