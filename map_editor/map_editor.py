"""
Helbreath Visual Map Editor
Main application: pygame loop, rendering, UI panels, editing tools.
Run: python map_editor.py [optional_map.amd]
"""
import sys
import os
import math
from pathlib import Path
from collections import deque

import pygame
import numpy as np

# Ensure constants module is found
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from constants import (
    TILE_DTYPE, TILE_SIZE, ATTR_BLOCKED, ATTR_TELEPORT, ATTR_FARM, ATTR_BUILD,
    ATTR_NOATTACK, ATTR_COLORS, ATTR_NAMES, TOOL_PAINT_GROUND, TOOL_PAINT_OBJECT,
    TOOL_ERASE_OBJECT, TOOL_EYEDROPPER, TOOL_FILL, TOOL_SELECT, TOOL_ATTRIBUTE,
    TOOL_WARP, TOOL_NAMES, TOOL_KEYS, SPRITE_REGISTRY, SPRITE_ID_TO_PAK,
    PALETTE_CATEGORIES, get_tile_color, TOOLBAR_WIDTH, PALETTE_WIDTH, STATUS_HEIGHT,
    MINIMAP_SIZE, UI_BG, UI_PANEL, UI_PANEL_BORDER, UI_TEXT, UI_TEXT_DIM,
    UI_HIGHLIGHT, UI_BUTTON, UI_BUTTON_HOVER, UI_BUTTON_ACTIVE, UI_GRID_COLOR,
)
from map_model import MapModel
from viewport import Viewport
from sprite_cache import SpriteCache
from config_io import MapConfig
from amd_io import read_amd, write_amd

# Paths
PROJECT_ROOT = Path(__file__).resolve().parent.parent
SPRITES_DIR = PROJECT_ROOT / "Client" / "SPRITES"
MAPS_DIR = PROJECT_ROOT / "Client" / "mapdata"

# Attribute flags in toggle order
ATTR_FLAGS = [ATTR_BLOCKED, ATTR_TELEPORT, ATTR_FARM, ATTR_NOATTACK]

PAN_SPEED = 400  # tiles per second at zoom=1 (scaled by zoom)


def bresenham(x0, y0, x1, y1):
    """Yield integer coordinates on the line from (x0,y0) to (x1,y1)."""
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx - dy
    while True:
        yield x0, y0
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x0 += sx
        if e2 < dx:
            err += dx
            y0 += sy


class MapEditorApp:
    def __init__(self, initial_file=None):
        pygame.init()
        info = pygame.display.Info()
        self.win_w = min(1600, info.current_w - 100)
        self.win_h = min(950, info.current_h - 100)
        self.screen = pygame.display.set_mode(
            (self.win_w, self.win_h), pygame.RESIZABLE
        )
        pygame.display.set_caption("Helbreath Map Editor")

        self.font = pygame.font.SysFont("Consolas", 14)
        self.font_small = pygame.font.SysFont("Consolas", 12)
        self.font_title = pygame.font.SysFont("Consolas", 16, bold=True)

        self.clock = pygame.time.Clock()
        self.running = True

        # Layout
        self._update_layout()

        # Core systems
        self.viewport = Viewport(self.map_rect.w, self.map_rect.h)
        self.sprite_cache = SpriteCache(SPRITES_DIR) if SPRITES_DIR.exists() else None

        # Map model
        self.model = None
        self.config = None

        # Tool state
        self.tool = TOOL_PAINT_GROUND
        self.brush_gnd_id = 0
        self.brush_gnd_frame = 0
        self.brush_obj_id = 100
        self.brush_obj_frame = 0
        self.brush_size = 1
        self.active_attr = ATTR_BLOCKED
        self.attr_paint_value = True  # True=set, False=clear

        # Interaction state
        self.painting = False
        self.panning = False
        self.pan_start = None
        self.last_paint_tile = None
        self.selecting = False
        self.selection_start = None
        self.selection_rect = None  # (x0, y0, x1, y1) in tiles
        self.clipboard = None       # (w, h, tiles_array)
        self.paste_preview = None   # (tx, ty) or None

        # Display toggles
        self.show_grid = False
        self.show_attributes = True
        self.show_objects = True
        self.show_config_overlay = True

        # Warp editing state
        self.selected_warp = None  # (sx, sy, dest_map, dx, dy, dir) or None
        self._warp_edit_index = None  # index into config.teleport_locs
        self._warp_edit_btn = None
        self._warp_del_btn = None
        self._warp_goto_btn = None

        # Palette state
        self.palette_scroll = 0
        self.palette_category = "Ground"
        self.palette_expanded_sprite = None  # sprite_id when showing frames

        # Minimap
        self.minimap_surface = None
        self.minimap_dirty = True

        # Keys held
        self.keys_held = set()

        # Load initial file
        if initial_file:
            self._load_map(initial_file)
        else:
            self._new_map(200, 200, "newmap")

    def _update_layout(self):
        self.toolbar_rect = pygame.Rect(0, 0, TOOLBAR_WIDTH, self.win_h)
        self.palette_rect = pygame.Rect(
            self.win_w - PALETTE_WIDTH, 0, PALETTE_WIDTH, self.win_h
        )
        self.map_rect = pygame.Rect(
            TOOLBAR_WIDTH, 0,
            self.win_w - TOOLBAR_WIDTH - PALETTE_WIDTH,
            self.win_h - STATUS_HEIGHT
        )
        self.status_rect = pygame.Rect(
            TOOLBAR_WIDTH, self.win_h - STATUS_HEIGHT,
            self.win_w - TOOLBAR_WIDTH - PALETTE_WIDTH, STATUS_HEIGHT
        )
        self.minimap_rect = pygame.Rect(
            self.palette_rect.x + 10,
            self.palette_rect.bottom - MINIMAP_SIZE - 10,
            PALETTE_WIDTH - 20, MINIMAP_SIZE
        )

    def run(self):
        while self.running:
            dt = self.clock.tick(60) / 1000.0
            self._handle_events()
            self._update(dt)
            self._render()
            pygame.display.flip()
        pygame.quit()

    # ---- Events ----

    def _handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self._try_quit()
            elif event.type == pygame.VIDEORESIZE:
                self.win_w, self.win_h = event.w, event.h
                self.screen = pygame.display.set_mode(
                    (self.win_w, self.win_h), pygame.RESIZABLE
                )
                self._update_layout()
                self.viewport.resize(self.map_rect.w, self.map_rect.h)
                self.minimap_dirty = True
            elif event.type == pygame.KEYDOWN:
                self._on_keydown(event)
                self.keys_held.add(event.key)
            elif event.type == pygame.KEYUP:
                self.keys_held.discard(event.key)
            elif event.type == pygame.MOUSEBUTTONDOWN:
                self._on_mousedown(event)
            elif event.type == pygame.MOUSEBUTTONUP:
                self._on_mouseup(event)
            elif event.type == pygame.MOUSEMOTION:
                self._on_mousemove(event)
            elif event.type == pygame.MOUSEWHEEL:
                self._on_mousewheel(event)

    def _on_keydown(self, event):
        mods = pygame.key.get_mods()
        ctrl = mods & pygame.KMOD_CTRL
        shift = mods & pygame.KMOD_SHIFT

        if ctrl:
            if event.key == pygame.K_n:
                self._new_map_dialog()
            elif event.key == pygame.K_o:
                self._open_dialog()
            elif event.key == pygame.K_s:
                if shift:
                    self._save_as_dialog()
                else:
                    self._save()
            elif event.key == pygame.K_z:
                if self.model:
                    self.model.undo()
                    self.minimap_dirty = True
            elif event.key == pygame.K_y:
                if self.model:
                    self.model.redo()
                    self.minimap_dirty = True
            elif event.key == pygame.K_c:
                self._copy_selection()
            elif event.key == pygame.K_v:
                self._start_paste()
            return

        # Tool keys
        name = pygame.key.name(event.key)
        if name in TOOL_KEYS:
            self.tool = TOOL_KEYS[name]
            self.selection_rect = None
            self.paste_preview = None

        # Zoom keys 1-6
        if pygame.K_1 <= event.key <= pygame.K_6:
            self.viewport.set_zoom(event.key - pygame.K_1)

        # Warp tool shortcuts
        if self.tool == TOOL_WARP:
            if event.key == pygame.K_e:
                self._edit_warp_dialog()
                return
            elif event.key == pygame.K_DELETE or event.key == pygame.K_BACKSPACE:
                self._delete_selected_warp()
                return
            elif event.key == pygame.K_SPACE and self.selected_warp:
                # Go to the selected warp source tile
                sx, sy = self.selected_warp[0], self.selected_warp[1]
                self.viewport.center_on(sx, sy)
                return

        if event.key == pygame.K_TAB:
            self.show_grid = not self.show_grid
        elif event.key == pygame.K_LEFTBRACKET:
            self.brush_size = max(1, self.brush_size - 2)
        elif event.key == pygame.K_RIGHTBRACKET:
            self.brush_size = min(15, self.brush_size + 2)

        # Attribute sub-toggle (1-4 when in attribute mode)
        if self.tool == TOOL_ATTRIBUTE:
            if event.key == pygame.K_1:
                self.active_attr = ATTR_BLOCKED
            elif event.key == pygame.K_2:
                self.active_attr = ATTR_TELEPORT
            elif event.key == pygame.K_3:
                self.active_attr = ATTR_FARM
            elif event.key == pygame.K_4:
                self.active_attr = ATTR_NOATTACK

    def _on_mousedown(self, event):
        mx, my = event.pos

        # Palette click
        if self.palette_rect.collidepoint(mx, my):
            self._palette_click(mx, my, event.button)
            return

        # Toolbar click
        if self.toolbar_rect.collidepoint(mx, my):
            self._toolbar_click(mx, my)
            return

        # Minimap click
        if self.minimap_rect.collidepoint(mx, my):
            self._minimap_click(mx, my)
            return

        # Map area
        if not self.map_rect.collidepoint(mx, my):
            return
        if self.model is None:
            return

        local_x = mx - self.map_rect.x
        local_y = my - self.map_rect.y

        # Middle mouse = pan
        if event.button == 2:
            self.panning = True
            self.pan_start = (mx, my)
            return

        # Right click = eyedropper shortcut
        if event.button == 3:
            tx, ty = self.viewport.screen_to_tile(local_x, local_y)
            self._eyedropper(tx, ty)
            return

        if event.button != 1:
            return

        tx, ty = self.viewport.screen_to_tile(local_x, local_y)

        # Paste mode
        if self.paste_preview is not None:
            self._do_paste(tx, ty)
            return

        # Tool actions
        if self.tool == TOOL_WARP:
            self._warp_click(tx, ty)
            return
        elif self.tool == TOOL_SELECT:
            self.selecting = True
            self.selection_start = (tx, ty)
            self.selection_rect = (tx, ty, tx, ty)
        elif self.tool == TOOL_FILL:
            self._do_fill(tx, ty)
        elif self.tool == TOOL_EYEDROPPER:
            self._eyedropper(tx, ty)
        else:
            # Paint/erase/attribute
            self.painting = True
            self.model.begin_stroke(TOOL_NAMES.get(self.tool, "Edit"))
            self.last_paint_tile = (tx, ty)
            self._paint_at(tx, ty)

    def _on_mouseup(self, event):
        if event.button == 2:
            self.panning = False
            self.pan_start = None
        if event.button == 1:
            if self.painting:
                self.painting = False
                self.model.end_stroke()
                self.minimap_dirty = True
                self.last_paint_tile = None
            if self.selecting:
                self.selecting = False

    def _on_mousemove(self, event):
        mx, my = event.pos

        if self.panning and self.pan_start:
            dx = mx - self.pan_start[0]
            dy = my - self.pan_start[1]
            self.viewport.pan(dx, dy)
            self.pan_start = (mx, my)
            return

        if not self.map_rect.collidepoint(mx, my):
            return
        if self.model is None:
            return

        local_x = mx - self.map_rect.x
        local_y = my - self.map_rect.y
        tx, ty = self.viewport.screen_to_tile(local_x, local_y)

        if self.paste_preview is not None:
            self.paste_preview = (tx, ty)

        if self.painting and self.last_paint_tile:
            # Bresenham interpolation from last point
            lx, ly = self.last_paint_tile
            for px, py in bresenham(lx, ly, tx, ty):
                self._paint_at(px, py)
            self.last_paint_tile = (tx, ty)

        if self.selecting and self.selection_start:
            sx, sy = self.selection_start
            self.selection_rect = (min(sx, tx), min(sy, ty), max(sx, tx), max(sy, ty))

    def _on_mousewheel(self, event):
        mx, my = pygame.mouse.get_pos()

        # Palette scroll
        if self.palette_rect.collidepoint(mx, my):
            self.palette_scroll -= event.y * 40
            self.palette_scroll = max(0, self.palette_scroll)
            return

        # Map zoom
        if self.map_rect.collidepoint(mx, my):
            local_x = mx - self.map_rect.x
            local_y = my - self.map_rect.y
            if event.y > 0:
                self.viewport.zoom_in(local_x, local_y)
            elif event.y < 0:
                self.viewport.zoom_out(local_x, local_y)

    # ---- Update ----

    def _update(self, dt):
        if self.model is None:
            return

        # WASD panning
        speed = PAN_SPEED * dt / self.viewport.zoom * 8
        if pygame.K_w in self.keys_held or pygame.K_UP in self.keys_held:
            self.viewport.cam_y -= speed
        if pygame.K_s in self.keys_held and self.tool != TOOL_SELECT or pygame.K_DOWN in self.keys_held:
            self.viewport.cam_y += speed
        if pygame.K_a in self.keys_held and self.tool != TOOL_ATTRIBUTE or pygame.K_LEFT in self.keys_held:
            self.viewport.cam_x -= speed
        if pygame.K_d in self.keys_held or pygame.K_RIGHT in self.keys_held:
            self.viewport.cam_x += speed

        self.viewport.clamp_to_map(self.model.width, self.model.height)

    # ---- Rendering ----

    def _render(self):
        self.screen.fill(UI_BG)

        if self.model:
            # Clip to map area
            self.screen.set_clip(self.map_rect)
            self._render_map()
            self.screen.set_clip(None)

        self._render_toolbar()
        self._render_palette()
        self._render_status_bar()
        if self.model:
            self._render_minimap()

    def _render_map(self):
        """Render the map tiles in the viewport area."""
        model = self.model
        vp = self.viewport
        zoom = vp.zoom
        x0, y0, x1, y1 = vp.visible_tile_rect(model.width, model.height)
        ox = self.map_rect.x
        oy = self.map_rect.y

        if zoom >= 8 and self.sprite_cache:
            # Sprite rendering mode
            for ty in range(y0, y1):
                for tx in range(x0, x1):
                    tile = model.tiles[ty, tx]
                    sx, sy = vp.tile_to_screen(tx, ty)
                    sx += ox
                    sy += oy
                    # Ground
                    gnd_id = int(tile['gnd_id'])
                    gnd_frame = int(tile['gnd_frame'])
                    surf = self.sprite_cache.get_frame_scaled(gnd_id, gnd_frame, zoom)
                    self.screen.blit(surf, (sx, sy))
                    # Object
                    if self.show_objects:
                        obj_id = int(tile['obj_id'])
                        if obj_id != 0:
                            obj_frame = int(tile['obj_frame'])
                            obj_surf = self.sprite_cache.get_frame_scaled(obj_id, obj_frame, zoom)
                            self.screen.blit(obj_surf, (sx, sy))
        else:
            # Color-coded mode (fast for low zoom)
            for ty in range(y0, y1):
                for tx in range(x0, x1):
                    tile = model.tiles[ty, tx]
                    sx, sy = vp.tile_to_screen(tx, ty)
                    sx += ox
                    sy += oy
                    color = get_tile_color(int(tile['gnd_id']))
                    if zoom <= 1:
                        self.screen.set_at((sx, sy), color)
                    else:
                        pygame.draw.rect(self.screen, color, (sx, sy, zoom, zoom))

                    # Object indicator at medium zoom
                    if self.show_objects and zoom >= 4 and int(tile['obj_id']) != 0:
                        obj_color = get_tile_color(int(tile['obj_id']))
                        half = zoom // 2
                        pygame.draw.rect(self.screen, obj_color,
                                         (sx + half // 2, sy + half // 2, half, half))

        # Attribute overlay
        if self.show_attributes and zoom >= 2:
            attr_surf = pygame.Surface((zoom, zoom), pygame.SRCALPHA)
            for ty in range(y0, y1):
                for tx in range(x0, x1):
                    attr = int(model.tiles[ty, tx]['attr'])
                    if attr == 0:
                        continue
                    sx, sy = vp.tile_to_screen(tx, ty)
                    sx += ox
                    sy += oy
                    for flag, color in ATTR_COLORS.items():
                        if attr & flag:
                            attr_surf.fill(color)
                            self.screen.blit(attr_surf, (sx, sy))
                            break  # show highest priority only

        # Grid
        if self.show_grid and zoom >= 4:
            grid_color = (0, 200, 200, 40)
            for tx in range(x0, x1 + 1):
                sx = vp.tile_to_screen(tx, 0)[0] + ox
                pygame.draw.line(self.screen, grid_color[:3],
                                 (sx, self.map_rect.y), (sx, self.map_rect.bottom), 1)
            for ty in range(y0, y1 + 1):
                sy = vp.tile_to_screen(0, ty)[1] + oy
                pygame.draw.line(self.screen, grid_color[:3],
                                 (self.map_rect.x, sy), (self.map_rect.right, sy), 1)

        # Selection rect
        if self.selection_rect:
            sx0, sy0, sx1, sy1 = self.selection_rect
            px0, py0 = vp.tile_to_screen(sx0, sy0)
            px1, py1 = vp.tile_to_screen(sx1 + 1, sy1 + 1)
            px0 += ox
            py0 += oy
            px1 += ox
            py1 += oy
            sel_rect = pygame.Rect(px0, py0, px1 - px0, py1 - py0)
            pygame.draw.rect(self.screen, (255, 255, 0), sel_rect, 2)

        # Paste preview
        if self.paste_preview and self.clipboard:
            px, py = self.paste_preview
            cw, ch, _ = self.clipboard
            px0, py0 = vp.tile_to_screen(px, py)
            px1, py1 = vp.tile_to_screen(px + cw, py + ch)
            px0 += ox
            py0 += oy
            px1 += ox
            py1 += oy
            paste_rect = pygame.Rect(px0, py0, px1 - px0, py1 - py0)
            pygame.draw.rect(self.screen, (0, 255, 128), paste_rect, 2)

        # Config overlay (teleport destinations, spawn points, no-attack areas)
        if self.show_config_overlay and self.config and zoom >= 2:
            self._render_config_overlay(vp, ox, oy, x0, y0, x1, y1, zoom)

        # Cursor highlight
        mx, my = pygame.mouse.get_pos()
        if self.map_rect.collidepoint(mx, my) and not self.panning:
            local_x = mx - self.map_rect.x
            local_y = my - self.map_rect.y
            cx, cy = vp.screen_to_tile(local_x, local_y)
            half = self.brush_size // 2
            for bx in range(-half, half + 1):
                for by in range(-half, half + 1):
                    px, py = vp.tile_to_screen(cx + bx, cy + by)
                    px += ox
                    py += oy
                    pygame.draw.rect(self.screen, (255, 255, 255), (px, py, zoom, zoom), 1)

    def _render_toolbar(self):
        """Render left toolbar."""
        r = self.toolbar_rect
        bw = r.w - 8  # button width
        pygame.draw.rect(self.screen, UI_PANEL, r)
        pygame.draw.line(self.screen, UI_PANEL_BORDER, r.topright, r.bottomright)

        # ---- File operation buttons at top ----
        y = 6
        mx, my = pygame.mouse.get_pos()
        file_buttons = [
            ("New",  (100, 200, 100)),
            ("Open", (100, 160, 220)),
            ("Save", (220, 180, 80)),
            ("SvAs", (180, 140, 80)),
        ]
        self._file_btn_rects = []
        for label, text_color in file_buttons:
            btn_rect = pygame.Rect(4, y, bw, 22)
            self._file_btn_rects.append(btn_rect)
            color = UI_BUTTON_HOVER if btn_rect.collidepoint(mx, my) else UI_BUTTON
            pygame.draw.rect(self.screen, color, btn_rect, border_radius=3)
            txt = self.font_small.render(label, True, text_color)
            self.screen.blit(txt, (btn_rect.x + (bw - txt.get_width()) // 2, btn_rect.y + 3))
            y += 26

        # Separator
        y += 4
        pygame.draw.line(self.screen, UI_PANEL_BORDER, (4, y), (4 + bw, y))
        y += 8

        # ---- Tool buttons ----
        tools = [
            (TOOL_PAINT_GROUND, "G", "Ground"),
            (TOOL_PAINT_OBJECT, "O", "Object"),
            (TOOL_ERASE_OBJECT, "E", "Erase"),
            (TOOL_EYEDROPPER,   "I", "Pick"),
            (TOOL_FILL,         "F", "Fill"),
            (TOOL_SELECT,       "S", "Select"),
            (TOOL_ATTRIBUTE,    "A", "Attr"),
            (TOOL_WARP,         "W", "Warp"),
        ]

        for tool_id, key, label in tools:
            btn_rect = pygame.Rect(4, y, bw, 28)
            color = UI_BUTTON_ACTIVE if tool_id == self.tool else UI_BUTTON
            if btn_rect.collidepoint(mx, my) and tool_id != self.tool:
                color = UI_BUTTON_HOVER
            pygame.draw.rect(self.screen, color, btn_rect, border_radius=4)
            # Show key + name
            key_txt = self.font_small.render(key, True, (180, 220, 255))
            name_txt = self.font_small.render(label, True, UI_TEXT if tool_id == self.tool else UI_TEXT_DIM)
            self.screen.blit(key_txt, (btn_rect.x + 4, btn_rect.y + 7))
            self.screen.blit(name_txt, (btn_rect.x + 16, btn_rect.y + 7))
            y += 32

        # Brush size indicator
        y += 8
        bs_txt = self.font_small.render(f"Brush:{self.brush_size}", True, UI_TEXT_DIM)
        self.screen.blit(bs_txt, (6, y))
        y += 16
        bs_hint = self.font_small.render("[ / ]", True, UI_TEXT_DIM)
        self.screen.blit(bs_hint, (6, y))

        # Attribute indicator when in attribute mode
        if self.tool == TOOL_ATTRIBUTE:
            y += 20
            attr_name = ATTR_NAMES.get(self.active_attr, "?")
            at_txt = self.font_small.render(attr_name[:6], True,
                                            ATTR_COLORS.get(self.active_attr, (255,255,255,255))[:3])
            self.screen.blit(at_txt, (4, y))

        # Layer toggles at bottom
        y = r.height - 120
        lbl = self.font_small.render("Layers:", True, UI_TEXT_DIM)
        self.screen.blit(lbl, (4, y))
        y += 16
        toggles = [
            ("Grid", self.show_grid),
            ("Attr", self.show_attributes),
            ("Obj", self.show_objects),
            ("Cfg", self.show_config_overlay),
        ]
        self._toggle_btn_y_start = y
        for label, active in toggles:
            btn_rect = pygame.Rect(4, y, bw, 18)
            color = UI_BUTTON_ACTIVE if active else UI_BUTTON
            pygame.draw.rect(self.screen, color, btn_rect, border_radius=3)
            txt = self.font_small.render(label, True, UI_TEXT)
            self.screen.blit(txt, (btn_rect.x + (bw - txt.get_width()) // 2,
                                   btn_rect.y + 2))
            y += 22

    def _render_palette(self):
        """Render right palette panel."""
        r = self.palette_rect
        pygame.draw.rect(self.screen, UI_PANEL, r)
        pygame.draw.line(self.screen, UI_PANEL_BORDER, r.topleft, r.bottomleft)

        # Warp tool: show warp properties panel instead of sprite palette
        if self.tool == TOOL_WARP:
            self._render_warp_panel(r)
            return

        # Category tabs
        tab_y = r.y + 4
        tab_x = r.x + 6
        categories = list(PALETTE_CATEGORIES.keys())
        for cat in categories:
            txt = self.font_small.render(cat[:5], True,
                                         UI_TEXT if cat == self.palette_category else UI_TEXT_DIM)
            tw = txt.get_width() + 12
            tab_rect = pygame.Rect(tab_x, tab_y, tw, 20)
            color = UI_BUTTON_ACTIVE if cat == self.palette_category else UI_BUTTON
            pygame.draw.rect(self.screen, color, tab_rect, border_radius=3)
            self.screen.blit(txt, (tab_x + 6, tab_y + 3))
            tab_x += tw + 4

        # Current brush info
        info_y = tab_y + 28
        if self.tool in (TOOL_PAINT_GROUND, TOOL_FILL):
            info = f"Ground: ID={self.brush_gnd_id} F={self.brush_gnd_frame}"
        elif self.tool == TOOL_PAINT_OBJECT:
            info = f"Object: ID={self.brush_obj_id} F={self.brush_obj_frame}"
        else:
            info = f"Ground: ID={self.brush_gnd_id} F={self.brush_gnd_frame}"
        txt = self.font_small.render(info, True, UI_TEXT)
        self.screen.blit(txt, (r.x + 8, info_y))

        # Sprite grid
        grid_y = info_y + 24
        grid_x = r.x + 8
        cell_size = 36
        cols = (r.w - 16) // cell_size
        if cols < 1:
            cols = 1

        entries = PALETTE_CATEGORIES.get(self.palette_category, [])
        # Build flat list of sprite IDs
        sprite_ids = []
        for start_id, count, pak_name, cat in entries:
            for i in range(count):
                sprite_ids.append(start_id + i)

        # Apply scroll
        total_rows = (len(sprite_ids) + cols - 1) // cols
        max_scroll = max(0, total_rows * cell_size - (r.h - grid_y + r.y - MINIMAP_SIZE - 30))
        self.palette_scroll = min(self.palette_scroll, max_scroll)

        # Clip to palette area
        clip_rect = pygame.Rect(r.x, grid_y, r.w, r.h - (grid_y - r.y) - MINIMAP_SIZE - 20)
        self.screen.set_clip(clip_rect)

        for idx, sid in enumerate(sprite_ids):
            row = idx // cols
            col = idx % cols
            cx = grid_x + col * cell_size
            cy = grid_y + row * cell_size - self.palette_scroll
            if cy + cell_size < grid_y or cy > clip_rect.bottom:
                continue

            cell_rect = pygame.Rect(cx, cy, cell_size - 2, cell_size - 2)

            # Highlight selected
            is_selected = False
            if self.tool in (TOOL_PAINT_GROUND, TOOL_FILL) and sid == self.brush_gnd_id:
                is_selected = True
            elif self.tool == TOOL_PAINT_OBJECT and sid == self.brush_obj_id:
                is_selected = True

            if is_selected:
                pygame.draw.rect(self.screen, UI_HIGHLIGHT, cell_rect, border_radius=3)
            else:
                pygame.draw.rect(self.screen, UI_BUTTON, cell_rect, border_radius=3)

            # Thumbnail
            if self.sprite_cache:
                thumb = self.sprite_cache.get_thumbnail(sid, 0, cell_size - 6)
                self.screen.blit(thumb, (cx + 2, cy + 2))
            else:
                color = get_tile_color(sid)
                pygame.draw.rect(self.screen, color, (cx + 2, cy + 2, cell_size - 6, cell_size - 6))

            # ID label
            id_txt = self.font_small.render(str(sid), True, (255, 255, 255))
            self.screen.blit(id_txt, (cx + 2, cy + cell_size - 14))

        self.screen.set_clip(None)

    def _render_warp_panel(self, r):
        """Render warp editing panel in the palette area."""
        y = r.y + 8
        title = self.font_title.render("Warp Editor", True, (0, 220, 255))
        self.screen.blit(title, (r.x + 8, y))
        y += 24

        if self.config is None:
            txt = self.font_small.render("No config loaded", True, UI_TEXT_DIM)
            self.screen.blit(txt, (r.x + 8, y))
            return

        # Selected warp info
        if self.selected_warp:
            sx, sy, dest, dx, dy, d = self.selected_warp
            dir_names = {1: "N", 2: "NE", 3: "E", 4: "SE", 5: "S", 6: "SW", 7: "W", 8: "NW"}
            lines = [
                f"Source: ({sx}, {sy})",
                f"Dest Map: {dest}",
                f"Dest Pos: ({dx}, {dy})",
                f"Direction: {d} ({dir_names.get(d, '?')})",
            ]
            for line in lines:
                txt = self.font.render(line, True, UI_TEXT)
                self.screen.blit(txt, (r.x + 8, y))
                y += 18

            y += 6
            # Edit button
            edit_rect = pygame.Rect(r.x + 8, y, 80, 24)
            mx, my = pygame.mouse.get_pos()
            color = UI_BUTTON_HOVER if edit_rect.collidepoint(mx, my) else UI_BUTTON
            pygame.draw.rect(self.screen, color, edit_rect, border_radius=4)
            txt = self.font.render("Edit (E)", True, UI_TEXT)
            self.screen.blit(txt, (edit_rect.x + 8, edit_rect.y + 4))
            self._warp_edit_btn = edit_rect

            # Delete button
            del_rect = pygame.Rect(r.x + 100, y, 90, 24)
            color = (180, 50, 50) if del_rect.collidepoint(mx, my) else (120, 40, 40)
            pygame.draw.rect(self.screen, color, del_rect, border_radius=4)
            txt = self.font.render("Delete", True, (255, 120, 120))
            self.screen.blit(txt, (del_rect.x + 14, del_rect.y + 4))
            self._warp_del_btn = del_rect

            # Go-to button
            y += 30
            goto_rect = pygame.Rect(r.x + 8, y, 120, 24)
            color = UI_BUTTON_HOVER if goto_rect.collidepoint(mx, my) else UI_BUTTON
            pygame.draw.rect(self.screen, color, goto_rect, border_radius=4)
            txt = self.font.render("Go To (Space)", True, UI_TEXT)
            self.screen.blit(txt, (goto_rect.x + 6, goto_rect.y + 4))
            self._warp_goto_btn = goto_rect
            y += 30
        else:
            txt = self.font_small.render("Click a teleport tile", True, UI_TEXT_DIM)
            self.screen.blit(txt, (r.x + 8, y))
            y += 16
            txt = self.font_small.render("to select/create a warp", True, UI_TEXT_DIM)
            self.screen.blit(txt, (r.x + 8, y))
            y += 24
            self._warp_edit_btn = None
            self._warp_del_btn = None
            self._warp_goto_btn = None

        # Warp list
        y += 10
        txt = self.font_title.render(f"Warps ({len(self.config.teleport_locs)})", True, UI_TEXT)
        self.screen.blit(txt, (r.x + 8, y))
        y += 22

        clip_rect = pygame.Rect(r.x, y, r.w, r.h - (y - r.y) - MINIMAP_SIZE - 20)
        self.screen.set_clip(clip_rect)

        for i, (sx, sy, dest, dx, dy, d) in enumerate(self.config.teleport_locs):
            entry_rect = pygame.Rect(r.x + 4, y, r.w - 8, 36)
            is_sel = (self._warp_edit_index == i) if hasattr(self, '_warp_edit_index') else False
            bg = UI_HIGHLIGHT if is_sel else UI_BUTTON
            pygame.draw.rect(self.screen, bg, entry_rect, border_radius=3)

            line1 = f"({sx},{sy}) → {dest}"
            line2 = f"  dest({dx},{dy}) dir={d}"
            txt1 = self.font_small.render(line1, True, (0, 220, 255) if is_sel else UI_TEXT)
            txt2 = self.font_small.render(line2, True, UI_TEXT_DIM)
            self.screen.blit(txt1, (r.x + 8, y + 2))
            self.screen.blit(txt2, (r.x + 8, y + 16))
            y += 40

        self.screen.set_clip(None)

    def _render_minimap(self):
        """Render minimap in the palette area."""
        if self.model is None:
            return

        r = self.minimap_rect
        pygame.draw.rect(self.screen, (30, 30, 35), r)
        pygame.draw.rect(self.screen, UI_PANEL_BORDER, r, 1)

        # Rebuild minimap surface if dirty
        if self.minimap_dirty or self.minimap_surface is None:
            self._build_minimap()
            self.minimap_dirty = False

        if self.minimap_surface:
            # Scale to fit
            mw = self.model.width
            mh = self.model.height
            scale = min((r.w - 4) / mw, (r.h - 4) / mh)
            sw = int(mw * scale)
            sh = int(mh * scale)
            scaled = pygame.transform.scale(self.minimap_surface, (sw, sh))
            dx = r.x + (r.w - sw) // 2
            dy = r.y + (r.h - sh) // 2
            self.screen.blit(scaled, (dx, dy))

            # Viewport rect
            vp = self.viewport
            vx0 = vp.cam_x * scale + dx
            vy0 = vp.cam_y * scale + dy
            vw = (vp.screen_w / vp.zoom) * scale
            vh = (vp.screen_h / vp.zoom) * scale
            vp_rect = pygame.Rect(int(vx0), int(vy0), int(vw), int(vh))
            pygame.draw.rect(self.screen, (255, 255, 255), vp_rect, 1)

    def _build_minimap(self):
        """Build minimap surface at 1px per tile."""
        if self.model is None:
            return
        w = self.model.width
        h = self.model.height
        surf = pygame.Surface((w, h))
        # Use numpy for speed
        pixels = pygame.surfarray.pixels3d(surf)
        for ty in range(h):
            for tx in range(w):
                gnd_id = int(self.model.tiles[ty, tx]['gnd_id'])
                color = get_tile_color(gnd_id)
                pixels[tx, ty] = color[:3]
        del pixels
        self.minimap_surface = surf

    def _render_status_bar(self):
        """Render bottom status bar."""
        r = self.status_rect
        pygame.draw.rect(self.screen, UI_PANEL, r)
        pygame.draw.line(self.screen, UI_PANEL_BORDER, r.topleft, r.topright)

        parts = []
        parts.append(TOOL_NAMES.get(self.tool, "?"))

        # Cursor position
        mx, my = pygame.mouse.get_pos()
        if self.map_rect.collidepoint(mx, my) and self.model:
            local_x = mx - self.map_rect.x
            local_y = my - self.map_rect.y
            tx, ty = self.viewport.screen_to_tile(local_x, local_y)
            if self.model.in_bounds(tx, ty):
                tile = self.model.tiles[ty, tx]
                parts.append(f"Tile({tx},{ty})")
                parts.append(f"Gnd:{tile['gnd_id']}/{tile['gnd_frame']}")
                if int(tile['obj_id']) != 0:
                    parts.append(f"Obj:{tile['obj_id']}/{tile['obj_frame']}")
                attr = int(tile['attr'])
                if attr:
                    flags = []
                    for f, n in ATTR_NAMES.items():
                        if attr & f:
                            flags.append(n[0])
                    parts.append(f"[{''.join(flags)}]")
                # Show warp info if on a warp tile
                if self.tool == TOOL_WARP and self.config:
                    for wsx, wsy, wdest, wdx, wdy, wd in self.config.teleport_locs:
                        if wsx == tx and wsy == ty:
                            parts.append(f"Warp→{wdest}({wdx},{wdy})")
                            break

        if self.model:
            parts.append(f"{self.model.width}x{self.model.height}")
            parts.append(f"Zoom:{self.viewport.zoom}px")
            if self.model.dirty:
                parts.append("*MODIFIED*")

        text = "  |  ".join(parts)
        txt = self.font_small.render(text, True, UI_TEXT)
        self.screen.blit(txt, (r.x + 8, r.y + 5))

    def _render_config_overlay(self, vp, ox, oy, x0, y0, x1, y1, zoom):
        """Render config data overlays: teleport markers, spawn points, no-attack areas."""
        cfg = self.config

        # No-attack areas (filled semi-transparent orange rectangles)
        for idx, l, t, r, b in cfg.no_attack_areas:
            if r < x0 or l > x1 or b < y0 or t > y1:
                continue
            px0, py0 = vp.tile_to_screen(l, t)
            px1, py1 = vp.tile_to_screen(r + 1, b + 1)
            px0 += ox
            py0 += oy
            px1 += ox
            py1 += oy
            na_surf = pygame.Surface((px1 - px0, py1 - py0), pygame.SRCALPHA)
            na_surf.fill((255, 140, 0, 40))
            self.screen.blit(na_surf, (px0, py0))
            pygame.draw.rect(self.screen, (255, 140, 0), (px0, py0, px1 - px0, py1 - py0), 1)
            if zoom >= 8:
                lbl = self.font_small.render(f"NA{idx}", True, (255, 140, 0))
                self.screen.blit(lbl, (px0 + 2, py0 + 2))

        # Teleport location markers (cyan diamonds with destination labels)
        for sx, sy, dest_map, dx, dy, direction in cfg.teleport_locs:
            if sx < x0 or sx > x1 or sy < y0 or sy > y1:
                continue
            px, py = vp.tile_to_screen(sx, sy)
            px += ox + zoom // 2
            py += oy + zoom // 2
            sz = max(3, zoom // 2)
            # Diamond shape
            points = [(px, py - sz), (px + sz, py), (px, py + sz), (px - sz, py)]
            pygame.draw.polygon(self.screen, (0, 220, 255), points)
            pygame.draw.polygon(self.screen, (255, 255, 255), points, 1)
            # Destination label
            if zoom >= 8:
                lbl = self.font_small.render(f"→{dest_map}({dx},{dy})", True, (0, 220, 255))
                self.screen.blit(lbl, (px + sz + 2, py - 6))

            # Highlight selected warp
            if self.selected_warp is not None:
                ws = self.selected_warp
                if ws[0] == sx and ws[1] == sy:
                    pygame.draw.rect(self.screen, (255, 255, 0),
                                     (px - sz - 2, py - sz - 2, sz * 2 + 4, sz * 2 + 4), 2)

        # Initial/spawn points (green circles)
        for idx, ix, iy in cfg.initial_points:
            if ix < x0 or ix > x1 or iy < y0 or iy > y1:
                continue
            px, py = vp.tile_to_screen(ix, iy)
            px += ox + zoom // 2
            py += oy + zoom // 2
            r = max(3, zoom // 3)
            pygame.draw.circle(self.screen, (0, 255, 80), (px, py), r)
            pygame.draw.circle(self.screen, (255, 255, 255), (px, py), r, 1)
            if zoom >= 8:
                lbl = self.font_small.render(f"SP{idx}", True, (0, 255, 80))
                self.screen.blit(lbl, (px + r + 2, py - 6))

        # Waypoints (yellow squares)
        for idx, wx, wy in cfg.waypoints:
            if wx < x0 or wx > x1 or wy < y0 or wy > y1:
                continue
            px, py = vp.tile_to_screen(wx, wy)
            px += ox
            py += oy
            sz = max(2, zoom // 3)
            pygame.draw.rect(self.screen, (255, 255, 0), (px + zoom // 2 - sz, py + zoom // 2 - sz, sz * 2, sz * 2))
            if zoom >= 8:
                lbl = self.font_small.render(f"WP{idx}", True, (255, 255, 0))
                self.screen.blit(lbl, (px + zoom // 2 + sz + 2, py + zoom // 2 - 6))

    # ---- Tool actions ----

    def _paint_at(self, tx, ty):
        """Apply current tool at tile position."""
        if self.model is None:
            return
        half = self.brush_size // 2
        for bx in range(-half, half + 1):
            for by in range(-half, half + 1):
                px, py = tx + bx, ty + by
                if not self.model.in_bounds(px, py):
                    continue
                if self.tool == TOOL_PAINT_GROUND:
                    self.model.set_ground(px, py, self.brush_gnd_id, self.brush_gnd_frame)
                elif self.tool == TOOL_PAINT_OBJECT:
                    self.model.set_object(px, py, self.brush_obj_id, self.brush_obj_frame)
                elif self.tool == TOOL_ERASE_OBJECT:
                    self.model.clear_object(px, py)
                elif self.tool == TOOL_ATTRIBUTE:
                    self.model.set_attr(px, py, self.active_attr, self.attr_paint_value)

    def _do_fill(self, tx, ty):
        if self.model and self.model.in_bounds(tx, ty):
            count = self.model.flood_fill(tx, ty, self.brush_gnd_id, self.brush_gnd_frame)
            self.minimap_dirty = True

    def _eyedropper(self, tx, ty):
        if self.model is None or not self.model.in_bounds(tx, ty):
            return
        tile = self.model.tiles[ty, tx]
        mods = pygame.key.get_mods()
        if mods & pygame.KMOD_SHIFT and int(tile['obj_id']) != 0:
            self.brush_obj_id = int(tile['obj_id'])
            self.brush_obj_frame = int(tile['obj_frame'])
            self.tool = TOOL_PAINT_OBJECT
        else:
            self.brush_gnd_id = int(tile['gnd_id'])
            self.brush_gnd_frame = int(tile['gnd_frame'])
            self.tool = TOOL_PAINT_GROUND

    def _copy_selection(self):
        if self.model is None or self.selection_rect is None:
            return
        x0, y0, x1, y1 = self.selection_rect
        w, h, region = self.model.copy_region(x0, y0, x1, y1)
        self.clipboard = (w, h, region)

    def _start_paste(self):
        if self.clipboard is None:
            return
        self.paste_preview = (0, 0)
        self.tool = TOOL_SELECT

    def _do_paste(self, tx, ty):
        if self.model is None or self.clipboard is None:
            return
        w, h, region = self.clipboard
        self.model.paste_region(tx, ty, w, h, region)
        self.paste_preview = None
        self.minimap_dirty = True

    # ---- Warp editing ----

    def _warp_click(self, tx, ty):
        """Handle click in warp edit mode. Select existing warp or create new one."""
        if self.config is None:
            return

        # Check if clicking on an existing warp (within 2-tile radius)
        for i, (sx, sy, dest, dx, dy, d) in enumerate(self.config.teleport_locs):
            if abs(sx - tx) <= 1 and abs(sy - ty) <= 1:
                self.selected_warp = (sx, sy, dest, dx, dy, d)
                self._warp_edit_index = i
                return

        # No existing warp — check if tile has TELEPORT attribute
        if self.model and self.model.in_bounds(tx, ty):
            attr = int(self.model.tiles[ty, tx]['attr'])
            if attr & ATTR_TELEPORT:
                # Teleport tile without a warp destination — offer to create one
                self._create_warp_dialog(tx, ty)
            else:
                # Not a teleport tile — deselect
                self.selected_warp = None
                self._warp_edit_index = None

    def _create_warp_dialog(self, sx, sy):
        """Dialog to create a new warp at (sx, sy)."""
        try:
            import tkinter as tk
            from tkinter import simpledialog
            root = tk.Tk()
            root.withdraw()
            dest_map = simpledialog.askstring(
                "New Warp", f"Destination map for warp at ({sx}, {sy}):",
                initialvalue="middleland", parent=root
            )
            if not dest_map:
                root.destroy()
                return
            dx = simpledialog.askinteger("New Warp", "Destination X:", initialvalue=-1, parent=root)
            if dx is None:
                root.destroy()
                return
            dy = simpledialog.askinteger("New Warp", "Destination Y:", initialvalue=-1, parent=root)
            if dy is None:
                root.destroy()
                return
            direction = simpledialog.askinteger(
                "New Warp", "Direction (1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW):",
                initialvalue=1, minvalue=1, maxvalue=8, parent=root
            )
            root.destroy()
            if direction is None:
                return

            warp = (sx, sy, dest_map.strip(), dx, dy, direction)
            self.config.teleport_locs.append(warp)
            self.selected_warp = warp
            self._warp_edit_index = len(self.config.teleport_locs) - 1
            if self.model:
                self.model.dirty = True
        except ImportError:
            pass

    def _edit_warp_dialog(self):
        """Edit the currently selected warp."""
        if self.selected_warp is None or self.config is None:
            return
        sx, sy, dest, dx, dy, d = self.selected_warp
        try:
            import tkinter as tk
            from tkinter import simpledialog
            root = tk.Tk()
            root.withdraw()
            new_dest = simpledialog.askstring(
                "Edit Warp", f"Destination map for warp at ({sx}, {sy}):",
                initialvalue=dest, parent=root
            )
            if not new_dest:
                root.destroy()
                return
            new_dx = simpledialog.askinteger("Edit Warp", "Destination X (-1=random):",
                                              initialvalue=dx, parent=root)
            if new_dx is None:
                root.destroy()
                return
            new_dy = simpledialog.askinteger("Edit Warp", "Destination Y (-1=random):",
                                              initialvalue=dy, parent=root)
            if new_dy is None:
                root.destroy()
                return
            new_dir = simpledialog.askinteger(
                "Edit Warp", "Direction (1-8):",
                initialvalue=d, minvalue=1, maxvalue=8, parent=root
            )
            root.destroy()
            if new_dir is None:
                return

            idx = self._warp_edit_index
            if idx is not None and idx < len(self.config.teleport_locs):
                warp = (sx, sy, new_dest.strip(), new_dx, new_dy, new_dir)
                self.config.teleport_locs[idx] = warp
                self.selected_warp = warp
                if self.model:
                    self.model.dirty = True
        except ImportError:
            pass

    def _delete_selected_warp(self):
        """Delete the currently selected warp."""
        if self.selected_warp is None or self.config is None:
            return
        idx = getattr(self, '_warp_edit_index', None)
        if idx is not None and idx < len(self.config.teleport_locs):
            self.config.teleport_locs.pop(idx)
            self.selected_warp = None
            self._warp_edit_index = None
            if self.model:
                self.model.dirty = True

    # ---- UI clicks ----

    def _toolbar_click(self, mx, my):
        bw = self.toolbar_rect.w - 8

        # File buttons (New, Open, Save, SaveAs)
        if hasattr(self, '_file_btn_rects'):
            for i, btn_rect in enumerate(self._file_btn_rects):
                if btn_rect.collidepoint(mx, my):
                    if i == 0:
                        self._new_map_dialog()
                    elif i == 1:
                        self._open_dialog()
                    elif i == 2:
                        self._save()
                    elif i == 3:
                        self._save_as_dialog()
                    return

        # Tool buttons
        tools = [
            TOOL_PAINT_GROUND, TOOL_PAINT_OBJECT, TOOL_ERASE_OBJECT,
            TOOL_EYEDROPPER, TOOL_FILL, TOOL_SELECT, TOOL_ATTRIBUTE,
            TOOL_WARP,
        ]
        # Tool buttons start after file buttons (4 * 26 + 4 + 8 = 120)
        y = 6 + 4 * 26 + 12
        for tool_id in tools:
            btn_rect = pygame.Rect(4, y, bw, 28)
            if btn_rect.collidepoint(mx, my):
                self.tool = tool_id
                return
            y += 32

        # Layer toggles
        if hasattr(self, '_toggle_btn_y_start'):
            y = self._toggle_btn_y_start
        else:
            y = self.toolbar_rect.height - 120 + 16
        if pygame.Rect(4, y, bw, 18).collidepoint(mx, my):
            self.show_grid = not self.show_grid
        y += 22
        if pygame.Rect(4, y, bw, 18).collidepoint(mx, my):
            self.show_attributes = not self.show_attributes
        y += 22
        if pygame.Rect(4, y, bw, 18).collidepoint(mx, my):
            self.show_objects = not self.show_objects
        y += 22
        if pygame.Rect(4, y, bw, 18).collidepoint(mx, my):
            self.show_config_overlay = not self.show_config_overlay

    def _palette_click(self, mx, my, button):
        r = self.palette_rect

        # Warp tool panel clicks
        if self.tool == TOOL_WARP:
            if hasattr(self, '_warp_edit_btn') and self._warp_edit_btn and self._warp_edit_btn.collidepoint(mx, my):
                self._edit_warp_dialog()
                return
            if hasattr(self, '_warp_del_btn') and self._warp_del_btn and self._warp_del_btn.collidepoint(mx, my):
                self._delete_selected_warp()
                return
            if hasattr(self, '_warp_goto_btn') and self._warp_goto_btn and self._warp_goto_btn.collidepoint(mx, my):
                if self.selected_warp:
                    self.viewport.center_on(self.selected_warp[0], self.selected_warp[1])
                return
            # Click on warp list entries
            if self.config:
                y_start = r.y + 8 + 24  # title
                if self.selected_warp:
                    y_start += 18 * 4 + 6 + 30 + 30  # selected info + buttons
                else:
                    y_start += 16 + 16 + 24  # hint text
                y_start += 10 + 22  # "Warps (N)" header
                for i, warp in enumerate(self.config.teleport_locs):
                    entry_rect = pygame.Rect(r.x + 4, y_start, r.w - 8, 36)
                    if entry_rect.collidepoint(mx, my):
                        self.selected_warp = warp
                        self._warp_edit_index = i
                        # Center on this warp
                        self.viewport.center_on(warp[0], warp[1])
                        return
                    y_start += 40
            return

        # Category tabs
        tab_y = r.y + 4
        tab_x = r.x + 6
        categories = list(PALETTE_CATEGORIES.keys())
        for cat in categories:
            txt = self.font_small.render(cat[:5], True, UI_TEXT)
            tw = txt.get_width() + 12
            tab_rect = pygame.Rect(tab_x, tab_y, tw, 20)
            if tab_rect.collidepoint(mx, my):
                self.palette_category = cat
                self.palette_scroll = 0
                return
            tab_x += tw + 4

        # Sprite grid
        info_y = tab_y + 28
        grid_y = info_y + 24
        grid_x = r.x + 8
        cell_size = 36
        cols = (r.w - 16) // cell_size
        if cols < 1:
            cols = 1

        entries = PALETTE_CATEGORIES.get(self.palette_category, [])
        sprite_ids = []
        for start_id, count, pak_name, cat in entries:
            for i in range(count):
                sprite_ids.append(start_id + i)

        for idx, sid in enumerate(sprite_ids):
            row = idx // cols
            col = idx % cols
            cx = grid_x + col * cell_size
            cy = grid_y + row * cell_size - self.palette_scroll

            cell_rect = pygame.Rect(cx, cy, cell_size - 2, cell_size - 2)
            if cell_rect.collidepoint(mx, my):
                if self.tool in (TOOL_PAINT_GROUND, TOOL_FILL):
                    self.brush_gnd_id = sid
                    self.brush_gnd_frame = 0
                elif self.tool == TOOL_PAINT_OBJECT:
                    self.brush_obj_id = sid
                    self.brush_obj_frame = 0
                else:
                    self.brush_gnd_id = sid
                    self.brush_gnd_frame = 0

                # Right click to change frame
                if button == 3:
                    info = self.sprite_cache.get_sprite_info(sid) if self.sprite_cache else None
                    max_frames = info['total_frames'] if info else 300
                    if self.tool in (TOOL_PAINT_GROUND, TOOL_FILL):
                        self.brush_gnd_frame = (self.brush_gnd_frame + 1) % max_frames
                    elif self.tool == TOOL_PAINT_OBJECT:
                        self.brush_obj_frame = (self.brush_obj_frame + 1) % max_frames
                return

    def _minimap_click(self, mx, my):
        """Click on minimap to pan camera."""
        if self.model is None or self.minimap_surface is None:
            return
        r = self.minimap_rect
        mw = self.model.width
        mh = self.model.height
        scale = min((r.w - 4) / mw, (r.h - 4) / mh)
        sw = int(mw * scale)
        sh = int(mh * scale)
        dx = r.x + (r.w - sw) // 2
        dy = r.y + (r.h - sh) // 2

        tile_x = (mx - dx) / scale
        tile_y = (my - dy) / scale
        self.viewport.center_on(tile_x, tile_y)

    # ---- File operations ----

    def _new_map(self, width=200, height=200, name="newmap"):
        self.model = MapModel(width, height, default_gnd_id=0, default_gnd_frame=0)
        self.config = MapConfig.generate_template(name, width, height)
        self.viewport.center_on(width / 2, height / 2)
        self.minimap_dirty = True
        self.selection_rect = None
        self.clipboard = None
        self.paste_preview = None
        pygame.display.set_caption(f"Helbreath Map Editor - {name} ({width}x{height})")

    def _load_map(self, filepath):
        try:
            self.model = MapModel.from_file(filepath)
            self.viewport.center_on(self.model.width / 2, self.model.height / 2)
            self.minimap_dirty = True
            self.selection_rect = None
            self.selected_warp = None
            self._warp_edit_index = None

            # Try loading companion .txt config from multiple locations
            map_name = Path(filepath).stem
            txt_path = self._find_config_txt(filepath, map_name)
            if txt_path:
                try:
                    self.config = MapConfig.read(str(txt_path))
                except Exception:
                    self.config = MapConfig.generate_template(
                        map_name, self.model.width, self.model.height
                    )
            else:
                self.config = MapConfig.generate_template(
                    map_name, self.model.width, self.model.height
                )

            name = Path(filepath).stem
            pygame.display.set_caption(
                f"Helbreath Map Editor - {name} ({self.model.width}x{self.model.height})"
            )
        except Exception as e:
            print(f"Error loading map: {e}")

    def _find_config_txt(self, amd_path, map_name):
        """Search for companion .txt config file in multiple locations."""
        candidates = [
            # Same directory as the .amd
            Path(amd_path).with_suffix('.txt'),
            Path(amd_path).with_suffix('.TXT'),
        ]
        # Also check server mapdata directories
        for server_dir in ["Towns", "Neutrals", "Middleland", "Events"]:
            candidates.append(PROJECT_ROOT / "Maps" / server_dir / "MAPDATA" / f"{map_name}.txt")
            candidates.append(PROJECT_ROOT / "Maps" / server_dir / "MAPDATA" / f"{map_name}.TXT")
        # Shared Maps/MAPDATA
        candidates.append(PROJECT_ROOT / "Maps" / "MAPDATA" / f"{map_name}.txt")
        candidates.append(PROJECT_ROOT / "Maps" / "MAPDATA" / f"{map_name}.TXT")

        for p in candidates:
            if p.exists():
                return p
        return None

    def _save(self):
        if self.model is None:
            return
        if self.model.filepath:
            self.model.save()
            # Also save config
            if self.config:
                txt_path = Path(self.model.filepath).with_suffix('.txt')
                self.config.write(str(txt_path))
        else:
            self._save_as_dialog()

    def _new_map_dialog(self):
        """Simple new map dialog using tkinter."""
        try:
            import tkinter as tk
            from tkinter import simpledialog
            root = tk.Tk()
            root.withdraw()
            name = simpledialog.askstring("New Map", "Map name:", initialvalue="newmap", parent=root)
            if not name:
                root.destroy()
                return
            width = simpledialog.askinteger("New Map", "Width (tiles):", initialvalue=200,
                                            minvalue=10, maxvalue=824, parent=root)
            if not width:
                root.destroy()
                return
            height = simpledialog.askinteger("New Map", "Height (tiles):", initialvalue=200,
                                             minvalue=10, maxvalue=824, parent=root)
            root.destroy()
            if not height:
                return
            self._new_map(width, height, name)
        except ImportError:
            self._new_map(200, 200, "newmap")

    def _open_dialog(self):
        """Open file dialog."""
        try:
            import tkinter as tk
            from tkinter import filedialog
            root = tk.Tk()
            root.withdraw()
            path = filedialog.askopenfilename(
                title="Open Map",
                filetypes=[("AMD Map Files", "*.amd"), ("All Files", "*.*")],
                initialdir=str(MAPS_DIR) if MAPS_DIR.exists() else str(PROJECT_ROOT)
            )
            root.destroy()
            if path:
                self._load_map(path)
        except ImportError:
            print("tkinter not available for file dialogs")

    def _save_as_dialog(self):
        """Save-as file dialog."""
        if self.model is None:
            return
        try:
            import tkinter as tk
            from tkinter import filedialog
            root = tk.Tk()
            root.withdraw()
            path = filedialog.asksaveasfilename(
                title="Save Map As",
                filetypes=[("AMD Map Files", "*.amd"), ("All Files", "*.*")],
                defaultextension=".amd",
                initialdir=str(MAPS_DIR) if MAPS_DIR.exists() else str(PROJECT_ROOT)
            )
            root.destroy()
            if path:
                self.model.save(path)
                if self.config:
                    txt_path = Path(path).with_suffix('.txt')
                    self.config.write(str(txt_path))
                name = Path(path).stem
                pygame.display.set_caption(
                    f"Helbreath Map Editor - {name} ({self.model.width}x{self.model.height})"
                )
        except ImportError:
            print("tkinter not available for file dialogs")

    def _try_quit(self):
        if self.model and self.model.dirty:
            # Simple confirmation
            try:
                import tkinter as tk
                from tkinter import messagebox
                root = tk.Tk()
                root.withdraw()
                result = messagebox.askyesnocancel("Unsaved Changes",
                    "You have unsaved changes. Save before quitting?")
                root.destroy()
                if result is True:
                    self._save()
                    self.running = False
                elif result is False:
                    self.running = False
                # Cancel = do nothing
            except ImportError:
                self.running = False
        else:
            self.running = False


def main():
    initial_file = sys.argv[1] if len(sys.argv) > 1 else None
    app = MapEditorApp(initial_file)
    app.run()


if __name__ == "__main__":
    main()
