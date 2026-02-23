# TILE_SIZE & Resolution Constants Audit

**Date**: 2026-02-22
**Purpose**: Identify every hardcoded tile-size (32) and resolution-derived constant (320, 240, 288, 224, 427, 639, 479, 640, 480) in the client codebase. Foundation for Option 2 (TILE_SIZE=64) refactor if Option 1 (supersampling) proves insufficient.

---

## Summary Statistics

| Category | Count | Files Affected | Option 2 Impact |
|----------|-------|----------------|-----------------|
| `*32` tile-to-pixel | ~200+ | Game.cpp, MapData.cpp, MessageHandler.cpp | ALL must change to `*TILE_SIZE` |
| `/32` pixel-to-tile | ~40+ | Game.cpp, MapData.cpp | ALL must change to `/TILE_SIZE` |
| `+= 32` tile loops | ~12 | Game.cpp | Change to `+= TILE_SIZE` |
| `+= 32` / `-= 32` movement | 16 | MessageHandler.cpp | Change to `+= TILE_SIZE` |
| `320` sound center X | ~50+ | Game.cpp | Change to `VIRTUAL_HALF_W` |
| `240` sound center Y | ~50+ | Game.cpp | Change to `VIRTUAL_HALF_H` |
| `427` playable bottom | ~10 | Game.cpp, Sprite.cpp | Most already use `PLAYABLE_H` |
| `639` screen max X | ~40+ | Game.cpp, DXC_ddraw.cpp | Change to `VIRTUAL_W-1` |
| `479` screen max Y | ~30+ | Game.cpp | Change to `VIRTUAL_H-1` |
| `640` screen width | ~10 | Game.cpp, Sprite.cpp | Most already use `VIRTUAL_W` |
| `480` screen height | ~5 | Game.cpp | Most already use `VIRTUAL_H` |

**Estimated total changes for Option 2: 450+ line edits across 4-5 files.**

---

## 1. Tile-to-Pixel Conversion (`*32`)

These convert tile coordinates to pixel coordinates. Under Option 2 with TILE_SIZE=64, all must change to `*TILE_SIZE`.

### Game.cpp — DrawObjects/Camera (~20 occurrences)
```
2272: m_sViewDstX = (indexX*32) - VIRTUAL_HALF_W;          // Camera centering
2273: m_sViewDstY = (indexY*32) - (VIRTUAL_HALF_H - 32);   // Camera centering
```

### Game.cpp — Effects/Spells (~150+ occurrences)
Pattern: `m_pEffectList[i]->m_mX = sX*32;` or `bAddNewEffect(N, dX*32, dY*32, ...)`
```
4953: m_pEffectList[i]->m_mX     = sX*32;
4954: m_pEffectList[i]->m_mY     = sY*32 - _iAttackerHeight[iV1];
5636: bAddNewEffect(69, dX*32 +20 - (rand() % 40), dY*32 +20 - (rand() % 40), ...);
5686: bAddNewEffect(14, dX*32 + (rand() % 120) - 60, dY*32 + (rand() % 80) - 40, ...);
... (continues for 100+ similar lines in the spell/combat effect system)
```

### Game.cpp — Effect Rendering (~50 occurrences)
Pattern: `dX = (m_pEffectList[i]->m_dX*32) - m_sViewPointX;`
```
6471: dX  = (m_pEffectList[i]->m_dX*32)  - m_sViewPointX;
6472: dY  = (m_pEffectList[i]->m_dY*32)  - m_sViewPointY;
... (repeated for ~25 effect types: lines 6493-6970)
```

### Game.cpp — Thunder Effect
```
6594: _DrawThunderEffect(m_pEffectList[i]->m_dX*32 - m_sViewPointX, m_pEffectList[i]->m_dY*32 - m_sViewPointY - 800, ...);
... (6 lines total: 6594-6601)
```

### Game.cpp — Crusade Construction/Teleport Markers
```
13669: DrawNewDialogBox(..., m_iConstructLocX*32 - m_sViewPointX, m_iConstructLocY*32 - m_sViewPointY, 41);
13670: DrawNewDialogBox(..., m_iTeleportLocX*32 - m_sViewPointX, m_iTeleportLocY*32 - m_sViewPointY, 42);
```

### Game.cpp — Projectile Hit Detection (~30 occurrences)
Pattern: `abs(m_pEffectList[i]->m_mX - m_pEffectList[i]->m_dX*32)`
```
13708: if (abs(m_pEffectList[i]->m_mX - m_pEffectList[i]->m_dX*32) <= 2)
... (repeated for each projectile type: lines 13700-14470)
```

### Game.cpp — Item Ground Sprite Position
```
3923: m_pSprite[...]->_GetSpriteRect(sX + 32 + m_pItemList[cItemID]->m_sX, ...);
3934: bHit = m_pSprite[...]->_bCheckCollison(sX + 32 + m_pItemList[cItemID]->m_sX, sY + 44 + ...);
```

### Game.cpp — Direction-to-Pixel Offset
```
8417: case 1: dy = 32; break;
8418: case 2: dy = 32; dx = -32; break;
8419: case 3: dx = -32; break;
8420: case 4: dx = -32; dy = -32; break;
8421: case 5: dy = -32; break;
8422: case 6: dy = -32; dx = 32; break;
8423: case 7: dx = 32; break;
8424: case 8: dx = 32; dy = 32; break;
```

### MessageHandler.cpp — Player Movement (~16 occurrences)
```
3386: case 1: m_pGame->m_sViewDstY -= 32; m_pGame->m_sPlayerY--; break;
3387: case 2: m_pGame->m_sViewDstY -= 32; m_pGame->m_sPlayerY--; m_pGame->m_sViewDstX += 32; ...
3388: case 3: m_pGame->m_sViewDstX += 32; m_pGame->m_sPlayerX++; break;
3389: case 4: m_pGame->m_sViewDstY += 32; ...; m_pGame->m_sViewDstX += 32; ...
3390: case 5: m_pGame->m_sViewDstY += 32; ...
3391: case 6: m_pGame->m_sViewDstY += 32; ...; m_pGame->m_sViewDstX -= 32; ...
3392: case 7: m_pGame->m_sViewDstX -= 32; ...
3393: case 8: m_pGame->m_sViewDstY -= 32; ...; m_pGame->m_sViewDstX -= 32; ...
```

### MessageHandler.cpp — Teleport Camera Snap
```
2137: m_pGame->m_sViewDstX = m_pGame->m_sViewPointX = (m_pGame->m_sPlayerX - 20) * 32;
2138: m_pGame->m_sViewDstY = m_pGame->m_sViewPointY = (m_pGame->m_sPlayerY - 14) * 32;
2241-2242: (same pattern, repeated for respawn)
2330-2331: (same pattern, repeated for map change)
```

### MessageHandler.cpp — Effect Spawning (~100+ occurrences)
```
2753: m_pGame->bAddNewEffect(64, (sX)*32, (sY)*32, ...);
3606: m_pGame->bAddNewEffect(12, (m_sPivotX+dX)*32 + 5 - (rand() % 10), ...);
... (continues for all combat effects, weather, dynamic objects)
```

### MapData.cpp — Map Coordinate System (~30 occurrences)
```
1816: sVal = sViewPointX - (m_sPivotX*32);
1817: sCenterX = (sVal / 32) + 10;
1818: sVal = sViewPointY - (m_sPivotY*32);
1819: sCenterY = (sVal / 32) + 8;
1878: m_pGame->bAddNewEffect(65, (m_sPivotX+dX)*32 +(rand()%10-5) +5, (m_sPivotY+dY)*32, ...);
... (continues for dynamic object effects, weather, animations)
```

---

## 2. Pixel-to-Tile Conversion (`/32`)

### Game.cpp — Sound Distance Calculation (~20 occurrences)
Pattern: `abs(((m_sViewPointX / 32) + 10) - sX)`
```
4929: sAbsX = abs(((m_sViewPointX / 32) + 10) - dX);
4930: sAbsY = abs(((m_sViewPointY / 32) + 7) - dY);
4975: sAbsX = abs(((m_sViewPointX / 32) + 10) - sX);
4976: sAbsY = abs(((m_sViewPointY / 32) + 7)  - sY);
... (repeated ~20 times throughout effect system)
```
Note: The `+ 10` and `+ 7` derive from the old 640/480 viewport: 640/32/2=10, 480/32/2~=7.

### Game.cpp — Sound Panning (tile-based)
Pattern: `lPan = -(((m_sViewPointX / 32) + 10) - sX)*1000;`
```
4979: lPan = -(((m_sViewPointX / 32) + 10) - sX)*1000;
... (repeated ~30 times: lines 4979-5855)
```

### Game.cpp — Distance-to-Volume
Pattern: `sDist = sDist / 32;`
```
4992: sDist = sDist / 32;
5008: sDist = sDist / 32;
5023: sDist = sDist / 32;
... (repeated ~25 times)
```

### MapData.cpp
```
1816-1819: sVal = sViewPointX - (m_sPivotX*32); sCenterX = (sVal / 32) + 10;
2157: cFrameMoveDots = 32 / cTotalFrame;  // Animation frame distribution
```

---

## 3. Tile Loop Increments (`+= 32`)

### Game.cpp — DrawBackground (ALREADY PARTIALLY FIXED)
```
13616: for (iy = -sModY; iy < PLAYABLE_H + 48; iy += 32)    // GPU path — FIXED bounds
13618:     for (ix = -sModX; ix < VIRTUAL_W + 48; ix += 32)   // GPU path — FIXED bounds
13634: for (iy = -sModY; iy < 427+48 ; iy += 32)             // DD path — NEEDS UPDATE
13637:     for (ix = -sModX; ix < 640+48 ; ix += 32)          // DD path — NEEDS UPDATE
13655: for (iy = -sModY; iy < PLAYABLE_H+48 ; iy += 32)      // Grid draw — FIXED bounds
13658:     for (ix = -sModX; ix < VIRTUAL_W+48 ; ix += 32)    // Grid draw — FIXED bounds
```

### Game.cpp — DrawObjects (ALREADY FIXED)
```
2026: for (iy = -sModY-480; iy <= PLAYABLE_H+704; iy += 32)  // FIXED bounds
2028:     for (ix = -sModX-256; ix <= VIRTUAL_W + 256; ix += 32)  // FIXED bounds
```

---

## 4. Sound Panning Centers (`320`, `240`)

These use the old screen center (320=640/2, 240=480/2) for sound positioning. Under both Option 1 and Option 2, these should be updated to `VIRTUAL_HALF_W` and `VIRTUAL_HALF_H`.

### Game.cpp — Pixel-based Sound Distance (~50+ occurrences)
Pattern: `sAbsX = abs(320 - (sX - m_sViewPointX));`
```
4988: sAbsX = abs(320 - (sX - m_sViewPointX));
4989: sAbsY = abs(240 - (sY - m_sViewPointY));
5004-5005: (same)
5019-5020: (same)
5113-5114: (same)
5142-5143: (same)
5158-5159: (same)
5189-5190: (same)
5202-5203: (same)
5217-5218: (same)
5232-5233: (same)
5252-5253: (same)
5269-5270: (same)
5283-5284: (same)
5305-5306: (same)
5319-5320: (same)
5341-5342: (same)
5369-5370: (same)
5412-5413: (same)
5434-5435: (same)
5450-5451: (same)
5464-5465: (same)
5479-5480: (same)
5492-5493: (same)
5549-5550: (same)
5895-5896: (same)
```

### Game.cpp — Sound Pan Calculation
Pattern: `lPan = -(320 - (sX - m_sViewPointX))*1000;` or `lPan = ((sX - m_sViewPointX)-320)*30;`
```
5009: lPan = -(320 - (sX - m_sViewPointX))*1000;
5024: lPan = -(320 - (sX - m_sViewPointX))*1000;
5147: lPan = -(320 - (sX - m_sViewPointX))*1000;
5163: lPan = -(320 - (sX - m_sViewPointX))*1000;
5207: lPan = -(320 - (sX - m_sViewPointX))*1000;
5222: lPan = -(320 - (sX - m_sViewPointX))*1000;
5237: lPan = ((sX - m_sViewPointX)-320)*30;
5257: lPan = ((sX - m_sViewPointX)-320)*30;
5274: lPan = ((sX - m_sViewPointX)-320)*30;
5288: lPan = ((sX - m_sViewPointX)-320)*30;
5310: lPan = ((sX - m_sViewPointX)-320)*30;
5324: lPan = ((sX - m_sViewPointX)-320)*30;
5346: lPan = ((sX - m_sViewPointX)-320)*30;
5497: lPan = ((sX - m_sViewPointX)-320)*30;
```

### Game.cpp — Weather Effect Spawn Centers
```
14405: bAddNewEffect(201, (rand() % 160) + 320, (rand() % 120) + 240, ...);
14406: bAddNewEffect(202, (rand() % 160) + 320, (rand() % 120) + 240, ...);
14420: bAddNewEffect(202, (rand() % 160) + 320, (rand() % 120) + 240, ...);
```

---

## 5. Screen Boundary Constants (`639`, `479`)

### Game.cpp — Tooltip/Name Clamping (~30 occurrences)
Pattern: `if ((tX + 235) > 639) tX = 639 - 235;` / `if ((tY + 100) > 479) tY = 479 - 100;`
```
4086-4088, 4107-4109, 4129-4131: (character name clamping)
26976-26978, 26996-26998, 27016-27018: (in-game name clamping)
27976-27978, 27991-27993, 28008-28010: (attack target clamping)
28027-28029, 28046-28048, 28063-28065: (more clamping)
28079-28081, 28095-28097, 28110-28112, 28126-28128: (more clamping)
```

### Game.cpp — DrawShadowBox Full-Screen (~15 occurrences)
```
21649: m_DDraw.DrawShadowBox(0,0,639,479);
23843-23847: m_DDraw.DrawShadowBox(0,0,639,479);  (x3)
24149: m_DDraw.DrawShadowBox(0,0,639,479);
24202-24205: m_DDraw.DrawShadowBox(0,0,639,479);  (x3)
24370: m_DDraw.DrawShadowBox(0,0,639,479);
25272: m_DDraw.DrawShadowBox(0,0,639,479);
27325-27326: m_DDraw.DrawShadowBox(0,0,639,479);  (x2)
```

### Game.cpp — Top Message Bar
```
17427: m_DDraw.DrawShadowBox(0, 0, 639, 30);
17430: PutAlignedString(0, 639, 10, m_cTopMsg, 255,255,255);
```

### Game.cpp — Observer Mode Panning (~6 occurrences)
```
27476: if ((msX == 639) && (msY == 0) && (...)) bSendCommand(MSGID_REQUEST_PANNING, NULL, 2, ...);
27478: if ((msX == 639) && (msY == 479) && (...)) bSendCommand(MSGID_REQUEST_PANNING, NULL, 4, ...);
27480: if ((msX == 0) && (msY == 479) && (...)) bSendCommand(MSGID_REQUEST_PANNING, NULL, 6, ...);
27484: if ((msX == 639) && (...)) bSendCommand(MSGID_REQUEST_PANNING, NULL, 3, ...);
27488: if ((msY == 479) && (...)) bSendCommand(MSGID_REQUEST_PANNING, NULL, 5, ...);
```

### Game.cpp — Dialog Positioning
```
27617: if( msX < 320 ) m_stDialogBoxInfo[9].sX = 0;
27620: else m_stDialogBoxInfo[9].sY = 427 - m_stDialogBoxInfo[9].sSizeY;
```

### Game.cpp — Bottom Bar
```
27166: m_DDraw.DrawShadowBox(0, 413, 639, 429);
```

### DXC_ddraw.cpp — DD Windowed Mode
```
145: SetWindowPos(hWnd, HWND_TOP, cx-320, cy-240, 640, 480, SWP_SHOWWINDOW);
154: SetRect(&m_rcFlipping, cx-320, cy-240, cx+320, cy+240);
343: SetWindowPos(hWnd, NULL, cx-320, cy-240, 640, 480, SWP_SHOWWINDOW);
354: SetRect(&m_rcFlipping, cx-320, cy-240, cx+320, cy+240);
674: if ((sX < 0) || (sY < 0) || (sX > 639) || (sY > 479)) return;
```

### Sprite.cpp — DD Path Clipping
```
799: if( iSangX >= 0 && iSangX < 640 && iSangY >= 0 && iSangY < 427 )
817: if( iSangX >= 0 && iSangX < 640 && iSangY >= 0 && iSangY < 427 )
```

---

## 6. Playable Area Bottom (`427`)

`427 = 480 - 53` (53 = bottom UI bar height). Already partially migrated to `PLAYABLE_H = 907`.

### Game.cpp — Still Hardcoded
```
13634: for (iy = -sModY; iy < 427+48 ; iy += 32)   // DD path tile loop
15345: if( sY > 427-128-20 ) sY = 427-128;           // Dialog clamping
24876: if( sY > 427-128-20 ) sY = 427-128;           // Dialog clamping
27166: m_DDraw.DrawShadowBox(0, 413, 639, 429);       // Bottom bar
27620: m_stDialogBoxInfo[9].sY = 427 - m_stDialogBoxInfo[9].sSizeY;
27674-27676: if (msY > 427) cLB = 0;                  // TEMPORARY click guard
```

### Game.cpp — Already Using PLAYABLE_H
```
2026: for (iy = -sModY-480; iy <= PLAYABLE_H+704; iy += 32)   // FIXED
2031: if (...(iy >= -sModY) && (iy <= PLAYABLE_H+32+16))       // FIXED
13616: for (iy = -sModY; iy < PLAYABLE_H + 48; iy += 32)      // FIXED
13655: for (iy = -sModY; iy < PLAYABLE_H+48 ; iy += 32)       // FIXED
```

### Sprite.cpp
```
799: iSangY < 427    // DD clipping, needs PLAYABLE_H
817: iSangY < 427    // DD clipping, needs PLAYABLE_H
```

---

## 7. Tile-Based Sound Center Constants (`+ 10`, `+ 7`)

These derive from the old viewport size in tiles: 640/32/2 = 10, 480/32/2 ~ 7-8.

### Game.cpp
```
4929: sAbsX = abs(((m_sViewPointX / 32) + 10) - dX);
4930: sAbsY = abs(((m_sViewPointY / 32) + 7) - dY);
4975-4976: (same)
4979: lPan = -(((m_sViewPointX / 32) + 10) - sX)*1000;
... (~30 occurrences)
```

### MapData.cpp
```
1817: sCenterX = (sVal / 32) + 10;
1819: sCenterY = (sVal / 32) + 8;
3843: sAbsX = abs(((m_pGame->m_sViewPointX / 32) + 10) - sX);
3844: sAbsY = abs(((m_pGame->m_sViewPointY / 32) + 7)  - sY);
```

Under Option 2 with TILE_SIZE=64: These `+ 10` and `+ 7` would change to `+ 10` and `+ 7` (unchanged, since VIRTUAL_W/TILE_SIZE/2 = 1280/64/2 = 10). This is a happy coincidence — the tile-based sound math stays the same!

Under Option 1: These should still change to `+ (VIRTUAL_HALF_W/32)` and `+ (VIRTUAL_HALF_H/32)` which equals `+ 20` and `+ 15`.

---

## 8. UI-Relative `240` References (NOT tile-related)

Many `240` references are dialog box coordinates (e.g., scrollbar X position, button Y position) that are relative to the dialog's sX/sY origin. These do NOT need changing for tile size — they're UI-internal offsets:

```
15124: if ((msX >= sX + 240) && (msX <= sX + 260) && ...  // Scrollbar hit test
17729: if ((msX >= sX + 35) && (msX <= sX + 240) && ...   // Dialog click area
31279: PutAlignedString(sX, sX+240, sY+20, cTxt, ...);    // Text alignment
32050: PutAlignedString(sX + 25, sX + 240, sY + 60, ...); // Text alignment
... (50+ occurrences — ALL are UI-internal, NOT resolution-dependent)
```

---

## 9. Login/Character Select Screen Constants

These screens use hardcoded pixel positions for the 640x480 layout:

```
23982: PutAlignedString(98, 357, 320 +15, ...);     // Character select text
24280: pMI->AddRect(370, 240, 370 + BTNSZX, ...);   // Button position
25138: pMI->AddRect(197, 320, 197 + BTNSZX, ...);   // Button position
25318: PutAlignedString(153, 487, 288, ...);         // Change password text
```

These will need updating when the login screens are repositioned for 1280x960.

---

## 10. DD Path Legacy Code (`640`, `480`, `640+32`)

### Game.cpp — DrawBackground DD Path
```
13632: SetRect(&m_DDraw.m_rcClipArea, 0,0, 640+32, 480+32);   // DD clip area
```

### Game.cpp — Weather Initial Position
```
19973: m_stWhetherObject[i].sY = (m_pMapData->m_sPivotY*32) + ((rand() % 800) - 600) + 240;
```

### Game.cpp — Effect Overlay Positioning
```
6458: m_pEffectSpr[101]->PutTransSprite_NoColorKey(320, 480, cTempFrame, dwTime);
```

---

## Option 1 vs Option 2 Impact Assessment

### Option 1 (Supersampling — RECOMMENDED)
Changes needed for Phase 2 UI repositioning:
- **Sound panning `320`/`240`**: ~100 lines (mechanical search-replace to `VIRTUAL_HALF_W`/`VIRTUAL_HALF_H`)
- **Screen bounds `639`/`479`**: ~70 lines (to `VIRTUAL_W-1`/`VIRTUAL_H-1`)
- **Playable bottom `427`**: ~10 lines (to `PLAYABLE_H`)
- **Observer panning**: ~6 lines
- **Login screens**: ~20 lines
- **DD legacy `640`/`480`**: ~5 lines
- **Total: ~210 lines, mostly mechanical replacements**
- **`*32` tile math: NO CHANGES**

### Option 2 (True 2x with TILE_SIZE=64)
All of the above PLUS:
- **`*32` tile-to-pixel**: ~200+ lines (to `*TILE_SIZE`)
- **`/32` pixel-to-tile**: ~40+ lines (to `/TILE_SIZE`)
- **`+= 32` loops**: ~12 lines (to `+= TILE_SIZE`)
- **`+= 32` movement**: ~16 lines (to `+= TILE_SIZE`)
- **Direction offsets**: 8 lines
- **Repack script**: brush dims * 2
- **spriteScale**: disable detection
- **Total: ~480+ lines across Game.cpp, MessageHandler.cpp, MapData.cpp, Sprite.cpp, and repack scripts**
- **Much higher risk of subtle positioning bugs**

---

## Files Not Containing Tile-Related Constants

These files were searched and contain NO tile-size or resolution references requiring changes:
- `Sources/Client/DirectX/GPURenderer.cpp` — uses virtual resolution params from init
- `Sources/Client/DirectX/SoundBuffer.cpp` — audio only
- `Sources/Client/DirectX/YWSound.cpp` — audio only
- `Sources/Client/DirectX/Effect.cpp` — uses pixel coords (set from tile*32 elsewhere)
- `Sources/Client/Net/Msg.cpp` — network protocol only
- `Sources/Client/SoundManager.cpp` — audio routing only
