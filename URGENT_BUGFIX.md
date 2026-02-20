# URGENT BUGFIX — HGserver.exe crashes on player map entry

**Priority:** Fix this BEFORE continuing any other tasks.

## What happened

1. Before your most recent changes to `C:\Helbreath Project\Sources\HG\HG.cpp`, the server was working perfectly — Cameron could log in, select his character, and enter the game world.
2. After your changes (adding `m_bRecallDamageTimer` for the recall damage timer toggle, and any other modifications), the newly compiled `HGserver.exe` breaks when a player tries to enter the game.
3. **Symptoms:** Login works. Character select works. When the character tries to enter the game world → "Connection Lost."
4. The new `HGserver.exe` is ~1.4MB. The old working one was ~2.9MB — this size difference may indicate a build configuration issue (Release vs Debug, missing dependencies, or missing code).

## What you need to do

### Step 1: Restore the working server FIRST

Check git for the last working version:
```cmd
cd "C:\Helbreath Project"
git log --oneline -10
```

If there's a commit before your changes, restore the old `HGserver.exe`:
```cmd
git stash
```
Then recompile, or find the old exe in git history.

If git doesn't have it, check if any of the map directories still have the old 2.9MB version:
```cmd
dir "C:\Helbreath Project\Maps\Towns\HGserver.exe"
dir "C:\Helbreath Project\Maps\Neutrals\HGserver.exe"
dir "C:\Helbreath Project\Maps\Middleland\HGserver.exe"
dir "C:\Helbreath Project\Maps\Events\HGserver.exe"
```

The old working exe was approximately **2,934,272 bytes (~2.9MB)**. If any copy is that size, immediately copy it to all map server directories to restore functionality.

### Step 2: Diagnose what went wrong

Your recent changes likely involved:
- Adding `m_bRecallDamageTimer` as a member variable in `C:\Helbreath Project\Sources\HG\HG.h` (or wherever CGame is declared)
- Initializing it in the CGame constructor in `HG.cpp` (around line 177)
- Adding case 26 to `bReadSettingsConfigFile()` to parse `recall-damage-timer` from Settings.cfg
- Adding the token match: `if (memcmp(token, "recall-damage-timer", 19) == 0) cReadMode = 26;`
- Modifying the Recall spell check at line 13292

Common causes for this type of crash:
1. **Variable not declared in the header** — if `m_bRecallDamageTimer` is used in HG.cpp but not declared in the class definition in the .h file, it won't compile (but if it did compile, maybe it was declared wrong)
2. **Wrong variable type or memory layout change** — adding a member variable to CGame changes the class memory layout. If other compiled modules (Login Server, etc.) share headers with CGame, they need recompiling too
3. **Initialization order** — if `m_bRecallDamageTimer` initialization is in the wrong place, it could corrupt adjacent memory
4. **Config parsing overflow** — if the case 26 parsing has a bug, it could corrupt the config reading state machine and cause all subsequent settings to be wrong
5. **Build configuration mismatch** — the old exe was ~2.9MB (likely Debug build), the new one is ~1.4MB (likely Release build). Check the Visual Studio build configuration matches what was used before

### Step 3: Fix and recompile

1. Review every change you made to `HG.cpp` and the header file
2. Make sure you're building with the **same configuration** as before (Debug/Release, Win32, v143 toolset)
3. Check that the `.sln` or `.vcxproj` file you're building matches the one that produced the old working exe
4. Compile and verify the new exe is approximately the same size as the old one
5. Copy the fixed `HGserver.exe` to ALL map server directories:
   - `C:\Helbreath Project\Maps\Towns\HGserver.exe`
   - `C:\Helbreath Project\Maps\Neutrals\HGserver.exe`
   - `C:\Helbreath Project\Maps\Middleland\HGserver.exe`
   - `C:\Helbreath Project\Maps\Events\HGserver.exe`

### Step 4: Test

1. Start Login Server
2. Start all HGserver instances
3. Launch client, log in with account **CamB**, select character **toke**
4. Confirm the character loads into the game world without "Connection Lost"
5. Only after this is confirmed working, verify the recall-damage-timer toggle works

### Step 5: THEN continue with remaining tasks

Only after the server is stable and Cameron can play again, continue with:
- Task 1 (Server Manager GUI layout)
- Task 3 (Logout timer)
- Task 4 (Bag inventory display)

> **If you cannot figure out what's wrong within 15 minutes, tell Cameron immediately.** He will consult with Claude to help trace the issue. Do not keep trying different things silently — communicate early.
