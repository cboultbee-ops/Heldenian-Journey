# Helbreath Server Manager — Code Bot Task List

**Date:** February 20, 2026  
**Project:** `C:\Helbreath Project\`  
**GitHub:** `cboultbee-ops/Helbreath-Project`

> **IMPORTANT:** Work through these tasks in order (Task 1 → 2 → 3 → 4). If you get stuck on any task for more than a few minutes, **stop and tell Cameron** so he can consult with Claude (his strategy assistant) to help you through it. Don't spin your wheels — ask for help early.

---

## Task 1: Server Manager GUI — Fix Horizontal Overflow

**Problem:** The first tab of the Server Manager GUI is too wide horizontally. Cameron has to drag/resize the window to see the "Save" and "Save & Restart" buttons. More settings will be added over time, so this needs a scalable solution.

**File to modify:** The Server Manager Python source file. Check these likely locations:
- `C:\Helbreath Project\Helbreath_Server_Manager.py`
- `C:\Helbreath Project\helbreath_server_manager.py`
- `C:\Helbreath Project\server_manager.py`
- Wherever the source `.py` file is that compiles to `C:\Helbreath Project\Helbreath_Server_Manager.exe`

**Requirements:**
1. Reorganize the Gameplay Tuning tab to use a **vertical/stacked layout** that fits within ~650-700px window width
2. Group related settings into clearly labeled sections (e.g., "Movement Speed", "Weapon Speeds", "Attack Timing", "Drop Rates", "Raid Times")
3. Use a **scrollable frame** so the tab can grow vertically as more settings are added without needing a wider window
4. The "Save", "Save & Restart", and any other action buttons must always be **visible without scrolling or resizing** — either pin them at the bottom of the tab outside the scrollable area, or place them at the top
5. Keep the existing dark theme and styling
6. Don't remove any existing functionality

**When done:** Launch the GUI and confirm all settings load correctly, the window fits at ~680x800 without horizontal scrolling, and Save/Save & Restart buttons are visible.

---

## Task 2: Recall Damage Timer Toggle

**Problem:** When a player takes damage, they must wait 10 seconds before the Recall spell works. This is correct for normal gameplay, but during development/testing Cameron wants to toggle this off so he can Recall instantly.

### Step 2A: Add the server-side config variable

**File:** `C:\Helbreath Project\Sources\HGServer\HG.cpp`

**How the Recall damage timer currently works (line 13292):**
```cpp
if((dwTime - caster->m_lastDamageTime) > 10000 || caster->IsGM()){
```
- `m_lastDamageTime` is updated whenever a player takes damage (lines 1035, 20587, 21077, 21858, 21957, 22077, 22141)
- The `10000` is 10 seconds in milliseconds
- GMs already bypass this check via `caster->IsGM()`
- If the check fails, `NOTIFY_CANNOTRECALL` is sent (line 13304)

**Changes needed in `HG.cpp`:**

1. **Add member variable initialization** — around line 177 (after `m_iSuperAttackMultiplier = 100;`), add:
   ```cpp
   m_bRecallDamageTimer = 1;  // 1 = enabled (normal), 0 = disabled (instant recall)
   ```

2. **Add config parsing** — The Settings.cfg parser uses numbered cases in `bReadSettingsConfigFile()`. The last case is 25 (`super-attack-multiplier`). Add case 26:

   In the switch block (after case 25 which ends around line 4434), add:
   ```cpp
   case 26:
       m_bRecallDamageTimer = atoi(token);
       wsprintf(cTxt, "(*) Recall damage timer: %s", m_bRecallDamageTimer ? "ENABLED" : "DISABLED");
       PutLogList(cTxt);
       cReadMode = 0;
       break;
   ```

   In the token matching block (after line 4463 `super-attack-multiplier`), add:
   ```cpp
   if (memcmp(token, "recall-damage-timer", 19) == 0) cReadMode = 26;
   ```

3. **Modify the Recall check at line 13292** — Change from:
   ```cpp
   if((dwTime - caster->m_lastDamageTime) > 10000 || caster->IsGM()){
   ```
   To:
   ```cpp
   if(!m_bRecallDamageTimer || (dwTime - caster->m_lastDamageTime) > 10000 || caster->IsGM()){
   ```

4. **Declare the member variable** — Find the CGame class header file (likely `C:\Helbreath Project\Sources\HGServer\Game.h` or `HG.h`) and add:
   ```cpp
   int m_bRecallDamageTimer;
   ```
   Place it near the other settings variables like `m_iSuperAttackMultiplier`.

### Step 2B: Add the setting to Settings.cfg

**File:** `C:\Helbreath Project\Sources\Settings.cfg` (the one in the HGServer directory that the server actually reads — check `C:\Helbreath Project\Maps\Towns\Settings.cfg` and any other map server directories too)

Add this line at the end:
```
recall-damage-timer		 = 1
```

### Step 2C: Add toggle button to Server Manager GUI

**File:** Same Python source as Task 1

Add a toggle button in the Gameplay Tuning tab (in the toggles/quick actions area) that:
- Reads `recall-damage-timer` from `Settings.cfg`
- Displays current state: "Recall Damage Timer: ON ✓" (green) or "Recall Damage Timer: OFF ✗" (red)
- Clicking it flips the value between `1` and `0` in `Settings.cfg`
- Shows a note: "Requires server restart to take effect"

### Step 2D: Compile and test

1. Compile HGServer in Visual Studio 2022 (v143 toolset, Win32 architecture)
2. Start the server
3. Check the server log for: `(*) Recall damage timer: ENABLED`
4. Toggle it off via the Server Manager, restart, confirm log shows: `(*) Recall damage timer: DISABLED`
5. In-game with character "toke" (account CamB): take damage, then immediately try to Recall. With timer OFF it should work. With timer ON it should show "Cannot Recall" message.

> **IF STUCK:** If you can't find the CGame header file, or the compile fails, tell Cameron. He'll get Claude to help locate the right files.

---

## Task 3: Logout Timer Fix

**Problem:** Cameron needs fast/instant logout during testing so he can quickly jump out of the game. The `GM.cfg` file has `logout-timer = 1` but it may not be wired up properly.

**Investigation needed:** The logout countdown is likely **client-side** (the client shows a timer before sending the disconnect to the server). Check these locations:

1. **Client source:** `C:\Helbreath Project\Sources\Client\Game.cpp` — search for:
   - `logout` / `Logout` / `LOGOUT`
   - `disconnect` / `Disconnect`
   - Any countdown timer that runs when the player presses Escape or a logout button
   - `DEF_LOGOUTTIME` or similar defines
   - Look for something like `m_iLogoutTimer` or a countdown variable

2. **Client header:** `C:\Helbreath Project\Sources\Client\Game.h` — search for logout-related variables

3. **Server side:** In `HG.cpp`, line 321 there's a `m_dwLogoutHackCheck` that logs if someone disconnects within 1 second of damage — this is a hack detection, not the actual logout timer. The real timer is almost certainly in the client.

**Goal:**
- Find where the logout countdown is implemented
- Make it read from a config value (either `GM.cfg` which the Server Manager already writes, or a new client config)
- When `logout-timer = 1`, logout should take 1 second (effectively instant)
- When `logout-timer = 10`, normal 10-second countdown

**Server Manager GUI addition:**
- Add a toggle button near the Recall toggle: "Instant Logout: ON/OFF"
- When ON, sets `logout-timer = 1` in the config
- When OFF, sets `logout-timer = 10`
- If the timer is client-side, the Server Manager may need to write to a client config file instead — adapt accordingly

> **IF STUCK:** The logout timer implementation could be in several places. If you can't find it within 10 minutes of searching, tell Cameron. He'll ask Claude to help trace it through the codebase.

---

## Task 4: Bag Inventory Item Count & Weight Display

**Problem:** When players open their inventory bag in-game, there's no indicator showing how many items they're carrying or how much weight. Players need this to plan their inventory management.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp`

**Investigation needed:**
1. Find where the inventory/bag window is drawn. Search for:
   - `DrawDialogBox_Inventory` or similar function name
   - `DIALOG_INVENTORY` or `DIALOGTYPE_INVENTORY`
   - The bag/backpack UI rendering code
2. Find where item count and weight are tracked:
   - Something like `m_iItemCount` or a loop that counts non-null inventory slots
   - Something like `m_iWeight` or `m_iTotalWeight` for carried weight
   - Max weight is usually based on STR stat
   - Max item slots is usually a fixed constant (like 50 or so)

**Implementation:**
- At the bottom of the bag window (or top, wherever there's space), render two lines of text:
  ```
  Items: 23 / 50
  Weight: 1250 / 3000
  ```
- Use `PutString2()` for rendering — **do NOT use PutString_SprFont3** (sprite fonts don't work in GPU mode, we already fixed this issue for damage numbers)
- Use a readable color — yellow (`RGB(255, 255, 0)`) or white (`RGB(255, 255, 255)`)
- Calculate current item count by counting non-null slots in the inventory array
- Calculate current weight by summing item weights in the inventory
- Get max weight from the character's STR-based carry capacity

**Example rendering code pattern (adapt to actual variable names):**
```cpp
char cTxt[64];
// Item count
int iItemCount = 0;
for (int i = 0; i < DEF_MAXITEMS; i++) {
    if (m_pItemList[i] != NULL) iItemCount++;
}
wsprintf(cTxt, "Items: %d / %d", iItemCount, DEF_MAXITEMS);
PutString2(iX + 10, iY + HEIGHT - 30, cTxt, 255, 255, 0);

// Weight
wsprintf(cTxt, "Weight: %d / %d", m_iCurrentWeight, m_iMaxWeight);
PutString2(iX + 10, iY + HEIGHT - 15, cTxt, 255, 255, 0);
```

The exact variable names and positions will depend on what you find in the source. Adapt accordingly.

> **IF STUCK:** If you can't find the inventory drawing function or the weight variables, tell Cameron. He knows the gameplay mechanics well and Claude can help search the codebase.

---

## After All Tasks

1. **Compile both** HGServer (server) and Client (client) in Visual Studio 2022, v143 toolset, Win32
2. **Copy updated configs** (`Settings.cfg` with new `recall-damage-timer` line) to all map server directories
3. **Test everything** with character "toke" (account CamB):
   - Server Manager: opens at reasonable size, no horizontal scrolling needed
   - Recall toggle: works in both ON and OFF states
   - Logout: fast logout when set to 1
   - Bag display: shows item count and weight when inventory is open
4. **Commit and push** all changes to GitHub (`cboultbee-ops/Helbreath-Project`) with a clear commit message describing what was changed
5. **Tell Cameron** what was completed and if anything needs follow-up

---

## File Location Reference

| File | Path | Purpose |
|------|------|---------|
| HG.cpp | `C:\Helbreath Project\Sources\HGServer\HG.cpp` | Main game server logic |
| Game.h (server) | `C:\Helbreath Project\Sources\HGServer\Game.h` (or `HG.h`) | CGame class header |
| Game.cpp (client) | `C:\Helbreath Project\Sources\Client\Game.cpp` | Client-side game logic |
| Settings.cfg | `C:\Helbreath Project\Sources\Settings.cfg` + each map server dir | Gameplay settings read by HGServer |
| GM.cfg | `C:\Helbreath Project\GM.cfg` | Server Manager config |
| LServer.cfg | `C:\Helbreath Project\LServer.cfg` | Login server config |
| LoginServer.cpp | `C:\Helbreath Project\Sources\LoginServer\LoginServer.cpp` | Login server logic |
| Server Manager | Find the `.py` source for `Helbreath_Server_Manager.exe` | GUI tool |
| GServer.cfg | `C:\Helbreath Project\Maps\Towns\GServer.cfg` (and other map dirs) | Per-map-server network config |
