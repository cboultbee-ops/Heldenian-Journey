# Helbreath Tasks — February 20, 2026

**Owner:** Cameron  
**Refer to:** `C:\Helbreath Project\CLAUDE_BRIEFING.md` for all paths, build instructions, and project structure.

> **Rules:**
> 1. Work through tasks in order.
> 2. Build HGserver as **DEBUG** (2.9MB). If you get 1.4MB, STOP — you built Release.
> 3. After ANY server-side change, copy `Maps\HGserver.exe` → Towns, Neutrals, Middleland, Events.
> 4. After ANY client-side change, rebuild client → `Client\Play_Me.exe`.
> 5. If stuck for more than 10 minutes, **tell Cameron immediately** so he can consult Claude.
> 6. Commit and push to GitHub after completing each task or group of related tasks.

---

## Task 1: Quick Fix — Server Manager launches wrong client

**File:** `C:\Helbreath Project\server_manager.py`

Change this line:
```python
CLIENT_EXE = os.path.join(BASE_DIR, "Client", "Client_d.exe")
```
To:
```python
CLIENT_EXE = os.path.join(BASE_DIR, "Client", "Play_Me.exe")
```

This ensures all client-side changes (bag display, logout timer, etc.) are active when launching from the Server Manager.

**Time estimate:** 1 minute.

---

## Task 2: Recall spell costs 0 MP

**File:** `C:\Helbreath Project\Sources\HG\HG.cpp`

The mana cost for all spells is calculated starting at line ~12702:
```cpp
iManaCost = spell->m_manaCost;
```

MP is deducted at line 13973:
```cpp
caster->m_iMP -= iManaCost;
```

The Recall spell type is `MAGICTYPE_TELEPORT`. To make Recall free, add this **after** line 12702 (after `iManaCost = spell->m_manaCost;`):
```cpp
// Recall costs 0 MP
if (spell->m_sType == MAGICTYPE_TELEPORT) {
    iManaCost = 0;
}
```

This goes before the safe-attack-mode multiplier and MP save ratio calculations so they don't override it.

**Compile:** Debug, copy to all 4 map dirs.  
**Test:** Cast Recall with character toke — MP should not decrease.

---

## Task 3: Fix cross-server map transition disconnects

**Problem:** Cameron gets disconnected when running from city maps into Middleland. This is likely the same root cause as the old Recall cross-server disconnect issue — when a player moves to a map hosted on a different game server, the handoff fails.

**Files:**
- `C:\Helbreath Project\Sources\HG\HG.cpp` — `RequestTeleportHandler()` at line ~13986
- Cross-server recall debug logging is at line ~14218
- Map gate/portal transitions are handled in the tile-step logic

**Investigation needed:**
1. Check the server logs when Cameron walks from a Towns map (e.g., elvine) into a Middleland map. Look for `(DEBUG) Recall CROSS-SERVER` messages or errors.
2. The issue is that when a player walks through a map gate to a map on a different HGserver instance, the player data transfer to the other server via the Login Server may be failing.
3. Check `RequestTeleportHandler()` type 2 (direct teleport to specific map/coords) — this is used for map gate transitions.
4. Check the Login Server logs (`C:\Helbreath Project\Login\`) for errors during the handoff.
5. Verify all 4 GServer.cfg files have correct `log-server-address` and `log-server-port` pointing to `172.16.0.1:4000`.

**Possible fixes:**
- If the Login Server isn't properly relaying the player data between game servers, the handoff fails and the client sees "Connection Lost"
- The `game-server-ext-address` in each GServer.cfg must match what the client expects
- There may be a timing issue where the destination server hasn't acknowledged the player before the source server disconnects them

> **IF STUCK:** This is a complex networking issue. Tell Cameron early — Claude has already analyzed the RequestTeleportHandler function and can help trace the problem.

---

## Task 4: Recall to specific location on maps (Recall Pad Selection)

**Context:** In standard Helbreath, when you cast Recall while in Elvine or Aresden city maps, the mini-map shows boxes around available recall pads, and you can click to choose which pad to teleport to. Cameron wants this existing functionality to work, AND wants to extend it to Middleland maps for all players.

**Files:**
- `C:\Helbreath Project\Sources\HG\HG.cpp` — lines 13293-13296 handle `m_nextRecallPoint`:
  ```cpp
  if (caster->m_nextRecallPoint != 0 && strcmp(caster->m_cMapName, sideMap[caster->m_side]) == 0){
      RequestTeleportHandler(iClientH, 3, caster->m_cMapName,
          m_pMapList[caster->m_cMapIndex]->m_pInitialPoint[caster->m_nextRecallPoint].x,
          m_pMapList[caster->m_cMapIndex]->m_pInitialPoint[caster->m_nextRecallPoint].y);
  }
  ```
- `C:\Helbreath Project\Sources\Client\Game.cpp` — search for recall pad selection UI, mini-map click handling, `m_nextRecallPoint`, `NOTIFY_FORCERECALLTIME`

**Requirements:**
1. The existing recall pad selection in Elvine/Aresden cities should work (verify first — it may already work)
2. **Extend to Middleland:** Allow all players to select recall points on Middleland maps (middleland, huntzone1-4, etc.)
3. **Faction restriction:** An Elvine player should NOT be able to use recall pad selection while in Aresden city (and vice versa). The existing code at line 13293 already checks `sideMap[caster->m_side]` — verify this works correctly
4. On Middleland maps, both factions should be able to use recall pad selection

**Server-side change (HG.cpp line ~13293):** Expand the condition to also allow recall point selection on Middleland maps:
```cpp
if (caster->m_nextRecallPoint != 0 && 
    (strcmp(caster->m_cMapName, sideMap[caster->m_side]) == 0 ||
     m_pMapList[caster->m_cMapIndex]->m_bIsMiddleland == TRUE)) {
```
(Find the correct flag/check for Middleland maps — it may be `m_bIsMiddleland` or you may need to check the map name.)

**Client-side:** The recall pad UI may need to be enabled for Middleland maps in the client as well. Search `Game.cpp` for where recall pad boxes are drawn on the mini-map.

> **IF STUCK:** Tell Cameron — this involves both server and client changes and faction logic.

---

## Task 5: Remove hover text over players and enemies

**Problem:** When hovering the mouse over your own character or enemy creatures, extra green text appears that shouldn't be there. This is debug or extended info text that needs to be removed or hidden.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp`

**Investigation:**
1. Search for where mouse-hover text is drawn over characters/NPCs. Look for:
   - `DrawObjectName` or similar function
   - Mouse position checks combined with `PutString` / `PutString2` calls
   - `m_stMCursor` or mouse cursor position checks near character drawing code
   - `OBJECTTYPE_PLAYER` or `OBJECTTYPE_NPC` combined with text rendering
2. Identify which text is the unwanted extra info (likely stat/debug info added during development)
3. Remove or comment out the unwanted text rendering — keep the character name display

**Note:** Cameron will likely need to provide a screenshot to clarify exactly which text is unwanted. Ask him if it's not clear from the code which text to remove.

---

## Task 6: Enemy creature health bars visible during combat

**Problem:** When attacking enemy creatures (monsters/NPCs), their health bar should visibly decrease as they take damage. Currently it may not be showing or updating.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp`

**Investigation:**
1. Search for health bar rendering: `DrawHP`, `HPbar`, `m_iHP`, `OBJECTTYPE_NPC`, `DrawObjectHPBar`
2. Health bars for enemies should appear once the player engages them in combat (first hit)
3. The health bar should show damage proportion (current HP / max HP)

**Requirements:**
- Enemy creatures show a red health bar that depletes as they take damage
- Health bar only appears once you start attacking (not before)
- Party members show a **green** health bar instead of red
- **Faction restriction:** An Aresden player should NOT see an Elvine player's health bar and vice versa. Only same-faction and party members.

---

## Task 7: Click-to-sell items from bag to NPC sell window

**Problem:** Currently when selling items to an NPC (shop, blacksmith, etc.), you must drag items from your bag to the sell list. Cameron wants a click option for faster selling.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp`

**Investigation:**
1. Search for the sell dialog/window: `DIALOG_SELL`, `DIALOGTYPE_SELL`, `DrawDialogBox_Sell`, `SellList`
2. Find where the bag inventory handles click events while the sell window is open
3. Currently double-click or drag is required — add single-click (or right-click) on a bag item while sell window is open to move it to the sell list

**Implementation:**
- When the sell window is open and the player clicks an item in their bag, automatically add that item to the sell list (same as if they dragged it)
- Keep the existing drag functionality as well — this is an addition, not a replacement

---

## Task 8: Ctrl+Click to equip stacked items

**Problem:** When multiple equipment pieces are stacked in the same bag slot area, equipping them requires double-clicking each one individually. Cameron wants to hold Ctrl and click once to equip all items in that stack rapidly.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp`

**Investigation:**
1. Find where inventory item clicks are handled — look for mouse click + inventory slot detection
2. Find the equip logic (double-click on item to equip)
3. Add a check: if Ctrl is held during click, loop through items at that position and equip each one sequentially

**Implementation:**
- Detect `GetKeyState(VK_CONTROL)` during inventory click
- If Ctrl is held, find all items overlapping that bag position
- Equip each one in sequence (with a small delay between each to avoid server rejection)

---

## Task 9: Weapon and armor color based on type/enchantment

**Problem:** Weapons and armor all appear the same color regardless of their type or enchantment. A Poisoning Great Sword should appear green, Knight Armor should be gold/brass, etc. Dye items on capes also don't work.

**File:** `C:\Helbreath Project\Sources\Client\Game.cpp` (and possibly sprite/rendering code)

**Investigation:**
1. Search for item color/tint rendering: `ItemColor`, `m_cItemColor`, `m_wColor`, `dwColor`, `ITEMCOLOR`
2. In Helbreath, item colors are typically determined by a color index that maps to palette shifts on the sprite
3. The color may be stored in the item data sent from server to client
4. Check if the color data exists in the item structure but isn't being applied during rendering
5. For dyes specifically, search for `DYE`, `ItemDye`, cape color logic

**Server-side check:** In `HG.cpp`, verify that item color data is being sent to the client in the item data packets. Search for `m_cItemColor` or similar in the character data save/load and item notification messages.

> **IF STUCK:** This could be a complex rendering issue involving palette manipulation. Tell Cameron — he knows how the original Helbreath color system worked and Claude can help trace the data flow.

---

## After All Tasks

1. Compile HGserver (Debug) → copy to all 4 map dirs
2. Compile Client (Release) → `Play_Me.exe`
3. Test with Server Manager: Start All → Launch Client → log in as CamB/toke
4. Verify each completed feature
5. Commit and push all changes to GitHub with descriptive commit message
6. Tell Cameron what was completed and what needs follow-up

---

## Priority Order

| Priority | Task | Difficulty | Type |
|----------|------|-----------|------|
| 1 | Task 1: Fix client exe path | Trivial | server_manager.py |
| 2 | Task 2: Recall 0 MP | Easy | Server (HG.cpp) |
| 3 | Task 3: Cross-server disconnect fix | Hard | Server + Login |
| 4 | Task 5: Remove hover text | Medium | Client (Game.cpp) |
| 5 | Task 6: Enemy health bars + party green bars | Medium | Client (Game.cpp) |
| 6 | Task 7: Click-to-sell | Medium | Client (Game.cpp) |
| 7 | Task 8: Ctrl+Click equip stack | Medium | Client (Game.cpp) |
| 8 | Task 4: Recall pad selection on Middleland | Hard | Server + Client |
| 9 | Task 9: Weapon/armor colors | Hard | Server + Client |
