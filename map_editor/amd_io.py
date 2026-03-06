"""
Helbreath Map Editor — .amd binary file reader/writer.
Format: 256-byte text header + (W * H * 10) bytes tile data.
Matches Sources/Client/Map/MapData.cpp:OpenMapDataFile and Sources/HG/map/Map.cpp.
"""
import struct
import numpy as np
from constants import TILE_DTYPE

HEADER_SIZE = 256
TILE_SIZE_BYTES = 10


def read_amd(filepath):
    """
    Read a .amd map file.
    Returns (width, height, tiles) where tiles is np.ndarray of shape (height, width)
    with TILE_DTYPE. Indexing: tiles[y][x].
    """
    with open(filepath, 'rb') as f:
        header_raw = f.read(HEADER_SIZE)
        if len(header_raw) < HEADER_SIZE:
            raise ValueError(f"File too short for header: {filepath}")

        width, height = _parse_header(header_raw)
        expected_size = width * height * TILE_SIZE_BYTES
        tile_data = f.read(expected_size)
        if len(tile_data) < expected_size:
            raise ValueError(
                f"Tile data truncated: expected {expected_size}, got {len(tile_data)}"
            )

    # Parse as structured numpy array — file order is Y-outer, X-inner
    tiles = np.frombuffer(tile_data, dtype=TILE_DTYPE).reshape((height, width)).copy()
    return width, height, tiles


def write_amd(filepath, width, height, tiles):
    """
    Write a .amd map file.
    tiles: np.ndarray of shape (height, width) with TILE_DTYPE.
    """
    header = _build_header(width, height)
    # Ensure padding byte is 0
    tiles_copy = tiles.copy()
    tiles_copy['pad'] = 0

    with open(filepath, 'wb') as f:
        f.write(header)
        f.write(tiles_copy.tobytes())


def _parse_header(raw):
    """Parse the 256-byte header to extract MAPSIZEX and MAPSIZEY."""
    # Replace null bytes with spaces for tokenization
    text = raw.replace(b'\x00', b' ').decode('ascii', errors='replace')
    tokens = text.split()
    width = None
    height = None
    for i, tok in enumerate(tokens):
        if tok == 'MAPSIZEX' and i + 2 < len(tokens):
            # Skip '=' token
            try:
                width = int(tokens[i + 2])
            except (ValueError, IndexError):
                pass
        elif tok == 'MAPSIZEY' and i + 2 < len(tokens):
            try:
                height = int(tokens[i + 2])
            except (ValueError, IndexError):
                pass
    if width is None or height is None:
        raise ValueError(f"Could not parse MAPSIZEX/MAPSIZEY from header: {text[:80]}")
    return width, height


def _build_header(width, height):
    """Build the 256-byte header string."""
    text = f"MAPSIZEX = {width} MAPSIZEY = {height} TILESIZE = 10"
    raw = text.encode('ascii')
    if len(raw) > HEADER_SIZE:
        raise ValueError(f"Header text too long ({len(raw)} > {HEADER_SIZE})")
    return raw.ljust(HEADER_SIZE, b'\x00')


def create_blank_tiles(width, height, default_gnd_id=0, default_gnd_frame=0):
    """Create a blank tile array with the given default ground tile."""
    tiles = np.zeros((height, width), dtype=TILE_DTYPE)
    tiles['gnd_id'] = default_gnd_id
    tiles['gnd_frame'] = default_gnd_frame
    return tiles
