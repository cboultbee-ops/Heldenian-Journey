# Helbreath — Remaining Issues Task List

---

## Task 1: Fix HP Bar Number Display

### Problem
HP bar shows no numbers. MP bar works correctly.

### Root Cause
Game.cpp around line 18104:
```cpp
wsprintf(G_cTxt, "(%d)", (short) m_iHP);
```

Two bugs: (1) `(short)` cast overflows at high HP (>32767), producing negative numbers. (2) Parentheses `()` are not supported by `PutString_SprNum` which only renders digits 0-9.

### Fix
Change to:
```cpp
wsprintf(G_cTxt, "%d", m_iHP);
```
This matches the working MP line at ~line 18119. Client rebuild required.

---

## Task 2: Skip Server Selection Screen

### Problem
After clicking Login on the main menu, there's an unnecessary server selection page before the username/password page. Since this is a private server with only one server, this extra page should be skipped.

### Fix
In Game.cpp, find everywhere that changes to `GAMEMODE_ONSELECTSERVER` and change them to go directly to `GAMEMODE_ONLOGIN` instead. There are several locations:
- Line ~2789: `ChangeGameMode(GAMEMODE_ONSELECTSERVER)` → `ChangeGameMode(GAMEMODE_ONLOGIN)`
- Line ~2818: same change
- Line ~23457: same change
- Line ~23482: same change
- Line ~24059: same change (this is in UpdateScreen_OnLogin itself, going back after connection failure)
- Line ~24123: same change

Also make sure the world server name is set before entering login mode, since that was previously set on the server selection screen. Add before the `ChangeGameMode(GAMEMODE_ONLOGIN)` call:
```cpp
if (strlen(m_cWorldServerName) == 0) {
    ZeroMemory(m_cWorldServerName, sizeof(m_cWorldServerName));
    strcpy(m_cWorldServerName, NAME_WORLDNAME1);
}
```

The `UpdateScreen_OnSelectServer()` function can remain in the code but will simply never be called.

Client rebuild required.

---

## Task 3: Fix Password Field Not Showing Text

### Problem
When typing a password on the login screen, no characters appear in the password field (not even asterisks/dots).

### Investigation
Look at `UpdateScreen_OnLogin()` in Game.cpp (around line 23975). Find where the password input field is drawn. The password field should show `*` characters for each typed character. Check:
1. Is `StartInputString` called for the password field?
2. Is the password field being drawn with `PutString` using the `bHide=TRUE` parameter?
3. Is the input string being captured at all?

The password field likely uses the same `PutString` function as the username, but with `bHide=TRUE` which converts characters to `*`. If the GPU renderer's text drawing has issues, the asterisks might not render. Check if `PutString` or `PutString2` is being used and if the hide mode works with the GPU renderer.

---

## Task 4: Escape Key to Logout

### Current Behavior
In standard Helbreath, pressing Escape opens a system menu dialog with a Logout button. The code in Game.cpp line ~24735 handles VK_ESCAPE and references `m_cLogOutCount` and dialog box 19.

### Check
1. Does pressing Escape in-game do anything at all? (open a menu, show a dialog?)
2. The code at line 24744 checks `if(m_cLogOutCount != -1)` — if the logout countdown is already active, Escape cancels it. If it's not active (`m_cLogOutCount == -1`), nothing visible happens because there's no `else` clause that opens the system menu.
3. The logout button is in a dialog box click handler at line ~28456. Dialog box 19 is the system menu.
4. Look for where dialog box 19 is enabled. The Escape key should call `EnableDialogBox(19, ...)` when no logout is in progress.

### Fix
If Escape doesn't open the system menu, add this to the VK_ESCAPE handler when `m_cLogOutCount == -1`:
```cpp
if (m_cLogOutCount == -1) {
    if (m_bIsDialogEnabled[19])
        DisableDialogBox(19);
    else
        EnableDialogBox(19, 0, 0, 0, NULL);
}
```

This toggles the system menu on/off with Escape. The system menu should already have a Logout button.

Client rebuild required.

---

## Task 5: Magic System — Spells Too Easy to Cast

### Current Behavior
Character with 29 INT can cast Berserk (Circle 6) and Energy Strike successfully. In standard Helbreath, spells must be purchased from the Magic Shop NPC and INT affects success rate.

### Root Cause
1. **All spells set to learned** — The database was updated to set all magic mastery values to 1, meaning every spell is "known" without buying them.
2. **Magic skill hardcoded to 100** — In HG.cpp line ~12508, the magic skill mastery check was replaced with a hardcoded value:
```cpp
dV1 = (double)100.0f;  // Should be: dV1 = (double)caster->m_cSkillMastery[SKILL_MAGIC];
```

### Decision for Cameron
This is a gameplay design choice. Options:
- **Option A: Keep it easy** — Leave spells all learned and magic at 100. Good for testing.
- **Option B: Restore standard magic system** — Uncomment the original magic skill code, reset spell masteries to 0 in the database, and require players to buy spells from the Magic Shop NPC. This means you'll need the Magic Shop NPC to be functional.
- **Option C: Hybrid** — Keep all spells learned but restore the magic skill check so INT and Magic skill level affect success rate. High circle spells would fail more often with low stats.

**Do NOT change this without confirming with Cameron which option they want.** For now, document the issue and ask.

---

## Task 6: GM Commands Reference

### Setup
The character "toke" needs admin level set in the database. The admin level is stored at byte offset 242 in the character save data. Set it to a value >= 1 for basic GM, >= 4 for full admin. Verify with: check what value `m_iAdminUserLevel` loads at HG.cpp line 4672.

Alternatively, check if there's a way to set admin level via the Login server database directly.

### Available GM Commands (type in chat with / prefix)
These require `m_iAdminUserLevel >= 1` unless noted:

**Player Management:**
- `/who` — Show online players
- `/summonplayer [name]` — Teleport a player to you
- `/closeconn [name]` — Disconnect a player
- `/shutup [name]` — Mute a player
- `/ban [name]` — Ban a player
- `/checkip [name]` — Check player's IP
- `/disconnectall` — Disconnect all players

**Teleportation:**
- `/goto [map]` — Teleport to a map
- `/tp [name]` or `/teleport [name]` — Teleport to a player

**Items & Stats:**
- `/ci [itemname]` or `/createitem [itemname]` — Create an item (admin level >= 2)
- `/sethp` — Set HP
- `/setmp` — Set MP
- `/setmag` — Set magic
- `/setattackmode [mode]` — Change attack mode

**Character:**
- `/setinvi` or `/invi` — Set invisible
- `/invincible` — Toggle invincibility
- `/noaggro` — Toggle monster aggro
- `/obs` or `/setobservermode` — Observer mode (admin >= 3)
- `/polymorph [npc]` — Transform into an NPC
- `/rep+ [name]` — Increase reputation
- `/rep- [name]` — Decrease reputation

**Map & Events:**
- `/clearmap` — Clear current map
- `/summon [npc]` — Summon an NPC/monster
- `/unsummonall` — Remove all summoned NPCs
- `/attack [target]` — Force attack
- `/createfish [type]` — Spawn fish
- `/energysphere [value]` — Energy sphere event

**Server Events (admin >= 4):**
- `/crusade` — Start crusade
- `/endcrusade` — End crusade
- `/apocalypse` — Start apocalypse event
- `/endapocalypse` — End apocalypse
- `/heldenian` — Start Heldenian
- `/endheldenian` — End Heldenian
- `/astoria [param]` — Astoria event
- `/endastoria` — End Astoria
- `/eventspell` — Spell event
- `/eventarmor` — Armor event
- `/eventshield` — Shield event
- `/eventchat` — Chat event
- `/eventparty` — Party event
- `/eventreset` — Reset events
- `/eventtp` — Teleport event
- `/eventillusion` — Illusion event
- `/reservefightzone` — Reserve fight zone
- `/shutdownthisserverrightnow [delay]` — Shutdown server (admin >= 4)

**Party:**
- `/joinparty [name]` — Join party
- `/dismissparty` — Dismiss party
- `/getpartyinfo` — Get party info
- `/deleteparty` — Delete party

**Other:**
- `/mcount` — Monster count
- `/send [message]` — Send server message
- `/afk` — Toggle AFK
- `/checkrep` — Check reputation
- `/setpf [value]` — Set profile
- `/pf [name]` — View profile
- `/hold` — Hold/freeze
- `/free` — Unfreeze
- `/setforcerecalltime [seconds]` — Set force recall timer
- `/gns [value]` — Unknown (guild related?)
- `/getticket` — Get ticket (admin >= 2)

### Create This as a Separate Document
Save this command reference as `C:\Helbreath Project\GM_COMMANDS.md` so Cameron can reference it easily.

---

## File Locations
- **Client source:** Game.cpp and related files in Client source directory
- **Server source:** HG.cpp in `C:\Helbreath Project\Sources\`
- **Character database:** MySQL, table for character data, byte offset 242 = admin level
