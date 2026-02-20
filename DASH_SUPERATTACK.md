# Helbreath — Dash Speed + Super Attack Text Color

---

## Task 1: Fix Dash Attack Speed (CRITICAL — PvP essential)

### Problem
Dash attacks are too slow. The dash (OBJECTATTACKMOVE) is triggered by Shift+running into an enemy from 2 tiles away. It should be a quick lunge+strike. Currently it feels sluggish.

### Context
The dash is critical for PvP combat in Helbreath — it's the primary way to close distance and attack. It must feel fast and responsive.

### Root Cause Analysis
The dash is NOT a frame-rate issue. It's likely one of these:

**Most likely: The `bCheckClientAttackFrequency` cooldown (HG.cpp line ~37843)**
This check rejects attacks that come faster than 500ms apart. The dash is a move+attack that happens in rapid succession. If the player recently attacked (within 500ms), the dash's attack component gets rejected by this frequency check, creating a perceived delay.

Look at HG.cpp line ~853: The OBJECTATTACKMOVE handler calls `bCheckClientAttackFrequency(iClientH, dwClientTime)` AFTER the attack. But if the previous attack was recent, the 500ms window might be delaying the dash.

**Fix:** Either:
1. Skip the attack frequency check for dash attacks (since they're already throttled by the movement frequency check)
2. Or reduce the attack frequency minimum from 500ms to something lower like 300ms
3. Or check if `NO_MSGSPEEDCHECK` should be defined to disable these speed checks entirely (since this is a private server, speed hack protection is unnecessary)

**Alternative: Client-side animation delay**
The client's `DrawObject_OnAttackMove` uses only 3 frames (cases 1,2,3) with pixel offsets 26, 16, 0. The animation frame advancement is controlled by `iObjectFrameCounter` in the MapData class. Find this function and check if the frame timing for OBJECTATTACKMOVE is using the weapon speed value — if so, the weapon speed minimum fix (WEAPONSPEEDLIMIT) would slow down the dash animation.

Search the MapData source (likely MapData.cpp or Map.cpp in the client source) for `iObjectFrameCounter` and check how it handles OBJECTATTACKMOVE vs OBJECTATTACK timing. The dash frames should advance faster than normal attack frames.

**Also check:** The `m_dwRecentAttackTime` check at HG.cpp line ~6827 requires 100ms between attacks. This should be fine for dash but verify.

### Testing
After fix, the dash should feel like a quick forward lunge — the character slides one tile forward and immediately strikes. The entire animation should complete in about 300-400ms total.

---

## Task 2: Super Attack Counter — Change Text to White

### Problem
The super attack count indicator is displayed in black text, making it hard to see against dark backgrounds.

### Fix
Find where the super attack count is drawn on the client UI. In Game.cpp, search for `m_iSuperAttackLeft` display code. It's around lines 16293-16302:

```cpp
if (m_iSuperAttackLeft > 0)
{   wsprintf(G_cTxt, "%d", m_iSuperAttackLeft);
```

Find the `PutString` or `PutString_SprNum` call that follows and change the color values to white (255, 255, 255) or a bright color. If it uses `PutString`, the color parameter is an RGB value. If it uses `PutString_SprNum`, the last three parameters are R, G, B.

Client rebuild required.

---

## File Locations
- **Server source:** `C:\Helbreath Project\Sources\`
- **Client source:** Game.cpp and MapData source files in the Client source directory
- **Deploy:** Client_d.exe to Client directory, HGserver.exe to all 4 Maps subdirectories
