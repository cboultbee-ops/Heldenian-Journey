# Helbreath International All-In-One

## Project Overview
Helbreath game client + server (HG) + login server. The client has dual rendering: original DirectDraw 7 (16-bit, 640x480) and GPU-accelerated OpenGL 3.3 (sprite batching, 2x upscale to 1280x960).

## Build

All three projects (Client, HG, Login) via the all-in-one solution:
```
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\All In One.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

Client only:
```
powershell.exe -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Sources\Client\Client.sln' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal"
```

- **Always build Release** (`Play_Me.exe`) unless explicitly asked for Debug
- Use `powershell.exe -Command` — `cmd.exe /c` fails with spaces in paths
- Output: `Client\Play_Me.exe` (Release), `Client\Client_d.exe` (Debug)
- HG output: `HGserver.exe` (external path in vcxproj)
- Login output: `Login\Login.exe`
- SAFESEH:NO required for old DirectX libs (DDRAW, DINPUT, DXGUID)
- Link: opengl32.lib, glew32.lib, glu32.lib (XAudio2 auto-links via SDK header)
- Expect C4244 warnings in Sprite.cpp — pre-existing, harmless
- PDB lock error (C1041) on parallel builds: use `/t:Rebuild` to clear stale locks

## Solutions & Projects
| Solution | Path | Contains |
|----------|------|----------|
| All In One | `Sources/All In One.sln` | Client + HG + Login |
| Client | `Sources/Client/Client.sln` | Client only |
| HG Server | `Sources/HG/HGserver.sln` | Game server only |

## Architecture

### Client Key Files
| File | Purpose |
|------|---------|
| `Sources/Client/Game.h/.cpp` | Main game class (~39K lines) |
| `Sources/Client/MessageHandler.h/.cpp` | Network message handlers (extracted from CGame, ~6.3K lines) |
| `Sources/Client/SoundManager.h/.cpp` | Sound subsystem (extracted from CGame) |
| `Sources/Client/DirectX/GPURenderer.h/.cpp` | OpenGL 3.3 renderer |
| `Sources/Client/DirectX/ShaderManager.h/.cpp` | GLSL shader management |
| `Sources/Client/DirectX/DXC_ddraw.h/.cpp` | DirectDraw 7 + GPU integration |
| `Sources/Client/DirectX/DXC_dinput.h/.cpp` | Mouse input (Raw Input API) |
| `Sources/Client/DirectX/Sprite.h/.cpp` | Sprite loading, DD blitting, GPU upload |
| `Sources/Client/DirectX/YWSound.h/.cpp` | XAudio2 device + mastering voice |
| `Sources/Client/DirectX/SoundBuffer.h/.cpp` | WAV buffer management (XAudio2 source voices) |
| `Sources/Client/DirectX/Effect.h/.cpp` | Visual effect objects |
| `Sources/Client/UI/Wmain.cpp` | Win32 window, WndProc, message loop |
| `Sources/Client/Map/MapData.h/.cpp` | Map tile data, chat message storage |
| `Sources/Client/Net/Msg.h/.cpp` | Network message handling |

### Rendering
- `DXC_ddraw` manages both DirectDraw and GPU renderer (`m_bUseGPU` flag)
- `CGPURenderer` handles OpenGL context, sprite batching, font atlas (512x512 RGBA)
- GPU init happens in `DXC_ddraw::bInit()` BEFORE DirectDraw display setup
- DirectDraw off-screen surfaces still created for pixel operations compatibility

### DD Blending → GPU Shader Mapping
| DD Function | DD Math | GPU Blend Mode |
|-------------|---------|----------------|
| `PutTransSprite` | `_CalcMaxValue` (additive) | BLEND_ADDITIVE (2) |
| `PutTransSprite50/70/25` | `_CalcMaxValue` (partial) | BLEND_ADDITIVE |
| `PutRevTransSprite` | `_CalcMinValue` (subtractive) | BLEND_SUBTRACTIVE (10) |
| `PutTransSpriteRGB` | additive + flat tint | BLEND_ADDITIVE + color |
| `PutColouredSprite` | replace dst with tinted src | BLEND_TINTED (9) |
| `PutShadowSprite` | darken dst | BLEND_SHADOW |
| `PutSpriteFastNoColorKey` | DDBLTFAST_NOCOLORKEY | BLEND_OPAQUE |
| `DrawLine`/`DrawLine2` | N/A | GL_LINES + additive |

- BLEND_ADDITIVE: `max(R,G,B)` as alpha — dark pixels smoothly fade (matches DD math)
- BLEND_SUBTRACTIVE: `GL_FUNC_REVERSE_SUBTRACT` — `result = dst - src`

### Mouse Input (Raw Input API)
- `DXC_dinput` uses `RegisterRawInputDevices()` + `WM_INPUT` (replaced DirectInput 7)
- `OnRawInput(LPARAM)` accumulates RAWMOUSE deltas, tracks button state (0x80/0 convention)
- `SetAcquire()` uses `ClipCursor()` + `ShowCursor()` for exclusive capture
- `UpdateMouseState()` API unchanged — all ~25 call sites in Game.cpp untouched

### Sound System (CSoundManager + XAudio2)
- `CSoundManager` owns `YWSound`, sound containers, BGM, volume/stat flags
- Sound containers: `std::unordered_map<int, CSoundBuffer*>` for Combat/Monster/Effect
- Method named `PlaySfx()` — NOT `PlaySound()` (Windows `#define PlaySound PlaySoundA` conflict)
- CGame holds `CSoundManager m_SoundMgr` + reference aliases for backward compat
- BGM location→filename mapping lives in `CSoundManager::StartBGM()`
- WAV validation: RIFF/WAVE magic, field sanity checks, 50MB size cap
- `YWSound` wraps `IXAudio2` + `IXAudio2MasteringVoice` (replaced DirectSound)
- `CSoundBuffer` wraps `IXAudio2SourceVoice` — heap-based WAV data, lazy loading
- Volume: DS dB (-10000..0) → XAudio2 linear (0..1) via `powf(10, dB/2000)`
- Pan: `SetOutputMatrix()` with stereo L/R channel gains

### Bank List
- `m_pBankList` is `std::vector<CItem*>` (was fixed array of MAXBANKITEMS=120)
- Dense fill: `push_back` to add, `erase()` to remove (replaces manual shift-down loops)
- `_iGetBankItemCount()` returns `(int)m_pBankList.size()`

### Message Handler System (CMessageHandler)
- `CMessageHandler` is a `friend` of CGame with `CGame* m_pGame` back-pointer
- Contains 102 handler functions (~6.3K lines) extracted from CGame
- Includes: `GameRecvMsgHandler`, `NotifyMsgHandler`, 78 `NotifyMsg_*` handlers, and 22 other handlers
- CGame dispatches via `m_MsgHandler.GameRecvMsgHandler(dwMsgSize, Data)`
- Init: `m_MsgHandler.Init(this)` in CGame constructor
- Handler-to-handler calls within CMessageHandler use `this->` (implicit)
- CGame methods called from handlers use `m_pGame->` prefix
- CGame nested types require `CGame::` qualification (e.g. `CGame::partyMember`, `CGame::partyIterator`)
- CGame members without `m_` prefix use `m_pGame->` (e.g. `m_pGame->friendsList`, `m_pGame->G_cTxt`)
- Global variables (`G_cSpriteAlphaDegree`, `G_cCmdLineTokenA`) declared `extern` in MessageHandler.cpp

### Frame Timing
- `UpdateScreen()` uses `QueryPerformanceCounter` for sub-ms frame delta
- Sleep strategy: bulk sleep when >3ms remaining, spin-wait on QPC for final precision
- `G_dwGlobalTime` still `timeGetTime()` (DWORD ms) — game logic timing unchanged
- `timeBeginPeriod(1)` and `timeSetEvent()` 1-second timer preserved

### Effect System
- `m_pEffectList` is `std::vector<CEffect*>` (was fixed array of 300)
- Hybrid slot-reuse: scans for NULL first, `push_back` if none found, capped at MAXEFFECTS

## Important Patterns & Gotchas

### Lifecycle / Shutdown
- `DXC_ddraw::~DXC_ddraw` calls `ShutdownGPURenderer()` → deletes GPU renderer
- CSprite destructors run AFTER GPU renderer is gone → `m_pDDraw` dangling
- `UnloadFromGPU` must NOT call `glDeleteTextures` — `wglDeleteContext` frees all GL textures

### BMP / Sprite Loading
- BMP bottom-up: `pSrc[0]` = bottom-left, NOT top-left. Color key reads from last row.
- DD NoColorKey functions use `DDBLTFAST_NOCOLORKEY` — GPU must use BLEND_OPAQUE
- `LoadToGPU`: preserve RGB for color-key pixels (alpha=0 only) so BLEND_OPAQUE works
- 16-bit BMPs: BI_RGB=RGB555, BI_BITFIELDS=check masks (may be RGB565)
- DD pixel format: `m_cPixelFormat` 1=RGB565, 2=RGB555, 3=BGR565

### Windows API Conflicts
- `#define PlaySound PlaySoundA` silently renames methods — use `PlaySfx` instead
- `SetWindowPos` in DD windowed mode overwrites the 1280x960 GPU window size

### Reference Aliases for Backward Compat
CGame uses reference members to avoid mass-renaming when extracting subsystems:
```cpp
BOOL & m_bSoundStat  = m_SoundMgr.m_bSoundStat;
char & m_cSoundVolume = m_SoundMgr.m_cSoundVolume;
```

### Arrays Left as Fixed (Intentional)
These were evaluated for conversion but left as-is:
- `m_pChatMsgList[500]` — indices stored in MapData tiles (external dependency)
- `m_pItemList[50]` — coupled with `m_cItemOrder`, `m_bIsItemEquipped`, `m_bIsItemDisabled`
- `m_pSprite[25000]` — uses SPRID_* constants as direct indices (lookup table)
- `m_tile[550][550]` — optimal spatial grid
- `m_stDialogBoxInfo[61]` — fixed count tied to dialog enum

## Refactoring History
All 13 fixes complete and verified:
1. `gethostbyname` → `inet_addr` + fallback
2. Buffer overflow protection (`strncpy`)
3. Mouse clamping bounds (`m_sMaxX`/`m_sMaxY`)
4. Cached file logging (HG + Login)
5. Dialog box enum constants
6. DirectInput 7 → Raw Input API
7. `m_pEffectList` → `std::vector`
8. `timeGetTime()` → `QueryPerformanceCounter`
9. Sound arrays → `unordered_map` + WAV validation
10. Sound manager extraction (`CSoundManager`)
11. `m_pBankList[120]` → `std::vector` (dense fill, erase replaces shift-down)
12. DirectSound → XAudio2 (`YWSound`, `CSoundBuffer` rewritten)
13. Message handler extraction (`CMessageHandler` — 102 functions, ~6.3K lines from CGame)

## Asset Pipeline

### Gigapixel Output & Backup
- `gigapixel_output/` — working copy of 2x upscaled sprites (defringe runs here)
- `gigapixel_output_backup/` — **PRISTINE Gigapixel output. NEVER modify this folder.** This is the clean 2x upscale before any defringe/post-processing. If defringe damages sprites, restore from this backup.
- `gigapixel_input/` — original 1x sprites extracted from PAKs (also never modify)
- Workflow: `gigapixel_input` → Topaz Gigapixel 2x → `gigapixel_output` → `defringe_v4.py` → repack PAKs

### Color-Key Alpha Extraction
- `colorkey_alpha.py`: quantize RGB to RGB565, exact 16-bit match (not tolerance)
- Edge-padding prevents bilinear fringe on transparent pixels
- Config: `dd_surface_format` — rgb565 (default), rgb555, bgr565

### CCSR Upscaling
- CCSRv2 uses separate venv (`venv_ccsr/`)
- Both `--backend esrgan` and `--backend ccsr` produce correct 4x RGBA output
- RTX 2070 Super 8GB, ~10s/sprite, peak VRAM ~6.2GB with tiled VAE

### Per-Sprite Prompt Guidance
- `generate_prompt_map.py` → `prompt_map.yaml` with per-folder prompts
- Defaults: `steps=6, guidance=5.0, t_max=0.6, conditioning_scale=1.0`
