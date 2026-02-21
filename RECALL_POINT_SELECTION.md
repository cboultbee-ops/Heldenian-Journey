# FEATURE: Recall Spell — Select Destination on Minimap

## What Already Exists (Partially Implemented)

### Server Side (HG.cpp) — FULLY WORKING
1. **`m_nextRecallPoint`** — stored per player, initialized to 0 (line 1681)
2. **`RequestSetRecallPoint`** (line 40586) — receives client's chosen recall point (1-5), validates it against the map's `m_pInitialPoint[]` array, and stores it
3. **Recall spell handler** (line 13312) — when casting recall:
   - If `m_nextRecallPoint != 0` AND player is in their home city or middleland:
     - Teleports to the selected `m_pInitialPoint[nextRecallPoint]` coordinates
   - Otherwise: random recall (default behavior)

### Client Side (Game.cpp) — PARTIALLY WORKING
1. **`DlgBoxClick_GuideMap`** (line 37846) — Ctrl+Click on minimap:
   - Only works if `m_bCtrlPressed == TRUE`
   - Has hardcoded recall point coordinates for 3 maps:
     - Aresden (mapIndex 11): 5 points at {140,49}, {68,125}, {170,145}, {140,205}, {116,245}
     - Elvine (mapIndex 3): 5 points at {158,57}, {110,89}, {170,145}, {242,129}, {158,249}
     - Middleland (mapIndex 4): 5 points at {192,228}, {100,70}, {400,100}, {100,400}, {350,400}
   - Range check: 25 tiles radius around each point
   - Sends `MSGID_REQUEST_SETRECALLPNT` with the point index
   - Plays sound on selection

### What's MISSING
1. **No visual indicators on the minimap** — the player can Ctrl+Click but has no idea where the recall points are
2. **No feedback** — no message telling the player which point they selected or that recall is now targeted
3. **Points reset on login** — `m_nextRecallPoint = 0` at PlayerMapEntry, so you have to re-select every time you enter a map
4. **The Middleland coordinates look wrong** — {192,228}, {100,70}, etc. seem like map coordinates but the minimap is 128x128 pixels. These might be map tile coordinates that need to be converted

## What to Implement

### Phase 1: Visual Indicators on Minimap (Client Only)

In `DrawDialogBox_GuideMap`, after drawing the player position dot and before the function returns, add recall point markers. Only draw them when the player is in Elvine, Aresden, or Middleland.

**For zoomed minimap (1:1 pixel = 1 tile):**
```cpp
// Draw recall point markers
if (m_cMapIndex == 3 || m_cMapIndex == 11 || m_cMapIndex == 4) {
    int town;
    if (m_cMapIndex == 11) town = 0;      // Aresden
    else if (m_cMapIndex == 3) town = 1;   // Elvine
    else town = 2;                          // Middleland
    
    int recallPoints[3][5][2] = {
        {{140,49}, {68,125}, {170,145}, {140,205}, {116,245}},
        {{158,57}, {110,89}, {170,145}, {242,129}, {158,249}},
        {{192,228}, {100,70}, {400,100}, {100,400}, {350,400}}
    };
    
    for (int i = 0; i < 5; i++) {
        int rpX = recallPoints[town][i][0];
        int rpY = recallPoints[town][i][1];
        
        if (m_bZoomMap) {
            // Zoomed: direct tile coordinates
            if (rpX >= shX && rpX <= shX+128 && rpY >= shY && rpY <= shY+128) {
                pointX = sX - shX + rpX;
                pointY = sY - shY + rpY;
            } else continue;
        } else {
            // Unzoomed: scale to 128x128
            pointX = sX + (rpX * 128) / m_pMapData->m_sMapSizeX;
            pointY = sY + (rpY * 128) / m_pMapData->m_sMapSizeY;
        }
        
        // Draw a blinking cyan/yellow dot (3x3 pixels)
        // Blink when Ctrl is held to indicate clickability
        bool bShow = true;
        if (m_bCtrlPressed) bShow = ((m_dwCurTime % 600) < 400); // blink
        
        if (bShow) {
            for (int px = -1; px <= 1; px++) {
                for (int py = -1; py <= 1; py++) {
                    if (m_bCtrlPressed)
                        m_DDraw.PutPixel(pointX + px, pointY + py, 255, 255, 0); // yellow when Ctrl held
                    else
                        m_DDraw.PutPixel(pointX + px, pointY + py, 0, 200, 255); // cyan normally
                }
            }
        }
    }
}
```

### Phase 2: Player Feedback (Client Only)

After sending the recall point selection (in `DlgBoxClick_GuideMap`, after line 37891), add a chat message:
```cpp
if (recallPoint != 0) {
    bSendCommand(MSGID_REQUEST_SETRECALLPNT, NULL, NULL, recallPoint, NULL, NULL, NULL, NULL);
    PlaySound('E', 14, 5);
    
    // Show feedback
    char cMsg[64];
    wsprintf(cMsg, "Recall target set to point %d. Cast Recall to teleport.", recallPoint);
    AddEventList(cMsg, 10);
}
```

### Phase 3: Verify Coordinates

The recall points in the client code are **map tile coordinates**, not minimap pixel coordinates. The minimap rendering code converts them properly using `(coord * 128) / mapSize` for the unzoomed view. However, the Middleland coordinates ({192,228}, {100,70}, {400,100}, {100,400}, {350,400}) need to be verified against the actual teleport pad locations on the middleland map.

**To verify:** Check the middleland map config file for `m_pInitialPoint` values. They should match or be near the hardcoded coordinates. If they don't match, update the client's hardcoded coordinates to match the actual pad locations.

Check in the HG server config:
```
grep "initial-point" Maps/Middleland/setup.cfg
```

### Phase 4 (Optional): Show on Minimap Only When Recall is Ready

Instead of always showing the dots, only show them when:
- The player is in a valid recall map (Elvine/Aresden/Middleland)
- The player hasn't taken damage in the last 10 seconds (matches the recall damage timer)

This gives the player a visual cue that recall is available.

## How It Works for the Player
1. Player is in Elvine city
2. Opens minimap — sees cyan dots at each recall pad location
3. Holds Ctrl — dots start blinking yellow to indicate they're clickable
4. Ctrl+Clicks a dot — hears a sound, sees "Recall target set to point 3"
5. Casts Recall spell — teleports to that specific pad instead of random location
6. If no point is selected (default), Recall works normally (random teleport pad)

## Files to Change
- **Game.cpp** — `DrawDialogBox_GuideMap()` (add dot rendering), `DlgBoxClick_GuideMap()` (add feedback message)
- No server changes needed — the server side already works

## Build
Client rebuild only.
