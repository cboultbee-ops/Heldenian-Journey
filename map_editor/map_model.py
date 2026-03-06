"""
Helbreath Map Editor — Map data model with undo/redo.
"""
import numpy as np
from constants import TILE_DTYPE
from amd_io import read_amd, write_amd, create_blank_tiles

MAX_UNDO = 100


class MapModel:
    def __init__(self, width=100, height=100, default_gnd_id=0, default_gnd_frame=0):
        self.width = width
        self.height = height
        self.tiles = create_blank_tiles(width, height, default_gnd_id, default_gnd_frame)
        self.dirty = False
        self.filepath = None
        self._undo_stack = []  # [(name, changed_tiles_list)]
        self._redo_stack = []
        self._current_stroke = None  # (name, {(x,y): old_tile_bytes})

    @classmethod
    def from_file(cls, filepath):
        """Load a map from an .amd file."""
        width, height, tiles = read_amd(filepath)
        model = cls.__new__(cls)
        model.width = width
        model.height = height
        model.tiles = tiles
        model.dirty = False
        model.filepath = filepath
        model._undo_stack = []
        model._redo_stack = []
        model._current_stroke = None
        return model

    def save(self, filepath=None):
        """Save map to .amd file."""
        path = filepath or self.filepath
        if path is None:
            raise ValueError("No filepath specified")
        write_amd(path, self.width, self.height, self.tiles)
        self.filepath = path
        self.dirty = False

    def in_bounds(self, x, y):
        return 0 <= x < self.width and 0 <= y < self.height

    def get_tile(self, x, y):
        """Get tile data at (x, y). Returns a numpy void with named fields."""
        if not self.in_bounds(x, y):
            return None
        return self.tiles[y, x]

    def begin_stroke(self, name="Edit"):
        """Begin a compound edit operation."""
        if self._current_stroke is not None:
            self.end_stroke()
        self._current_stroke = (name, {})

    def end_stroke(self):
        """Finalize compound edit, push to undo stack."""
        if self._current_stroke is None:
            return
        name, changes = self._current_stroke
        self._current_stroke = None
        if not changes:
            return
        # Store as (name, {(x,y): old_tile_bytes})
        self._undo_stack.append((name, changes))
        if len(self._undo_stack) > MAX_UNDO:
            self._undo_stack.pop(0)
        self._redo_stack.clear()
        self.dirty = True

    def set_ground(self, x, y, gnd_id, gnd_frame):
        """Set ground tile, recording undo."""
        if not self.in_bounds(x, y):
            return
        self._record_old(x, y)
        self.tiles[y, x]['gnd_id'] = gnd_id
        self.tiles[y, x]['gnd_frame'] = gnd_frame

    def set_object(self, x, y, obj_id, obj_frame):
        """Set object on tile, recording undo."""
        if not self.in_bounds(x, y):
            return
        self._record_old(x, y)
        self.tiles[y, x]['obj_id'] = obj_id
        self.tiles[y, x]['obj_frame'] = obj_frame

    def clear_object(self, x, y):
        """Remove object from tile."""
        self.set_object(x, y, 0, 0)

    def set_attr(self, x, y, attr_flag, value=True):
        """Set or clear an attribute flag."""
        if not self.in_bounds(x, y):
            return
        self._record_old(x, y)
        cur = int(self.tiles[y, x]['attr'])
        if value:
            cur |= attr_flag
        else:
            cur &= ~attr_flag
        self.tiles[y, x]['attr'] = cur

    def toggle_attr(self, x, y, attr_flag):
        """Toggle an attribute flag."""
        if not self.in_bounds(x, y):
            return
        cur = int(self.tiles[y, x]['attr'])
        self.set_attr(x, y, attr_flag, not (cur & attr_flag))

    def _record_old(self, x, y):
        """Record old tile value for undo (only first time per stroke)."""
        if self._current_stroke is None:
            self.begin_stroke()
        _, changes = self._current_stroke
        key = (x, y)
        if key not in changes:
            changes[key] = self.tiles[y, x].copy()

    def undo(self):
        """Undo last stroke."""
        if not self._undo_stack:
            return False
        name, changes = self._undo_stack.pop()
        # Save current state for redo
        redo_changes = {}
        for (x, y), old_tile in changes.items():
            redo_changes[(x, y)] = self.tiles[y, x].copy()
            self.tiles[y, x] = old_tile
        self._redo_stack.append((name, redo_changes))
        self.dirty = True
        return True

    def redo(self):
        """Redo last undone stroke."""
        if not self._redo_stack:
            return False
        name, changes = self._redo_stack.pop()
        undo_changes = {}
        for (x, y), new_tile in changes.items():
            undo_changes[(x, y)] = self.tiles[y, x].copy()
            self.tiles[y, x] = new_tile
        self._undo_stack.append((name, undo_changes))
        self.dirty = True
        return True

    def flood_fill(self, start_x, start_y, new_gnd_id, new_gnd_frame, max_tiles=100000):
        """Flood-fill ground tiles from start position."""
        if not self.in_bounds(start_x, start_y):
            return 0
        target_id = int(self.tiles[start_y, start_x]['gnd_id'])
        target_frame = int(self.tiles[start_y, start_x]['gnd_frame'])
        if target_id == new_gnd_id and target_frame == new_gnd_frame:
            return 0

        self.begin_stroke("Fill")
        from collections import deque
        queue = deque([(start_x, start_y)])
        visited = set()
        count = 0
        while queue and count < max_tiles:
            x, y = queue.popleft()
            if (x, y) in visited:
                continue
            if not self.in_bounds(x, y):
                continue
            tile = self.tiles[y, x]
            if int(tile['gnd_id']) != target_id or int(tile['gnd_frame']) != target_frame:
                continue
            visited.add((x, y))
            self.set_ground(x, y, new_gnd_id, new_gnd_frame)
            count += 1
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
                if (nx, ny) not in visited:
                    queue.append((nx, ny))
        self.end_stroke()
        return count

    def copy_region(self, x0, y0, x1, y1):
        """Copy a rectangular region. Returns (w, h, tiles_copy)."""
        x0, x1 = min(x0, x1), max(x0, x1)
        y0, y1 = min(y0, y1), max(y0, y1)
        x0 = max(0, x0)
        y0 = max(0, y0)
        x1 = min(self.width - 1, x1)
        y1 = min(self.height - 1, y1)
        w = x1 - x0 + 1
        h = y1 - y0 + 1
        region = self.tiles[y0:y0+h, x0:x0+w].copy()
        return w, h, region

    def paste_region(self, dest_x, dest_y, w, h, region):
        """Paste a copied region at destination."""
        self.begin_stroke("Paste")
        for ry in range(h):
            for rx in range(w):
                tx = dest_x + rx
                ty = dest_y + ry
                if self.in_bounds(tx, ty):
                    self._record_old(tx, ty)
                    self.tiles[ty, tx] = region[ry, rx]
        self.end_stroke()
