# LOGOUT TIMER FIX — Exact Instructions

## The Problem
`m_dwLogOutCountTime` is initialized to 0 (line 4470) and never set to the current time when logout begins. The countdown logic at line 30542 checks `(dwTime - m_dwLogOutCountTime) > 1000` — since `m_dwLogOutCountTime` is 0, this is always true, so ALL countdown ticks happen in a single frame. Logout appears instant.

## The Fix
Add `m_dwLogOutCountTime = timeGetTime();` immediately after `m_cLogOutCount` is set at ALL THREE logout initiation points.

---

### Fix Location 1: Game.cpp line 24799 (ESC / Observer logout)

Find this code (around line 24798):
```cpp
if (m_cLogOutCount == -1) {
    m_cLogOutCount = (char)m_iLogOutTimer;
    m_sLogOutStartX = m_sPlayerX;
    m_sLogOutStartY = m_sPlayerY;
}
```

Change to:
```cpp
if (m_cLogOutCount == -1) {
    m_cLogOutCount = (char)m_iLogOutTimer;
    m_dwLogOutCountTime = timeGetTime();
    m_sLogOutStartX = m_sPlayerX;
    m_sLogOutStartY = m_sPlayerY;
}
```

---

### Fix Location 2: Game.cpp line 28564 (SysMenu "Log Out" button)

Find this code (around line 28563):
```cpp
if( m_cLogOutCount == -1 ) {
    m_cLogOutCount = (char)m_iLogOutTimer;
    m_sLogOutStartX = m_sPlayerX;
    m_sLogOutStartY = m_sPlayerY;
} else {
```

Change to:
```cpp
if( m_cLogOutCount == -1 ) {
    m_cLogOutCount = (char)m_iLogOutTimer;
    m_dwLogOutCountTime = timeGetTime();
    m_sLogOutStartX = m_sPlayerX;
    m_sLogOutStartY = m_sPlayerY;
} else {
```

---

### Fix Location 3: Wmain.cpp line 75 (WM_CLOSE / window X button)

Find this code (around line 74):
```cpp
if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > (char)G_pGame->m_iLogOutTimer) {
    G_pGame->m_cLogOutCount = (char)G_pGame->m_iLogOutTimer;
    G_pGame->m_sLogOutStartX = G_pGame->m_sPlayerX;
    G_pGame->m_sLogOutStartY = G_pGame->m_sPlayerY;
}
```

Change to:
```cpp
if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > (char)G_pGame->m_iLogOutTimer) {
    G_pGame->m_cLogOutCount = (char)G_pGame->m_iLogOutTimer;
    G_pGame->m_dwLogOutCountTime = timeGetTime();
    G_pGame->m_sLogOutStartX = G_pGame->m_sPlayerX;
    G_pGame->m_sLogOutStartY = G_pGame->m_sPlayerY;
}
```

---

### Also check: Game.cpp line 36562

There appears to be another logout initiation at line 36562:
```cpp
if (m_cLogOutCount == -1) {
```
If `m_cLogOutCount` is set to a positive value here, add `m_dwLogOutCountTime = timeGetTime();` after it too.

And line 38143:
```cpp
if( m_cLogOutCount < 0 || m_cLogOutCount > 5 ) m_cLogOutCount = 5;
```
This is a forced logout with a 5-second cap. Add `m_dwLogOutCountTime = timeGetTime();` after this line too.

---

## DO NOT TOUCH
- The countdown tick logic at line 30542-30546 — this is correct
- The movement cancel at line 30538-30540 — this is correct
- The `m_iLogOutTimer` parsing from GM.cfg — this is correct

## Verify
1. Rebuild client → Play_Me.exe
2. Set `logout-timer = 10` in Client\GM.cfg
3. Press ESC → Log Out. Should count down 10, 9, 8... one per second
4. Move during countdown → should cancel
5. Set `logout-timer = 1` → should take ~1 second
