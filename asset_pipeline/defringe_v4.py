"""Remove color-key fringe from upscaled sprites (v4 — safe edge cleanup).

After Topaz Gigapixel 2x upscaling, the AI creates semi-transparent and
color-contaminated pixels at sprite edges. This script:
1. Binary alpha snap: restore crisp edges (alpha < cut -> 0, >= cut -> 255)
2. Edge cleanup: replace RGB of edge pixels with interior-propagated colors
3. Fallback desaturation for unreachable pixels (NEVER deletes pixels)

Key changes from v3:
- REMOVED the erosion pass that set alpha=0 on edge pixels (destroyed small sprites)
- REMOVED the unfilled-edge deletion (destroyed thin tips like horns, wand edges)
- ADDED sprite-size-aware edge layers (small sprites get gentler 1-layer treatment)
- ADDED desaturation fallback for pixels that can't be filled from interior
  (neutralizes color-key tint without deleting the pixel)

Usage:
    python defringe_v4.py                     # Process all categories
    python defringe_v4.py --cat Cat Mhr       # Process specific categories
    python defringe_v4.py --test              # Test mode: save before/after
"""
import argparse
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

BASE = Path(r"C:\Helbreath Project")
GIGAPIXEL_INPUT = BASE / "gigapixel_input"
GIGAPIXEL_OUTPUT = BASE / "gigapixel_output"
TEST_DIR = BASE / "asset_pipeline" / "defringe_test"

# UI categories are not upscaled -- skip them
UI_CATEGORIES = frozenset([
    "interface", "interface2", "GameDialog", "New-Dialog", "LoginDialog",
    "DialogText", "SPRFONTS", "newmaps", "Telescope", "Telescope2", "exch",
])


def get_colorkey(input_png: Path) -> np.ndarray:
    """Get the color key RGB from the top-left pixel of the original sprite."""
    img = Image.open(input_png).convert("RGBA")
    data = np.array(img)
    return data[0, 0, :3].copy()


def find_edge_pixels(alpha: np.ndarray) -> np.ndarray:
    """Find opaque pixels adjacent to transparent pixels."""
    h, w = alpha.shape
    opaque = alpha == 255
    transparent = alpha == 0
    edge = np.zeros_like(opaque)

    # Check 4 neighbors
    edge[1:, :]  |= opaque[1:, :]  & transparent[:-1, :]  # neighbor above
    edge[:-1, :] |= opaque[:-1, :] & transparent[1:, :]   # neighbor below
    edge[:, 1:]  |= opaque[:, 1:]  & transparent[:, :-1]   # neighbor left
    edge[:, :-1] |= opaque[:, :-1] & transparent[:, 1:]    # neighbor right

    return edge


def is_contaminated(rgb: np.ndarray, colorkey: np.ndarray,
                    threshold: float = 200) -> np.ndarray:
    """Check which pixels have color-key contamination.

    Uses two complementary checks:
    1. Euclidean distance from color key < threshold
    2. Color key channel dominance (e.g., green dominates for green CK)
    """
    ck = colorkey.astype(np.float32)
    rgb_f = rgb.astype(np.float32)

    # Distance from color key
    diff = rgb_f - ck[np.newaxis, :]
    dist = np.sqrt(np.sum(diff ** 2, axis=1))
    close_to_ck = dist < threshold

    # Channel dominance check: if the CK has a dominant channel,
    # check if the pixel also has that channel dominant
    ck_max = np.max(ck)
    ck_min = np.min(ck)

    if ck_max - ck_min > 80:  # CK has a clear dominant channel
        dominant_ch = np.argmax(ck)
        pixel_dominant = rgb_f[:, dominant_ch]

        # Other channels
        other_chs = [i for i in range(3) if i != dominant_ch]
        other_max = np.maximum(rgb_f[:, other_chs[0]], rgb_f[:, other_chs[1]])

        # Pixel is CK-influenced if dominant channel > other channels by margin
        ck_dominant = (pixel_dominant > 80) & (pixel_dominant > other_max * 1.2)

        return close_to_ck | ck_dominant
    else:
        # CK is grayish (like black or white) -- use distance only
        return close_to_ck


def desaturate_dominant_channel(data: np.ndarray, mask: np.ndarray,
                                colorkey: np.ndarray) -> int:
    """Desaturate the color-key's dominant channel on masked pixels.

    Instead of deleting pixels (alpha=0), this neutralizes the color-key
    tint by blending the dominant channel toward the average of the other
    channels. Preserves the pixel while removing the green/purple cast.

    Returns count of pixels modified.
    """
    ck = colorkey.astype(np.float32)
    ck_max = np.max(ck)
    ck_min = np.min(ck)

    if ck_max - ck_min <= 80:
        # CK is grayish -- no dominant channel to desaturate
        return 0

    dominant_ch = int(np.argmax(ck))
    other_chs = [i for i in range(3) if i != dominant_ch]

    # Get pixels to fix
    ys, xs = np.where(mask)
    if len(ys) == 0:
        return 0

    count = 0
    for y, x in zip(ys, xs):
        r, g, b = int(data[y, x, 0]), int(data[y, x, 1]), int(data[y, x, 2])
        channels = [r, g, b]
        dom_val = channels[dominant_ch]
        other_vals = [channels[c] for c in other_chs]
        other_avg = sum(other_vals) // 2

        # Only desaturate if dominant channel is actually elevated
        if dom_val > other_avg + 25 and dom_val > 60:
            # Blend dominant channel 80% toward average of others
            new_val = (other_avg * 4 + dom_val) // 5
            data[y, x, dominant_ch] = np.uint8(new_val)
            count += 1

    return count


def choose_edge_layers(h: int, w: int) -> int:
    """Choose edge layer count based on sprite dimensions.

    Small sprites get gentler treatment to avoid consuming all pixels as "edge".
    """
    min_dim = min(h, w)
    opaque_area = max(h, w)  # rough proxy

    if min_dim < 48:
        return 1  # Very small/thin sprites: 1 layer only
    elif min_dim < 96:
        return 2  # Medium sprites: 2 layers
    else:
        return 3  # Large sprites: full 3 layers


def defringe_sprite(upscaled_path: Path, colorkey_rgb: np.ndarray,
                    alpha_cut: int = 128, edge_layers: int = 0) -> tuple:
    """Remove color-key fringe from one upscaled sprite.

    Returns (modified: bool, stats: dict).

    Algorithm:
    1. Binary alpha snap
    2. Choose edge layers based on sprite size (or use override)
    3. Find opaque pixels within N layers of the transparent boundary
    4. Propagate interior RGB outward to replace edge pixels
    5. Desaturate unfilled contaminated pixels (NEVER delete)
    """
    img = Image.open(upscaled_path).convert("RGBA")
    data = np.array(img)
    h, w = data.shape[:2]

    alpha = data[:, :, 3]
    modified = False
    stats = {"semi_snapped": 0, "edge_fixed": 0, "desaturated": 0,
             "edge_layers": 0}

    # Step 1: Binary alpha snap
    semi = (alpha > 0) & (alpha < 255)
    if np.any(semi):
        stats["semi_snapped"] = int(np.sum(semi))
        data[:, :, 3] = np.where(alpha < alpha_cut, 0, 255).astype(np.uint8)
        alpha = data[:, :, 3]
        modified = True

    # Step 2: Choose edge layers based on sprite content size
    opaque = alpha == 255
    transparent = alpha == 0

    # Skip if no transparent or no opaque pixels
    if not np.any(transparent) or not np.any(opaque):
        if modified:
            Image.fromarray(data).save(upscaled_path, "PNG")
        return modified, stats

    # Measure opaque bounding box to determine effective sprite size
    opaque_ys, opaque_xs = np.where(opaque)
    sprite_h = int(opaque_ys.max() - opaque_ys.min() + 1)
    sprite_w = int(opaque_xs.max() - opaque_xs.min() + 1)

    if edge_layers <= 0:
        edge_layers = choose_edge_layers(sprite_h, sprite_w)
    stats["edge_layers"] = edge_layers

    # Step 3: Find all edge pixels (within edge_layers of transparent boundary)
    boundary = transparent.copy()
    all_edge = np.zeros_like(opaque)
    for layer in range(edge_layers):
        next_boundary = np.zeros_like(boundary)
        next_boundary[1:, :]  |= opaque[1:, :]  & boundary[:-1, :]
        next_boundary[:-1, :] |= opaque[:-1, :] & boundary[1:, :]
        next_boundary[:, 1:]  |= opaque[:, 1:]  & boundary[:, :-1]
        next_boundary[:, :-1] |= opaque[:, :-1] & boundary[:, 1:]
        new_edge = next_boundary & ~all_edge
        all_edge |= new_edge
        boundary = boundary | new_edge

    if not np.any(all_edge):
        if modified:
            Image.fromarray(data).save(upscaled_path, "PNG")
        return modified, stats

    # Step 4: Propagate interior RGB outward to replace edge pixels
    interior = opaque & ~all_edge

    if not np.any(interior):
        # Sprite is too small -- all opaque pixels are edge pixels.
        # Instead of skipping (v3), desaturate contaminated pixels.
        edge_rgb = data[:, :, :3][all_edge]
        contam = is_contaminated(edge_rgb, colorkey_rgb, threshold=200)
        if np.any(contam):
            contam_mask = np.zeros((h, w), dtype=bool)
            contam_mask[all_edge] = contam
            count = desaturate_dominant_channel(data, contam_mask, colorkey_rgb)
            if count > 0:
                stats["desaturated"] = count
                modified = True

        if modified:
            Image.fromarray(data).save(upscaled_path, "PNG")
        return modified, stats

    filled_rgb = data[:, :, :3].copy()
    fill_mask = interior.copy()
    total_fixed = 0

    # More iterations to reach thin appendages (horns, wand tips)
    max_iters = edge_layers * 4 + 16
    for iteration in range(max_iters):
        remaining = all_edge & ~fill_mask
        if not np.any(remaining):
            break

        new_fill = np.zeros_like(fill_mask)
        new_rgb = filled_rgb.copy()

        for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            if dy == -1:
                src_y, dst_y = slice(1, h), slice(0, h - 1)
            elif dy == 1:
                src_y, dst_y = slice(0, h - 1), slice(1, h)
            else:
                src_y, dst_y = slice(None), slice(None)

            if dx == -1:
                src_x, dst_x = slice(1, w), slice(0, w - 1)
            elif dx == 1:
                src_x, dst_x = slice(0, w - 1), slice(1, w)
            else:
                src_x, dst_x = slice(None), slice(None)

            can_fill = ~fill_mask[dst_y, dst_x] & fill_mask[src_y, src_x]
            should_fill = can_fill & all_edge[dst_y, dst_x]

            if np.any(should_fill):
                new_rgb[dst_y, dst_x][should_fill] = filled_rgb[src_y, src_x][should_fill]
                new_fill[dst_y, dst_x] |= should_fill

        filled_rgb = new_rgb
        fill_mask = fill_mask | new_fill

    # Apply: replace all edge pixels that we could fill
    replace = all_edge & fill_mask
    if np.any(replace):
        old_rgb = data[:, :, :3][replace].astype(int)
        new_vals = filled_rgb[replace].astype(int)
        actually_changed = np.any(old_rgb != new_vals, axis=1)
        total_fixed = int(np.sum(actually_changed))
        if total_fixed > 0:
            data[replace, :3] = filled_rgb[replace]
            modified = True

    # Step 5: Unfilled edge pixels -- desaturate instead of deleting.
    # These are thin tips/appendages too far from interior for propagation.
    # Desaturating the CK-dominant channel removes the green cast while
    # preserving the pixel.  NEVER set alpha=0.
    unfilled_edge = all_edge & ~fill_mask
    if np.any(unfilled_edge):
        uf_rgb = data[:, :, :3][unfilled_edge]
        uf_contam = is_contaminated(uf_rgb, colorkey_rgb, threshold=200)
        if np.any(uf_contam):
            contam_mask = np.zeros((h, w), dtype=bool)
            contam_mask[unfilled_edge] = uf_contam
            count = desaturate_dominant_channel(data, contam_mask, colorkey_rgb)
            stats["desaturated"] = count
            if count > 0:
                modified = True

    # NO erosion pass -- v3's erosion deleted real sprite content on small sprites.
    # The runtime defringe in Sprite.cpp::LoadToGPU() handles any remaining
    # edge contamination at load time without modifying the source PNGs.

    stats["edge_fixed"] = total_fixed

    if modified:
        Image.fromarray(data).save(upscaled_path, "PNG")

    return modified, stats


def process_category(cat_name: str, alpha_cut: int = 128,
                     test_mode: bool = False, test_limit: int = 3) -> tuple:
    """Process all sprites in one category. Returns (total, modified, skipped)."""
    inp_dir = GIGAPIXEL_INPUT / cat_name
    out_dir = GIGAPIXEL_OUTPUT / cat_name

    if not inp_dir.is_dir() or not out_dir.is_dir():
        return 0, 0, 0

    total = 0
    modified = 0
    skipped = 0

    sprites = sorted(inp_dir.glob("sprite_*.png"))
    if test_mode:
        sprites = sprites[:test_limit]

    for png in sprites:
        upscaled = out_dir / png.name
        if not upscaled.exists():
            skipped += 1
            continue

        total += 1
        try:
            ck = get_colorkey(png)

            if test_mode:
                # Save before image
                TEST_DIR.mkdir(exist_ok=True)
                before_path = TEST_DIR / f"{cat_name}_{png.stem}_BEFORE_v4.png"
                Image.open(upscaled).save(before_path)

            did_modify, stats = defringe_sprite(upscaled, ck, alpha_cut)

            if test_mode:
                after_path = TEST_DIR / f"{cat_name}_{png.stem}_AFTER_v4.png"
                Image.open(upscaled).save(after_path)
                print(f"  {cat_name}/{png.name}: layers={stats['edge_layers']}, "
                      f"snapped={stats['semi_snapped']}, "
                      f"edge_fixed={stats['edge_fixed']}, "
                      f"desaturated={stats['desaturated']}")

            if did_modify:
                modified += 1
        except Exception as e:
            print(f"  ERROR {cat_name}/{png.name}: {e}")
            skipped += 1

    return total, modified, skipped


def main():
    parser = argparse.ArgumentParser(
        description="Remove color-key fringe from upscaled sprites (v4 — safe)")
    parser.add_argument("--cat", nargs="*", help="Specific categories to process")
    parser.add_argument("--alpha-cut", type=int, default=128,
                        help="Alpha cutoff for binary snap (default: 128)")
    parser.add_argument("--test", action="store_true",
                        help="Test mode: process 3 sprites per category, save before/after")
    parser.add_argument("--test-limit", type=int, default=3,
                        help="Number of sprites per category in test mode (default: 3)")
    args = parser.parse_args()

    start = time.time()

    # Discover categories
    if args.cat:
        categories = args.cat
    else:
        categories = sorted([
            d.name for d in GIGAPIXEL_OUTPUT.iterdir()
            if d.is_dir() and d.name not in UI_CATEGORIES
            and (GIGAPIXEL_INPUT / d.name).is_dir()
        ])

    print("=" * 60)
    print(f"Defringe v4 (safe) -- {len(categories)} categories")
    print(f"Alpha cutoff: {args.alpha_cut}")
    print("Edge layers: auto (1 for <48px, 2 for <96px, 3 for >=96px)")
    print("Erosion: DISABLED (desaturation fallback instead)")
    if args.test:
        print(f"TEST MODE: {args.test_limit} sprites per category, before/after saved")
    print("=" * 60)

    grand_total = 0
    grand_modified = 0
    grand_skipped = 0

    for i, cat in enumerate(categories):
        total, mod, skip = process_category(cat, args.alpha_cut,
                                            test_mode=args.test,
                                            test_limit=args.test_limit)
        grand_total += total
        grand_modified += mod
        grand_skipped += skip

        if mod > 0 and not args.test:
            print(f"  [{cat}] {mod}/{total} defringed")
        if (i + 1) % 50 == 0 and not args.test:
            print(f"  [{i + 1}/{len(categories)}] processed...")

    elapsed = time.time() - start
    mins = int(elapsed // 60)
    secs = int(elapsed % 60)

    print(f"\n{'=' * 60}")
    print(f"COMPLETE -- {mins}m {secs}s")
    print(f"Total sprites: {grand_total}")
    print(f"Modified:      {grand_modified}")
    print(f"Skipped:       {grand_skipped}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
