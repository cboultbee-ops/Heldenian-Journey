"""
Helbreath Map Editor — Sprite loading and caching from PAK files.
Reuses asset_pipeline/pak_utils.py for PAK format parsing.
"""
import struct
import io
import os
from pathlib import Path
from collections import OrderedDict

import pygame
from PIL import Image

from constants import SPRITE_REGISTRY, SPRITE_ID_TO_PAK, get_tile_color

# PAK format constants (from pak_utils.py)
PAK_HEADER_RESERVED = 20
BLOCK_HEADER_SIZE = 100
BRUSH_STRUCT = struct.Struct("<6h")  # sx, sy, szx, szy, pvx, pvy


def _apply_color_key(img):
    """Apply color-key transparency using the bottom-left pixel of the BMP.
    BMP stores rows bottom-up; PIL opens top-down, so color key is at (0, height-1)."""
    if img.mode == 'P':
        img = img.convert('RGB')
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    w, h = img.size
    if w == 0 or h == 0:
        return img
    # Color key from bottom-left pixel (in PIL coords: 0, h-1)
    ck = img.getpixel((0, h - 1))[:3]
    # Use numpy for fast pixel manipulation
    import numpy as np
    data = np.array(img)
    mask = (data[:, :, 0] == ck[0]) & (data[:, :, 1] == ck[1]) & (data[:, :, 2] == ck[2])
    data[mask, 3] = 0
    return Image.fromarray(data, 'RGBA')


class SpriteCache:
    """Lazy-loads sprite frames from PAK files, caches as pygame surfaces."""

    def __init__(self, sprites_dir, max_scaled_cache=8000):
        self.sprites_dir = Path(sprites_dir)
        self._pak_data = {}         # pak_name -> {offsets, sprites:[{brushes, bmp_data}]}
        self._frame_cache = {}      # (sprite_id, frame) -> pygame.Surface (1:1)
        self._scaled_cache = OrderedDict()  # (sprite_id, frame, zoom) -> pygame.Surface
        self._max_scaled = max_scaled_cache
        self._failed_paks = set()   # PAK names that failed to load
        self._sprite_info = {}      # sprite_id -> {total_frames, brushes}
        self._fallback_surfaces = {}  # sprite_id -> pygame.Surface (color block)

    def get_frame(self, sprite_id, frame=0):
        """Get a sprite frame at 1:1 scale. Returns pygame.Surface or fallback color block."""
        key = (sprite_id, frame)
        if key in self._frame_cache:
            return self._frame_cache[key]

        surf = self._load_frame(sprite_id, frame)
        if surf is not None:
            self._frame_cache[key] = surf
            return surf

        return self._get_fallback(sprite_id)

    def get_frame_scaled(self, sprite_id, frame, zoom):
        """Get a frame scaled to zoom level. Uses LRU cache."""
        key = (sprite_id, frame, zoom)
        if key in self._scaled_cache:
            self._scaled_cache.move_to_end(key)
            return self._scaled_cache[key]

        base = self.get_frame(sprite_id, frame)
        if zoom == 1:
            # At 1px/tile just use a single colored pixel
            scaled = self._get_fallback(sprite_id, 1)
        elif base.get_width() <= 1:
            # Fallback surface
            scaled = pygame.Surface((zoom, zoom))
            scaled.fill(get_tile_color(sprite_id))
        else:
            scaled = pygame.transform.scale(base, (zoom, zoom))

        self._scaled_cache[key] = scaled
        if len(self._scaled_cache) > self._max_scaled:
            self._scaled_cache.popitem(last=False)
        return scaled

    def get_thumbnail(self, sprite_id, frame=0, size=32):
        """Get a thumbnail for the palette."""
        surf = self.get_frame(sprite_id, frame)
        if surf.get_width() == size and surf.get_height() == size:
            return surf
        return pygame.transform.scale(surf, (size, size))

    def get_sprite_info(self, sprite_id):
        """Get sprite metadata: total_frames, brush data. May trigger PAK load."""
        if sprite_id in self._sprite_info:
            return self._sprite_info[sprite_id]
        # Try loading PAK metadata
        lookup = SPRITE_ID_TO_PAK.get(sprite_id)
        if lookup is None:
            return None
        pak_name, idx_in_pak, _ = lookup
        self._ensure_pak_loaded(pak_name)
        return self._sprite_info.get(sprite_id)

    def _load_frame(self, sprite_id, frame):
        """Load a specific frame from PAK. Returns pygame.Surface or None."""
        lookup = SPRITE_ID_TO_PAK.get(sprite_id)
        if lookup is None:
            return None
        pak_name, idx_in_pak, _ = lookup
        if pak_name in self._failed_paks:
            return None

        self._ensure_pak_loaded(pak_name)

        pak = self._pak_data.get(pak_name)
        if pak is None:
            return None

        sprites = pak.get('sprites', [])
        if idx_in_pak >= len(sprites):
            return None

        sprite_data = sprites[idx_in_pak]
        brushes = sprite_data.get('brushes', [])
        if frame >= len(brushes):
            return None

        sx, sy, szx, szy, pvx, pvy = brushes[frame]
        if szx <= 0 or szy <= 0:
            return None

        # Get the atlas image
        atlas = sprite_data.get('atlas')
        if atlas is None:
            return None

        # Crop frame from atlas
        try:
            frame_img = atlas.crop((sx, sy, sx + szx, sy + szy))
            # Convert to RGBA for pygame
            if frame_img.mode != 'RGBA':
                frame_img = frame_img.convert('RGBA')
            data = frame_img.tobytes()
            surf = pygame.image.frombytes(data, (szx, szy), 'RGBA')
            return surf
        except Exception:
            return None

    def _ensure_pak_loaded(self, pak_name):
        """Load PAK file metadata and atlas images if not already loaded."""
        if pak_name in self._pak_data or pak_name in self._failed_paks:
            return

        pak_path = self.sprites_dir / f"{pak_name}.pak"
        if not pak_path.exists():
            self._failed_paks.add(pak_name)
            return

        try:
            pak_data = self._read_pak(pak_path, pak_name)
            self._pak_data[pak_name] = pak_data
        except Exception as e:
            print(f"Failed to load PAK {pak_name}: {e}")
            self._failed_paks.add(pak_name)

    def _read_pak(self, pak_path, pak_name):
        """Read PAK file: offset table, brush data, BMP atlases."""
        data = {'sprites': []}
        file_size = os.path.getsize(pak_path)

        with open(pak_path, 'rb') as f:
            f.read(PAK_HEADER_RESERVED)
            i_total = struct.unpack('<i', f.read(4))[0]

            offsets = []
            for _ in range(i_total):
                block_start = struct.unpack('<I', f.read(4))[0]
                f.read(4)  # second dword (unused for us)
                offsets.append(block_start)

            # Find which sprite IDs this PAK maps to
            registry_entry = None
            for start_id, count, pname, cat in SPRITE_REGISTRY:
                if pname == pak_name:
                    registry_entry = (start_id, count, cat)
                    break

            for sprite_idx in range(i_total):
                sprite_entry = {'brushes': [], 'atlas': None}
                offset = offsets[sprite_idx]

                f.seek(offset)
                f.read(BLOCK_HEADER_SIZE)  # block header

                total_frames = struct.unpack('<i', f.read(4))[0]

                # Read brush entries
                brushes = []
                for _ in range(total_frames):
                    brush = BRUSH_STRUCT.unpack(f.read(12))
                    brushes.append(brush)  # (sx, sy, szx, szy, pvx, pvy)
                sprite_entry['brushes'] = brushes

                # Skip 4-byte padding between frame table and BMP data
                # C++ formula: m_dwBitmapFileStartLoc = iASDstart + 108 + 12*frames
                # At this point we're at offset + 104 + 12*frames, need +4 more
                f.read(4)

                # Read BMP atlas
                bmp_start = f.tell()
                bmp_header = f.read(14)
                if len(bmp_header) >= 6:
                    bf_size = struct.unpack('<I', bmp_header[2:6])[0]
                    # Cap at next block or EOF
                    next_offset = offsets[sprite_idx + 1] if sprite_idx + 1 < len(offsets) else file_size
                    available = next_offset - bmp_start
                    actual_size = min(bf_size, available)

                    f.seek(bmp_start)
                    bmp_data = f.read(actual_size)

                    try:
                        img = Image.open(io.BytesIO(bmp_data))
                        img.load()
                        # Apply color-key transparency: bottom-left pixel in BMP
                        # is the color key. In PIL (top-down), that's (0, height-1).
                        img = _apply_color_key(img)
                        sprite_entry['atlas'] = img
                    except Exception:
                        sprite_entry['atlas'] = None

                data['sprites'].append(sprite_entry)

                # Register sprite info
                if registry_entry:
                    sid = registry_entry[0] + sprite_idx
                    self._sprite_info[sid] = {
                        'total_frames': total_frames,
                        'brushes': brushes,
                    }

        return data

    def _get_fallback(self, sprite_id, size=32):
        """Get a solid-color fallback surface."""
        key = (sprite_id, size)
        if key not in self._fallback_surfaces:
            surf = pygame.Surface((size, size))
            surf.fill(get_tile_color(sprite_id))
            self._fallback_surfaces[key] = surf
        return self._fallback_surfaces[key]

    def clear_scaled_cache(self):
        """Clear scaled cache (e.g., after zoom change)."""
        self._scaled_cache.clear()
