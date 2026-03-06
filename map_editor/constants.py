"""
Helbreath Map Editor — Constants and sprite registry.
Derived from Sources/Client/Game.cpp MakeTileSpr calls (lines 3018-3068).
"""
import numpy as np

# Tile size in pixels (original game)
TILE_SIZE = 32

# Attribute flags (byte 8 of tile data)
ATTR_BLOCKED  = 0x80
ATTR_TELEPORT = 0x40
ATTR_FARM     = 0x20
ATTR_BUILD    = 0x10
ATTR_NOATTACK = 0x04

ATTR_NAMES = {
    ATTR_BLOCKED:  "Blocked",
    ATTR_TELEPORT: "Teleport",
    ATTR_FARM:     "Farm",
    ATTR_BUILD:    "Build",
    ATTR_NOATTACK: "No-Attack",
}

# Attribute overlay colors (R, G, B, A)
ATTR_COLORS = {
    ATTR_BLOCKED:  (255, 0,   0,   120),
    ATTR_TELEPORT: (0,   80,  255, 120),
    ATTR_FARM:     (0,   200, 0,   120),
    ATTR_BUILD:    (255, 255, 0,   100),
    ATTR_NOATTACK: (255, 140, 0,   120),
}

# Tool modes
TOOL_PAINT_GROUND = 0
TOOL_PAINT_OBJECT = 1
TOOL_ERASE_OBJECT = 2
TOOL_EYEDROPPER   = 3
TOOL_FILL         = 4
TOOL_SELECT       = 5
TOOL_ATTRIBUTE    = 6
TOOL_WARP         = 7

TOOL_NAMES = {
    TOOL_PAINT_GROUND: "Paint Ground",
    TOOL_PAINT_OBJECT: "Paint Object",
    TOOL_ERASE_OBJECT: "Erase Object",
    TOOL_EYEDROPPER:   "Eyedropper",
    TOOL_FILL:         "Fill",
    TOOL_SELECT:       "Select",
    TOOL_ATTRIBUTE:    "Attribute",
    TOOL_WARP:         "Warp Edit",
}

TOOL_KEYS = {
    'g': TOOL_PAINT_GROUND,
    'o': TOOL_PAINT_OBJECT,
    'e': TOOL_ERASE_OBJECT,
    'i': TOOL_EYEDROPPER,
    'f': TOOL_FILL,
    's': TOOL_SELECT,
    'a': TOOL_ATTRIBUTE,
    'w': TOOL_WARP,
}

# Sprite ID → PAK mapping: (start_id, count, pak_name, category)
# From Game.cpp MakeTileSpr sequence
SPRITE_REGISTRY = [
    (0,   32, "maptiles1",       "ground"),
    (70,  27, "Sinside1",        "dungeon"),
    (100, 46, "Trees1",          "tree"),
    (150, 46, "TreeShadows",     "shadow"),
    (200, 10, "objects1",        "object"),
    (211,  5, "objects2",        "object"),
    (216,  4, "objects3",        "object"),
    (220,  2, "objects4",        "object"),
    (223,  3, "Tile223-225",     "object"),
    (226,  4, "Tile226-229",     "object"),
    (230,  9, "objects5",        "object"),
    (238,  4, "objects6",        "object"),
    (242,  7, "objects7",        "object"),
    (300, 15, "maptiles2",       "ground"),
    (320, 10, "maptiles4",       "ground"),
    (330, 19, "maptiles5",       "ground"),
    (349,  4, "maptiles6",       "ground"),
    (353,  9, "maptiles353-361", "ground"),
    (363,  4, "Tile363-366",     "ground"),
    (367,  1, "Tile367-367",     "ground"),
    (370, 12, "Tile370-381",     "ground"),
    (382,  6, "Tile382-387",     "ground"),
    (388, 15, "Tile388-402",     "ground"),
    (403,  3, "Tile403-405",     "ground"),
    (406, 16, "Tile406-421",     "ground"),
    (422,  8, "Tile422-429",     "object"),
    (430, 14, "Tile430-443",     "ground"),
    (444,  1, "Tile444-444",     "ground"),
    (445, 17, "Tile445-461",     "ground"),
    (462, 12, "Tile462-473",     "ground"),
    (474,  5, "Tile474-478",     "ground"),
    (479, 10, "Tile479-488",     "ground"),
    (489, 34, "Tile489-522",     "ground"),
    (523,  8, "Tile523-530",     "ground"),
    (531, 10, "Tile531-540",     "ground"),
    (541,  5, "Tile541-545",     "ground"),
]

# Build lookup: sprite_id → (pak_name, index_within_pak)
SPRITE_ID_TO_PAK = {}
for _start, _count, _pak, _cat in SPRITE_REGISTRY:
    for _i in range(_count):
        SPRITE_ID_TO_PAK[_start + _i] = (_pak, _i, _cat)

# Category groupings for the tile palette
PALETTE_CATEGORIES = {
    "Ground": [r for r in SPRITE_REGISTRY if r[3] == "ground"],
    "Dungeon": [r for r in SPRITE_REGISTRY if r[3] == "dungeon"],
    "Trees": [r for r in SPRITE_REGISTRY if r[3] == "tree"],
    "Objects": [r for r in SPRITE_REGISTRY if r[3] == "object"],
    "Shadows": [r for r in SPRITE_REGISTRY if r[3] == "shadow"],
}

# Water tile ID (special rendering)
WATER_TILE_ID = 19

# numpy dtype for tile data (matches .amd binary layout)
TILE_DTYPE = np.dtype([
    ('gnd_id',    '<i2'),
    ('gnd_frame', '<i2'),
    ('obj_id',    '<i2'),
    ('obj_frame', '<i2'),
    ('attr',      'u1'),
    ('pad',       'u1'),
])

# Color LUT for low-zoom rendering (sprite_id → RGB tuple)
# Approximate colors for common tile types
def _build_color_lut():
    lut = {}
    # maptiles1 (0-31): various terrain
    for i in range(32):
        lut[i] = (80 + i * 4, 120 + i * 3, 60 + i * 2)  # greenish-brown
    lut[WATER_TILE_ID] = (30, 80, 180)  # water = blue
    # Sinside/dungeon (70-96): dark stone
    for i in range(70, 97):
        lut[i] = (60, 55, 50)
    # Trees (100-145): dark green
    for i in range(100, 146):
        lut[i] = (20, 100, 30)
    # Shadows (150-195): dark
    for i in range(150, 196):
        lut[i] = (30, 30, 30)
    # Objects (200-248): gray
    for i in range(200, 249):
        lut[i] = (140, 130, 120)
    # Extended ground (300+): earthy tones
    for i in range(300, 546):
        h = (i - 300) % 60
        lut[i] = (100 + h * 2, 90 + h, 70 + h)
    return lut

COLOR_LUT = _build_color_lut()

def get_tile_color(sprite_id):
    """Get approximate RGB color for a sprite ID (low-zoom rendering)."""
    return COLOR_LUT.get(sprite_id, (128, 128, 128))

# UI layout constants
TOOLBAR_WIDTH = 56
PALETTE_WIDTH = 280
STATUS_HEIGHT = 24
MINIMAP_SIZE = 200

# UI colors
UI_BG = (40, 40, 45)
UI_PANEL = (55, 55, 60)
UI_PANEL_BORDER = (80, 80, 85)
UI_TEXT = (220, 220, 220)
UI_TEXT_DIM = (140, 140, 140)
UI_HIGHLIGHT = (70, 130, 200)
UI_BUTTON = (65, 65, 70)
UI_BUTTON_HOVER = (80, 80, 90)
UI_BUTTON_ACTIVE = (70, 130, 200)
UI_GRID_COLOR = (0, 200, 200, 60)
