"""
Helbreath Map Editor — Camera viewport with zoom and pan.
"""


class Viewport:
    ZOOM_LEVELS = [1, 2, 4, 8, 16, 32]

    def __init__(self, screen_w, screen_h):
        self.cam_x = 0.0  # tile coordinates (float for smooth pan)
        self.cam_y = 0.0
        self.zoom_idx = 3  # start at 8 px/tile
        self.screen_w = screen_w
        self.screen_h = screen_h

    @property
    def zoom(self):
        return self.ZOOM_LEVELS[self.zoom_idx]

    def resize(self, screen_w, screen_h):
        self.screen_w = screen_w
        self.screen_h = screen_h

    def zoom_in(self, center_sx=None, center_sy=None):
        """Zoom in, keeping the screen point under the cursor stable."""
        if self.zoom_idx >= len(self.ZOOM_LEVELS) - 1:
            return
        if center_sx is not None and center_sy is not None:
            # Tile under cursor before zoom
            tx = self.cam_x + center_sx / self.zoom
            ty = self.cam_y + center_sy / self.zoom
        self.zoom_idx += 1
        if center_sx is not None and center_sy is not None:
            # Adjust camera so same tile stays under cursor
            self.cam_x = tx - center_sx / self.zoom
            self.cam_y = ty - center_sy / self.zoom

    def zoom_out(self, center_sx=None, center_sy=None):
        """Zoom out, keeping the screen point under the cursor stable."""
        if self.zoom_idx <= 0:
            return
        if center_sx is not None and center_sy is not None:
            tx = self.cam_x + center_sx / self.zoom
            ty = self.cam_y + center_sy / self.zoom
        self.zoom_idx -= 1
        if center_sx is not None and center_sy is not None:
            self.cam_x = tx - center_sx / self.zoom
            self.cam_y = ty - center_sy / self.zoom

    def set_zoom(self, idx):
        """Set zoom level by index (0-5)."""
        self.zoom_idx = max(0, min(len(self.ZOOM_LEVELS) - 1, idx))

    def pan(self, dx_pixels, dy_pixels):
        """Pan camera by pixel delta (screen space)."""
        self.cam_x -= dx_pixels / self.zoom
        self.cam_y -= dy_pixels / self.zoom

    def center_on(self, tile_x, tile_y):
        """Center the viewport on a tile coordinate."""
        self.cam_x = tile_x - (self.screen_w / self.zoom) / 2
        self.cam_y = tile_y - (self.screen_h / self.zoom) / 2

    def screen_to_tile(self, sx, sy):
        """Convert screen pixel to tile coordinate (int)."""
        tx = int(self.cam_x + sx / self.zoom)
        ty = int(self.cam_y + sy / self.zoom)
        return tx, ty

    def tile_to_screen(self, tx, ty):
        """Convert tile coordinate to screen pixel (int)."""
        sx = int((tx - self.cam_x) * self.zoom)
        sy = int((ty - self.cam_y) * self.zoom)
        return sx, sy

    def visible_tile_rect(self, map_w, map_h):
        """Return (x0, y0, x1, y1) of visible tile range, clamped to map bounds."""
        x0 = max(0, int(self.cam_x))
        y0 = max(0, int(self.cam_y))
        x1 = min(map_w, int(self.cam_x + self.screen_w / self.zoom) + 2)
        y1 = min(map_h, int(self.cam_y + self.screen_h / self.zoom) + 2)
        return x0, y0, x1, y1

    def clamp_to_map(self, map_w, map_h):
        """Clamp camera so it doesn't go too far outside the map."""
        max_x = map_w - self.screen_w / self.zoom
        max_y = map_h - self.screen_h / self.zoom
        self.cam_x = max(-2, min(max_x + 2, self.cam_x))
        self.cam_y = max(-2, min(max_y + 2, self.cam_y))
