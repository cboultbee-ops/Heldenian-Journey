# Palette/Color-Index Equipment Sprite System: Comprehensive Analysis

## Executive Summary

Helbreath equipment sprites are **NOT palette-indexed**. They are stored as standard **16-bit RGB (RGB565/RGB555) BMP images** inside PAK files, with no embedded palettes for runtime swapping. Equipment coloring is achieved through **real-time pixel-level additive color tinting** applied during rendering. This is a purely runtime operation -- the sprite asset files themselves are "base color" (typically gray/neutral tone) and the color is computed per-pixel at draw time via lookup tables.

This means **2x upscaled sprites will work correctly with the color system** with no loss of color-index properties, because there are no color indices to lose.

---

## 1. How Equipment Sprites Are Stored

### File Format
All equipment sprites (and all other sprites) are 16-bit BMP images packed into PAK archive files. The PAK format is:

- 20-byte reserved header
- 4-byte `iTotalimage` count
- Offset table (8 bytes per entry)
- Per-sprite blocks: 100-byte block header + frame count + brush data + BMP image data

**File**: `Sources/Client/DirectX/Sprite.cpp`, lines 24-78 (constructor)

The BMPs inside PAKs are standard Windows BMP format at 16-bit color depth (RGB565 with `BI_BITFIELDS` or RGB555 with `BI_RGB`). There are no palette entries used for color swapping. The `LoadToGPU()` function at line 2920+ explicitly handles:
- 1-bit (monochrome with 2-entry palette)
- 4-bit (16-color palette)
- 8-bit (256-color palette)
- 16-bit (RGB565/RGB555, no palette)
- 24-bit (RGB, no palette)
- 32-bit (BGRA)

Equipment sprites are 16-bit. They use a color-key transparency system where the top-left pixel defines the transparent color.

### PAK File Naming Convention

**File**: `Sources/Client/Game.cpp`, lines 3271-3550

| Prefix | Meaning | Examples |
|--------|---------|---------|
| `M*` | Male equipment | `MLArmor`, `MCMail`, `MSMail`, `MPMail`, `MShirt`, `MHauberk` |
| `W*` | Female equipment | `WLArmor`, `WCMail`, `WSMail`, `WPMail`, `WShirt`, `WHauberk` |
| `MH*` | Male hero equipment | `MHPMail1`, `MHRobe1`, `MHHelm1`, `MHLeggings1`, `MHHauberk1` |
| `WH*` | Female hero equipment | `WHPMail1`, `WHRobe1`, `WHHelm1`, `WHLeggings1`, `WHHauberk1` |
| `NM*` | New male helms | `NMHelm1`, `NMHelm2`, `NMHelm3`, `NMHelm4` |
| `NW*` | New female helms | `NWHelm1`, `NWHelm2`, `NWHelm3`, `NWHelm4` |
| `Mpt` | Male underwear (all skins) | 96 frames (8 skin types x 12 animation frames) |
| `Wpt` | Female underwear | Same structure |
| `Mhr` | Male hair | 96 frames (8 hair styles x 12 animation frames) |
| `Whr` | Female hair | Same structure |
| `Msw` | Male swords (combined) | 672 frames (12 weapon types x 56 frames each) |
| `Msh` | Male shields (combined) | 63 frames (9 shield types x 7 frames each) |

---

## 2. The Color Tinting System

### Overview

Equipment coloring works through a **three-layer system**:

1. **Server** assigns a 4-bit color index (0-15) per equipment slot, packed into a 32-bit `m_iApprColor` field
2. **Client** extracts each 4-bit color index and looks up RGB tint values from precomputed color tables
3. **Rendering** applies the tint via per-pixel additive math (DD path) or shader tinting (GPU path)

### Layer 1: Server-Side Color Assignment

**File**: `Sources/HG/HG.cpp`, lines 8785-8976

Each item has a `m_cItemColor` property (a single byte, values 0-15). When equipment is worn, the server packs each item's color into a specific 4-bit nibble of the 32-bit `m_iApprColor`:

```
m_iApprColor (32 bits):
  Bits 31-28: Weapon color    (EQUIPPOS_RHAND)
  Bits 27-24: Shield color    (EQUIPPOS_LHAND)
  Bits 23-20: Body armor color (EQUIPPOS_BODY)
  Bits 19-16: Mantle color    (EQUIPPOS_BACK)
  Bits 15-12: Arm armor color (EQUIPPOS_ARMS)
  Bits 11-8:  Pants color     (EQUIPPOS_PANTS)
  Bits 7-4:   Boots color     (EQUIPPOS_LEGGINGS)
  Bits 3-0:   Helm color      (EQUIPPOS_HEAD)
```

### Layer 2: Client-Side Color Tables

**File**: `Sources/Client/Game.h`, lines 928-929

```cpp
WORD m_wR[16], m_wG[16], m_wB[16];     // Armor/equipment colors
WORD m_wWR[16], m_wWG[16], m_wWB[16];  // Weapon colors
```

**File**: `Sources/Client/Game.cpp`, lines 823-850

These tables are initialized at startup by converting RGB values to the display's 16-bit pixel format (5-bit or 6-bit channel values):

**Armor Color Table (`m_wR/m_wG/m_wB`)**:

| Index | Color Name | RGB Source |
|-------|-----------|------------|
| 0 | Base/Neutral | (100, 100, 100) |
| 1 | Indigo Blue | (40, 40, 96) |
| 2 | Custom-Weapon | (79, 79, 62) |
| 3 | Gold | (135, 104, 30) |
| 4 | Crimson | (127, 18, 0) |
| 5 | Green | (10, 60, 10) |
| 6 | Gray | (40, 40, 40) |
| 7 | Aqua | (47, 79, 80) |
| 8 | Pink | (127, 52, 90) |
| 9 | (unnamed) | (90, 60, 90) |
| 10 | Blue | (0, 35, 60) |
| 11 | Tan | (105, 90, 70) |
| 12 | Khaki | (94, 91, 53) |
| 13 | Yellow | (85, 85, 8) |
| 14 | Red | (75, 10, 10) |
| 15 | Black | (12, 20, 30) |

**Weapon Color Table (`m_wWR/m_wWG/m_wWB`)**:

| Index | Color Name | RGB Source |
|-------|-----------|------------|
| 1-3 | Light-blue | (70, 70, 80) |
| 4 | Green | (70, 100, 70) |
| 5 | Critical | (130, 90, 10) |
| 6 | Heavy-blue | (42, 53, 111) |
| 7 | White | (145, 145, 145) |
| 8 | (unnamed) | (120, 100, 120) |
| 9 | Heavy-Red | (75, 10, 10) |
| 10 | Gold | (135, 104, 30) |

### Layer 3: Runtime Rendering

**File**: `Sources/Client/Game.cpp`, lines 7683-7690 (color extraction)

```cpp
iWeaponColor = (_tmp_iApprColor & 0xF0000000) >> 28;
iShieldColor = (_tmp_iApprColor & 0x0F000000) >> 24;
iArmorColor  = (_tmp_iApprColor & 0x00F00000) >> 20;
iMantleColor = (_tmp_iApprColor & 0x000F0000) >> 16;
iArmColor    = (_tmp_iApprColor & 0x0000F000) >> 12;
iPantsColor  = (_tmp_iApprColor & 0x00000F00) >> 8;
iBootsColor  = (_tmp_iApprColor & 0x000000F0) >> 4;
iHelmColor   = (_tmp_iApprColor & 0x0000000F);
```

For each equipment piece, if the color index is 0, the sprite is drawn with `PutSpriteFast()` (no tinting). If non-zero, it's drawn with `PutSpriteRGB()` using a **differential tint**:

**File**: `Sources/Client/Game.cpp`, lines 7896-7899

```cpp
if (iWeaponColor == 0)
    m_pSprite[iWeaponIndex]->PutSpriteFast(sX, sY, _tmp_cFrame, dwTime);
else
    m_pSprite[iWeaponIndex]->PutSpriteRGB(sX, sY, _tmp_cFrame,
        m_wWR[iWeaponColor] - m_wR[0],   // red delta
        m_wWG[iWeaponColor] - m_wG[0],   // green delta
        m_wWB[iWeaponColor] - m_wB[0],   // blue delta
        dwTime);
```

The formula is: `colorDelta = targetColor[index] - baseColor[0]`

The base color (`m_wR[0]` etc.) is the "neutral" gray. The tint is the signed difference between the desired color and this neutral. Positive values brighten/shift toward that color; negative values darken.

---

## 3. The PutSpriteRGB Tinting Algorithm

### DirectDraw Path (Software Rendering)

**File**: `Sources/Client/DirectX/Sprite.cpp`, lines 3045-3210

The DD path uses precomputed lookup tables (`G_iAddTable31[64][510]` and `G_iAddTable63[64][510]`) for fast per-pixel color addition with clamping.

**Table initialization** at `Sources/Client/UI/Wmain.cpp`, lines 584-593:

```cpp
for (iX = 0; iX < 64; iX++)
for (iY = 0; iY < 510; iY++) {
    iSum = iX + (iY - 255);
    if (iSum <= 0)  iSum = 1;
    if (iSum >= 31) iSum = 31;
    G_iAddTable31[iX][iY] = iSum;
    // ...same for 63-value table
}
```

The table maps `(channelValue, tintValue+255)` to a clamped result. The `+255` offset allows negative tint values (darkening).

**Per-pixel math** (RGB565 format, line 3180):

```cpp
pDst[ix] = (WORD)(
    (G_iAddTable31[(pSrc[ix] & 0xF800) >> 11][iRedPlus255] << 11) |
    (G_iAddTable63[(pSrc[ix] & 0x7E0) >> 5][iGreenPlus255] << 5) |
    G_iAddTable31[(pSrc[ix] & 0x1F)][iBluePlus255]
);
```

This extracts each 5-bit (or 6-bit) color channel from the source pixel, looks up the clamped sum with the tint delta in the precomputed table, and reassembles the 16-bit pixel. **This is a per-pixel additive operation, not a palette swap.**

### GPU Path (Shader Rendering)

**File**: `Sources/Client/DirectX/Sprite.cpp`, lines 3115-3148

```cpp
float colorR = sRed / rMax;
float colorG = sGreen / gMax;
float colorB = sBlue / bMax;
m_pDDraw->m_pGPURenderer->QueueSprite(
    m_glTextureID, dX, dY, sx, sy, szx, szy,
    m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
    BLEND_TINTED, 1.0f, colorR, colorG, colorB);
```

The GPU path normalizes the 5/6-bit tint values to [0,1] float range and passes them as `uColorTint` to the fragment shader.

**Shader code** at `Sources/Client/DirectX/GPURenderer.cpp`, lines 98-103:

```glsl
else if (uBlendMode == 9) {
    // Tinted replacement: add tint to texture RGB, draw with standard alpha blend
    vec3 tinted = clamp(texColor.rgb + uColorTint, 0.0, 1.0);
    FragColor = vec4(tinted, texColor.a) * Color;
}
```

The shader adds the color tint to each texel's RGB and clamps to [0,1]. This exactly replicates the DD table lookup behavior.

---

## 4. Hair Color System

**File**: `Sources/Client/Game.cpp`, lines 4465-4500

Hair uses the same `PutSpriteRGB` mechanism but with a dedicated color mapping function:

```cpp
void CGame::_GetHairColorRGB(int iColorType, int * pR, int * pG, int * pB) {
    switch (iColorType) {
    case 0: *pR = 14; *pG = -5; *pB = -5; break;  // Dark red
    case 1: *pR = 20; *pG = 0;  *pB = 0;  break;  // Orange
    case 2: *pR = 22; *pG = 13; *pB = -10; break; // Light brown
    case 3: *pR = 0;  *pG = 10; *pB = 0;  break;  // Green
    case 4: *pR = 0;  *pG = 0;  *pB = 22; break;  // Bright blue
    case 5: *pR = -5; *pG = -5; *pB = 15; break;  // Dark blue
    case 6: *pR = 15; *pG = -5; *pB = 16; break;  // Mauve
    case 7: *pR = -6; *pG = -6; *pB = -6; break;  // Black
    // ... cases 8-15 with more colors
    }
}
```

The 4-bit hair color type is extracted from `_tmp_sAppr1` bits 4-7: `((_tmp_sAppr1 & 0x00F0) >> 4)`.

Usage at line 7935:
```cpp
if ((iHairIndex != -1) && (iHelmIndex == -1)) {
    _GetHairColorRGB(((_tmp_sAppr1 & 0x00F0) >> 4), &iR, &iG, &iB);
    m_pSprite[iHairIndex]->PutSpriteRGB(sX, sY, frame, iR, iG, iB, dwTime);
}
```

Hair tint values are raw 5-bit signed deltas (range approximately -31 to +31), passed directly to `PutSpriteRGB` without subtracting a base color.

---

## 5. Weapon Glare Effects

**File**: `Sources/Client/Game.cpp`, lines 36954-36963

Special DK (Dark Knight) weapons have glare effects applied via `PutTransSpriteRGB`:

```cpp
void CGame::DKGlare(int iWeaponColor, int iWeaponIndex, int *iWeaponGlare) {
    if (iWeaponColor != 9) return;  // Only color index 9 (Heavy-Red)
    // DK sword3 -> blue glare (3)
    // DK staff3 -> green glare (2)
}
```

The glare is drawn as an additive overlay after the base tinted sprite:
```cpp
case 1: PutTransSpriteRGB(sX, sY, frame, m_iDrawFlag, 0, 0, dwTime); // Red
case 2: PutTransSpriteRGB(sX, sY, frame, 0, m_iDrawFlag, 0, dwTime); // Green
case 3: PutTransSpriteRGB(sX, sY, frame, 0, 0, m_iDrawFlag, dwTime); // Blue
```

---

## 6. Complete Equipment Drawing Order

**File**: `Sources/Client/Game.cpp`, lines 7894-8000 (DrawObject_OnAttack, `_cDrawingOrder[_tmp_cDir] == 1` branch)

The layered draw order for a fully-equipped character is:

1. Shadow sprite
2. Active aura effect
3. **Weapon** (with `PutSpriteRGB` if colored, then glare overlay)
4. **Body sprite** (character model - Bm/Wm/Ym for male, Bw/Ww/Yw for female)
5. **Mantle** (if `_cMantleDrawingOrder == 0`)
6. **Underwear** (always `PutSpriteFast`, no color tinting)
7. **Hair** (with `PutSpriteRGB` for hair color, only if no helm equipped)
8. **Boots** (if skirt draw order requires it)
9. **Pants** (with color tinting)
10. **Arm armor** (with color tinting)
11. **Boots** (normal order)
12. **Body armor** (with color tinting)
13. **Helm** (with color tinting)
14. **Mantle** (if `_cMantleDrawingOrder == 2`)
15. **Shield** (with color tinting + glare)
16. **Mantle** (if `_cMantleDrawingOrder == 1`)

---

## 7. SPRID Index Mapping

**File**: `Sources/Client/DirectX/SpriteID.h`

```
Male (M*)                      Female (W*)
SPRID_UNDIES_M     = 1400      SPRID_UNDIES_W     = 11400
SPRID_HAIR_M       = 1600      SPRID_HAIR_W       = 11600
SPRID_BODYARMOR_M  = 1800      SPRID_BODYARMOR_W  = 11800
SPRID_BERK_M       = 2100      SPRID_BERK_W       = 12100   (arm armor)
SPRID_LEGG_M       = 2300      SPRID_LEGG_W       = 12300   (pants)
SPRID_BOOT_M       = 2500      SPRID_BOOT_W       = 12500
SPRID_MANTLE_M     = 2600      SPRID_MANTLE_W     = 12600
SPRID_HEAD_M       = 2800      SPRID_HEAD_W       = 12800   (helms)
SPRID_WEAPON_M     = 3000      SPRID_WEAPON_W     = 13000
SPRID_SHIELD_M     = 6500      SPRID_SHIELD_W     = 16500
```

Each equipment type has slots for multiple item variants. For example, body armor occupies indices `SPRID_BODYARMOR_M + apprValue * 15`, giving 15 sprite slots per armor type (12 animation frames for 8 directions, plus extras).

---

## 8. Implications for 2x Upscaling

### Will upscaled sprites lose their color-index properties?

**No.** Equipment sprites have no color-index properties to lose. The coloring is a runtime additive operation applied per-pixel. The sprites themselves are neutral-toned full-RGB images. When upscaled:

1. **The base sprite gets upscaled** from 1x to 2x resolution (more detail, same neutral base color)
2. **At runtime, the same `PutSpriteRGB` tinting applies** -- each pixel gets the same additive R/G/B delta
3. **The math is resolution-independent** -- it operates per-pixel regardless of sprite size

### Specific considerations:

**GPU Path (BLEND_TINTED shader)**: The shader operates on normalized [0,1] texture colors. A 2x upscaled sprite loaded into a 2x-sized texture with `m_iSpriteScale = 2` will have identical color values per texel. The `clamp(texColor.rgb + uColorTint, 0.0, 1.0)` math works identically. **No changes needed.**

**DD Path (G_iAddTable lookup)**: This path operates on 16-bit pixel values from the DD surface. Upscaled sprites packed back as 16-bit BMPs will have the same per-pixel channel values (possibly with better gradient detail from upscaling). The lookup table math is unchanged. **No changes needed.**

**Auto-scale detection** (`Sources/Client/DirectX/Sprite.cpp`, lines 3008-3018): The `m_iSpriteScale` field is auto-detected by comparing bitmap width to brush coordinates. If the PAK has 2x brush coordinates AND 2x bitmap size, `m_iSpriteScale` will be 1 (no scaling). If the PAK has original 1x brush coordinates but 2x bitmap, the ratio will be 2 and `m_iSpriteScale = 2`. The repack script supports both modes via `upscale_factor` parameter.

### Potential subtle difference:

The original sprites at 16-bit color depth have only 32 or 64 levels per channel. The additive tint operates in this quantized space (5-bit R, 6-bit G, 5-bit B). When sprites are upscaled to 2x as PNG/24-bit/32-bit, they have 256 levels per channel. The GPU shader path already works in full float precision, so this is a non-issue for the GPU renderer. The DD fallback path would need the sprite loaded into a 16-bit DD surface, which happens automatically via `_pMakeSpriteSurface()`.

**For the Gigapixel upscaling pipeline**: The upscaler operates on the full-RGB PNG exports from `colorkey_alpha.py`. It does not know or care about the tinting system. The upscaler will produce a higher-quality neutral-tone base sprite, and the runtime tinting will work exactly as before, just at 2x resolution with more detail.

### Summary

| Aspect | Impact of 2x Upscale | Action Required |
|--------|----------------------|-----------------|
| Sprite storage format | None - stays as BMP/PNG in PAK | None |
| Color tinting math | Per-pixel additive, resolution-independent | None |
| GPU BLEND_TINTED shader | Works identically on higher-res texture | None |
| DD software path | Works identically on 16-bit surface | None |
| Hair color tinting | Same `PutSpriteRGB` mechanism | None |
| Weapon glare effects | Same `PutTransSpriteRGB` overlay | None |
| Color tables (m_wR, m_wWR) | Unrelated to sprite resolution | None |
| Server-side m_iApprColor | Unrelated to sprite resolution | None |
| Brush coordinates | Must be scaled 2x in PAK (handled by repack script) | Already supported |
| `m_iSpriteScale` auto-detect | Will correctly detect 2x scale | None |

---

## 9. Key File References Summary

| File | Lines | Content |
|------|-------|---------|
| `Sources/Client/DirectX/SpriteID.h` | 62-85 | SPRID_* constants for all equipment types |
| `Sources/Client/Game.h` | 928-929 | Color table declarations (`m_wR[16]`, `m_wWR[16]`) |
| `Sources/Client/Game.cpp` | 823-850 | Color table initialization (RGB to pixel-format) |
| `Sources/Client/Game.cpp` | 3271-3550 | Equipment PAK loading (`MakeSprite` calls) |
| `Sources/Client/Game.cpp` | 4465-4500 | `_GetHairColorRGB()` -- hair color lookup |
| `Sources/Client/Game.cpp` | 7663-8000 | `DrawObject_OnAttack()` -- equipment drawing with tinting |
| `Sources/Client/Game.cpp` | 36954-36963 | `DKGlare()` -- weapon glare effect logic |
| `Sources/Client/DirectX/Sprite.cpp` | 3045-3210 | `PutSpriteRGB()` -- DD tinting with lookup tables |
| `Sources/Client/DirectX/Sprite.cpp` | 3115-3148 | `PutSpriteRGB()` -- GPU tinting path (BLEND_TINTED) |
| `Sources/Client/DirectX/Sprite.cpp` | 3213-3360 | `PutTransSpriteRGB()` -- additive tinting (glare effects) |
| `Sources/Client/DirectX/GPURenderer.cpp` | 98-103 | Fragment shader BLEND_TINTED implementation |
| `Sources/Client/UI/Wmain.cpp` | 584-600 | `G_iAddTable31/63` initialization |
| `Sources/Client/Misc.cpp` | 180-214 | `ColorTransfer()` -- RGB to pixel-format conversion |
| `Sources/HG/HG.cpp` | 8785-8976 | Server equip handler -- `m_iApprColor` packing |
| `asset_pipeline/pak_repack.py` | 84-94 | PAK repack with brush scaling support |
