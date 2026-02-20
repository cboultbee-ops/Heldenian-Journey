# Helbreath — Server Manager Fixes + New Gameplay Options

---

## Task 1: Fix Movement Speed Controls Not Working in Server Manager

### Problem
The movement speed, run speed, and dash speed sliders/fields in the Server Manager GUI don't actually change anything noticeable in-game.

### Root Cause
The Server Manager is likely writing values to its own ini file but NOT writing them to the actual server config files that HGserver.exe reads. Or the server doesn't have config keys for these values — they're hardcoded in the source.

### How Movement/Attack Speed Works in Server Code

**Attack Frequency** (HG.cpp line ~37843):
```cpp
if (dwTimeGap < 500) {  // 500ms minimum between attacks
    // kick player as speed hack
}
```

**Move Frequency** (HG.cpp line ~37906):
```cpp
if (dwTimeGap < 250) {  // 250ms minimum between moves
    // kick player as speed hack
}
```

**Magic Frequency** (HG.cpp line ~37869):
```cpp
if (dwTimeGap < 1500) {  // 1500ms minimum between casts
    // kick player as speed hack
}
```

These are **hardcoded constants** in the source code, NOT read from config files. The Server Manager needs to:

1. Add new config keys to the server config file: `attack-frequency-min`, `move-frequency-min`, `magic-frequency-min`
2. Modify HG.cpp to read these values from the config at startup instead of using hardcoded constants
3. Store them as member variables (e.g., `m_iAttackFreqMin`, `m_iMoveFreqMin`, `m_iMagicFreqMin`)
4. Use those variables in the frequency check functions instead of the hardcoded 500/250/1500

**IMPORTANT:** All three frequency checks are inside `#ifndef NO_MSGSPEEDCHECK` blocks. If `NO_MSGSPEEDCHECK` is defined, these checks are completely skipped. For a private server, it may be simpler to just define `NO_MSGSPEEDCHECK` and skip these anti-cheat checks entirely. BUT if that's done, make sure the game still feels right — these checks also prevent the game from processing inputs too fast.

### For Dash Speed Specifically
The dash (OBJECTATTACKMOVE) calls `bCheckClientAttackFrequency` at HG.cpp line ~853. The 500ms minimum between attacks is what makes the dash feel slow — if you attacked within the last 500ms, the dash's attack component gets rejected.

**Fix:** Either reduce the 500ms to something lower (like 200-300ms), or skip the frequency check for dash attacks specifically by checking `bIsDash`.

### Server Manager Integration
The Server Manager should:
1. Write the frequency values to the server config file
2. The server reads them on startup
3. When "Apply & Restart" is clicked, the new values take effect after restart

---

## Task 2: Super Attack Indicator — White Font

### Problem
The super attack count number is displayed in black or dark text, making it nearly invisible.

### Fix
In Game.cpp, find the super attack display code around lines 16293-16302. Look for the `PutString` or `PutString_SprNum` call that draws `m_iSuperAttackLeft` and change the color to white:
- If `PutString`: change RGB to `RGB(255, 255, 255)`
- If `PutString_SprNum`: change the R, G, B parameters to `255, 255, 255`
- If `PutString2`: change the R, G, B parameters to `255, 255, 255`

Client rebuild required.

---

## Task 3: Add Super Attack Damage Multiplier to Server Manager

### Overview
Add a "Super Attack Damage Multiplier" field to the Gameplay Tuning tab in the Server Manager.

### How Super Attack Damage Works
The `calculateAttackEffect` function receives `wType` (20-27 for super attacks). Inside this function, the super attack type applies bonus damage. Find this function (it may be in a separate .cpp file from HG.cpp — search all server source files for `calculateAttackEffect`).

The damage multiplier should be a configurable value (default 1.0, range 0.5-5.0) that scales the super attack bonus damage. Add it as:
1. A new config key: `super-attack-multiplier`
2. A member variable: `m_fSuperAttackMultiplier`
3. Read from config at startup
4. Applied in the calculateAttackEffect function wherever super attack bonus damage is calculated
5. Exposed in the Server Manager GUI on the Gameplay Tuning tab

---

## Task 4: Add Casting Speed to Server Manager

### Overview
Add a "Casting Speed (min ms)" field to the Gameplay Tuning tab.

### Implementation
The casting speed minimum is currently hardcoded at 1500ms in `bCheckClientMagicFrequency` (HG.cpp line ~37869). Make this configurable:

1. Add config key: `magic-frequency-min` (default 1500, range 500-5000)
2. Add member variable: `m_iMagicFreqMin`
3. Read from config at startup
4. Replace the hardcoded `1500` in bCheckClientMagicFrequency with `m_iMagicFreqMin`
5. Add to Server Manager Gameplay Tuning tab

---

## Task 5: Verify Server Manager Settings Actually Apply

### Checklist
After implementing the above, verify this end-to-end flow works:
1. Change a value in the Server Manager GUI
2. Click "Save" or "Apply & Restart"
3. Verify the value was written to the correct server config file
4. Verify HGserver.exe reads and uses the new value on startup
5. Test in-game that the change has the expected effect

Add a debug log line when each custom config value is loaded, like:
```cpp
wsprintf(g_cTxt, "(*) Attack frequency min: (%d)ms", m_iAttackFreqMin);
PutLogList(g_cTxt);
```

---

## File Locations
- **Server source:** `C:\Helbreath Project\Sources\`
- **Server config files:** Look in `C:\Helbreath Project\Maps\` subdirectories for the config files that HGserver.exe reads (the files containing `exp-modifier`)
- **Client source:** Game.cpp in the Client source directory
- **Server Manager source:** `C:\Helbreath Project\server_manager.py` (or wherever it was created)
