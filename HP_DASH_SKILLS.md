# Helbreath Client/Server — HP Display, Dash Speed, Skills

---

## Task 1: Fix HP Bar Not Showing Numbers

### Problem
The HP bar does not display the HP number. The MP bar works fine.

### Root Cause (CONFIRMED)
In Game.cpp around line 18104, the HP display code has TWO bugs:

```cpp
wsprintf(G_cTxt, "(%d)", (short) m_iHP);
```

**Bug 1:** The `(short)` cast truncates m_iHP. At high levels, HP exceeds 32,767 (max short value), causing overflow to a negative number. The `-` character isn't supported by `PutString_SprNum`.

**Bug 2:** The format `"(%d)"` includes parentheses `(` and `)`. The `PutString_SprNum` function (line ~7460) only renders digits 0-9 (characters 0x30-0x39). Parentheses are silently skipped. Even at low HP, the parentheses consume space and may shift the digits off-screen or produce invisible output.

### Fix
Change line ~18104 from:
```cpp
wsprintf(G_cTxt, "(%d)", (short) m_iHP);
```
To:
```cpp
wsprintf(G_cTxt, "%d", m_iHP);
```

This removes the cast AND the parentheses to match the MP format. The MP line (line ~18119) already uses `"%d"` with no cast and works correctly.

### Rebuild
Client-side fix. Rebuild the client.

---

## Task 2: Dash Attack Too Slow

### Problem
Dash attacks (Shift+run toward enemy at 2 tiles) are noticeably slow compared to standard Helbreath.

### Analysis
The dash is a combined move+attack (OBJECTATTACKMOVE). The client animation uses 3 frames with pixel offsets (26, 16, 0) for the movement. The server processes it as a move followed by an attack.

The slowness may be caused by:

1. **The weapon speed minimum fix from earlier** — if the `WEAPONSPEEDLIMIT` fix increased the weapon's base speed value too high, it would also slow the dash animation since the client uses weapon speed to time attack animations. Check what minimum was set for axes — standard Helbreath uses `sSpeed < 3` for axes but the dash should still feel fast.

2. **The attack frequency check** — `bCheckClientAttackFrequency` (HG.cpp line ~37831) rejects attacks faster than 500ms apart. The dash is a move+attack that should happen quickly. If this check is blocking the attack portion, the player would see the dash movement but the attack would be delayed.

3. **Client-side animation timing** — The `iObjectFrameCounter` function controls how fast animation frames advance. If the VSync or frame limiter changes affected the frame counter speed, dash animations would slow down.

### Investigation
1. Check if the dash worked at normal speed BEFORE the weapon speed fix. If so, the weapon speed minimum is too high for dash attacks.
2. Add debug logging in the OBJECTATTACKMOVE handler (HG.cpp line ~843) to log timestamps.
3. Compare the attack animation frame timing between normal attacks and dash attacks on the client.
4. Check the `m_cSpeed` value of the Battle Axe in Item.cfg — what is the base weapon speed?

### Possible Fix
If the weapon speed minimum is making dashes feel slow, consider NOT applying the weapon speed minimum during dash attacks. In the OBJECTATTACKMOVE handler at line 848, the attack is called with `bIsDash = TRUE`. The `calculateAttackEffect` function receives this parameter. The speed sent to the client could bypass the minimum for dash attacks.

---

## Task 3: Skills — What's Available

### Current State
The server and client support exactly **10 skill slots** (indices 0-9). The database stores 10 skill mastery values. This is a hard limit in the code — changing it requires modifying the database format, save/load code, and client display.

### Standard Helbreath Skill Mapping (indices 0-9):
| Index | Skill | Notes |
|-------|-------|-------|
| 0 | Mining | Gathering |
| 1 | Manufacturing | Item crafting |
| 2 | Alchemy | Potion making |
| 3 | Magic | Spell casting power |
| 4 | Short Sword | Daggers, short swords |
| 5 | Long Sword | Most melee weapons |
| 6 | Fencing | Esterks |
| 7 | Axe | Battle axes |
| 8 | Shield | Shield defense |
| 9 | Archery | Bows |

### Hidden Skills (exist in code but NOT in the 10-slot display):
These skills are referenced in the server source but don't have dedicated display slots:
- **Hammer** — referenced in weapon speed code and `_CheckAttackType` case 26
- **Staff/Wand** — referenced in `_CheckAttackType` case 27
- **Hand Attack** — referenced in `_CheckAttackType` case 20
- **Fishing** — referenced in `SKILL_FISHING`, has SSN calculation
- **Farming** — referenced in `SKILL_FARMING`
- **Pretend Corpse** — referenced as `SKILL_PRETENDCORPSE`

### To Add Hammer to the F8 Window
Since we only have 10 slots, one of the existing skills would need to be removed to make room for Hammer. Options:
- Replace Mining (index 0) if mining isn't used
- Replace Manufacturing (index 1) if crafting isn't used yet
- Or **expand to more than 10 skill slots** — this is a larger change requiring:
  1. Increase `MAXSKILLTYPE` constant
  2. Expand the database storage (currently 10 bytes at offsets 598-607)
  3. Modify `MSGID_PLAYERSKILLCONTENTS` to send more skills (HG.cpp lines 3735-3762)
  4. Modify client `InitSkillList()` to receive more skills
  5. Modify F8 dialog drawing to show more skills
  6. Update both Skillcfg.txt files

### Recommendation
If you want Hammer, Staff, and Hand Attack visible, expand to 13 skill slots. This is a medium-sized code change but doable. Let Cameron confirm which skills they want visible before making changes.

---

## Task 4: Remove Debug Coordinate Display

Delete this line from Game.cpp (around line 2806):
```cpp
{ char dbg[64]; wsprintf(dbg, "X:%d Y:%d", msX, msY); PutString2(300, 10, dbg, 255, 255, 0); }
```

Rebuild the client.
