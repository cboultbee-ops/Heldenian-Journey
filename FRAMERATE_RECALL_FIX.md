# Helbreath Client — Frame Rate + Recall Fix

---

## Task 1: Fix Game Speed (Too Fast)

### Problem
The game runs too fast — character swings axe rapidly, runs at high speed, animations play in fast-forward. This affects all movement and combat.

### Root Cause
The original Helbreath client used DirectDraw's `Flip()` with `DDFLIP_WAIT`, which naturally limited the framerate to the monitor's refresh rate (VSync). The GPU/OpenGL renderer does not enforce VSync, so the game loop runs at hundreds or thousands of FPS. Since animation advancement and game logic happen every frame in `UpdateScreen_OnGame()`, everything runs in fast-forward.

### Fix — Two Options (try Option A first)

**Option A: Enable VSync in the GPU renderer (preferred)**

Find the DDraw/GPU renderer source file (likely `DDraw.cpp` or a GPU-specific file in the Client source). Look for where the OpenGL context is created or where `SwapBuffers` / `wglSwapBuffers` is called.

Add VSync enable after OpenGL context creation:
```cpp
// Enable VSync - limits framerate to monitor refresh rate
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 
    (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
if (wglSwapIntervalEXT) {
    wglSwapIntervalEXT(1);  // 1 = VSync on, 0 = VSync off
}
```

This is the cleanest fix — it matches the original DirectDraw behavior.

**Option B: Add a frame limiter in the game loop (fallback)**

If Option A doesn't work or the renderer code is hard to modify, add a Sleep-based frame limiter at the END of `UpdateScreen_OnGame()` in Game.cpp. Find the very end of the function (after the last `iFlip()` call and before the closing brace) and add:

```cpp
// Frame limiter - cap at ~60 FPS (16ms per frame)
static DWORD dwLastFrameTime = 0;
DWORD dwCurrentTime = timeGetTime();
DWORD dwElapsed = dwCurrentTime - dwLastFrameTime;
if (dwElapsed < 16) {
    Sleep(16 - dwElapsed);
}
dwLastFrameTime = timeGetTime();
```

Note: `Sleep(1)` on Windows actually sleeps ~15ms due to timer resolution. For more precise control, call `timeBeginPeriod(1)` at program startup and `timeEndPeriod(1)` at shutdown to get 1ms Sleep resolution. But for a simple 60fps cap, the default Sleep granularity should be close enough.

### Verify
After the fix, enable the FPS display (check if there's a key binding, or set `ShowFPS=1` in the client settings file). The game should run at approximately 60 FPS and movement/attack speed should feel normal.

### Important
This is a CLIENT-SIDE fix only. Rebuild the client, not the server.

---

## Task 2: Fix Recall Spell Disconnect

### Problem
Casting Recall on yourself causes "connection lost" and kicks the player from the game. Other spells (Berserk, Energy Bolt) now work correctly.

### Analysis
Recall teleports the player to their town's spawn point, which may be on a different map handled by a different HGserver instance. The disconnect likely happens during the inter-server transfer process.

### Investigation
1. Check the server console logs for errors when Recall is cast. Look for the HGserver that handles the map the player is on AND the Login server console.

2. Check if Recall is trying to send the player to a map that doesn't exist or isn't loaded. In HG.cpp, search for the Recall handler — look for `MAGICTYPE_TELEPORT` or `MAGICTYPE_RECALL`. The target map should be the player's `m_cLocation` (town affiliation — "elvine" or "aresden").

3. Common causes of Recall disconnect:
   - The target map name in the player's location doesn't match any loaded map name exactly (case-sensitive)
   - The Login server's inter-server transfer handler has a bug or timeout
   - The target map's HGserver isn't running or isn't connected to the Login server
   - The player's save data is corrupted during the transfer

4. Add debug logging in the Recall/Teleport handler:
```cpp
wsprintf(g_cTxt, "(DEBUG) Recall: player=%s location=%s mapName=%s", 
    caster->m_cCharName, caster->m_cLocation, caster->m_cMapName);
PutLogList(g_cTxt);
```

5. Also check if this is a same-server recall (player recalling within a map on the same HGserver) or a cross-server recall. If the player is in Elvine and recalling to Elvine, it should be handled by the same Towns server without needing a server transfer.

### Note
If Recall within the same server zone works (e.g., recalling while already in town), but cross-server Recall disconnects, the issue is in the Login server's transfer logic, not the game server's spell handler.

---

## File Locations
- **Client source:** `C:\Helbreath Project\Sources\Client\`
- **GPU renderer:** Look for DDraw.cpp, GPU.cpp, or OpenGL-related files in the Client source
- **Server source:** `C:\Helbreath Project\Sources\HG\` or `Sources\`
- **Client executable:** `C:\Helbreath Project\Client\Client_d.exe`
