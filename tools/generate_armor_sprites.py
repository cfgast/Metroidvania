#!/usr/bin/env python3
"""Generate armored player sprite sheets (tiers 2-5) from the base sheet.

Each tier progressively adds knight armor pieces to the base character by
detecting the character silhouette per frame and painting armor overlays
in the appropriate body regions.

Usage:
    python tools/generate_armor_sprites.py [--base assets/player_spritesheet.png]

Output:
    assets/player_armor_2.png  -- Boots
    assets/player_armor_3.png  -- Boots + Leg plates
    assets/player_armor_4.png  -- Boots + Legs + Chestplate
    assets/player_armor_5.png  -- Full knight armor
"""

import argparse
import os
import sys
from pathlib import Path

from PIL import Image

# Sprite sheet layout: 50x50 frames
FRAME_W = 50
FRAME_H = 50
ROWS = [
    ("idle",             4),
    ("run-right",        4),
    ("run-left",         4),
    ("jump",             2),
    ("fall",             2),
    ("wall-slide-right", 2),
    ("wall-slide-left",  2),
    ("dash-right",       3),
    ("dash-left",        3),
]

# Armor colors (RGBA)
METAL_DARK   = (90, 95, 105, 255)     # Dark steel edges/outlines
METAL_MID    = (140, 148, 160, 255)    # Mid-tone steel
METAL_LIGHT  = (185, 195, 210, 255)   # Lighter steel
METAL_BRIGHT = (210, 220, 235, 255)   # Bright/polished steel
GOLD_ACCENT  = (200, 180, 80, 255)    # Gold trim for tier 5
RIVET        = (70, 75, 85, 255)      # Rivet/dark accent dots


def find_silhouette_bbox(img, fx, fy):
    """Find bounding box of non-transparent pixels within a single frame."""
    min_x, min_y = FRAME_W, FRAME_H
    max_x, max_y = -1, -1
    pixels = img.load()
    for y in range(fy, fy + FRAME_H):
        for x in range(fx, fx + FRAME_W):
            r, g, b, a = pixels[x, y]
            if a > 20:
                lx, ly = x - fx, y - fy
                min_x = min(min_x, lx)
                min_y = min(min_y, ly)
                max_x = max(max_x, lx)
                max_y = max(max_y, ly)
    if max_x < 0:
        return None
    return (min_x, min_y, max_x, max_y)


def is_opaque(pixels, x, y, fx, fy, img_w, img_h):
    """Check if a pixel at (fx+x, fy+y) is opaque (part of the character)."""
    abs_x, abs_y = fx + x, fy + y
    if abs_x < 0 or abs_y < 0 or abs_x >= img_w or abs_y >= img_h:
        return False
    return pixels[abs_x, abs_y][3] > 20


def set_pixel(pixels, fx, fy, lx, ly, color, img_w, img_h):
    """Set pixel at frame-local (lx, ly) if within bounds."""
    abs_x, abs_y = fx + lx, fy + ly
    if 0 <= abs_x < img_w and 0 <= abs_y < img_h:
        pixels[abs_x, abs_y] = color


def paint_region(pixels, fx, fy, bbox, y_start_frac, y_end_frac,
                 fill_color, edge_color, img_w, img_h, inset=1):
    """Paint armor onto a vertical slice of the character silhouette.

    y_start_frac / y_end_frac are fractions of the bounding box height
    (0.0 = top, 1.0 = bottom). Only opaque pixels within that vertical
    band are recolored, with edge pixels getting the edge_color.
    """
    bx0, by0, bx1, by1 = bbox
    bh = by1 - by0 + 1
    y_lo = by0 + int(bh * y_start_frac)
    y_hi = by0 + int(bh * y_end_frac)

    for ly in range(y_lo, y_hi + 1):
        # Find leftmost and rightmost opaque pixel on this row
        left, right = None, None
        for lx in range(bx0, bx1 + 1):
            if is_opaque(pixels, lx, ly, fx, fy, img_w, img_h):
                if left is None:
                    left = lx
                right = lx
        if left is None:
            continue

        for lx in range(left + inset, right - inset + 1):
            if not is_opaque(pixels, lx, ly, fx, fy, img_w, img_h):
                continue
            # Edge pixels get darker outline color
            is_edge = (lx <= left + inset or lx >= right - inset or
                       ly <= y_lo + 1 or ly >= y_hi - 1)
            color = edge_color if is_edge else fill_color
            set_pixel(pixels, fx, fy, lx, ly, color, img_w, img_h)


def add_plate_lines(pixels, fx, fy, bbox, y_start_frac, y_end_frac,
                    line_color, img_w, img_h, spacing=3):
    """Add horizontal plate-edge lines within a region for visual detail."""
    bx0, by0, bx1, by1 = bbox
    bh = by1 - by0 + 1
    y_lo = by0 + int(bh * y_start_frac)
    y_hi = by0 + int(bh * y_end_frac)

    for ly in range(y_lo, y_hi + 1):
        if (ly - y_lo) % spacing != 0:
            continue
        for lx in range(bx0 + 2, bx1 - 1):
            if is_opaque(pixels, lx, ly, fx, fy, img_w, img_h):
                set_pixel(pixels, fx, fy, lx, ly, line_color, img_w, img_h)


def add_rivets(pixels, fx, fy, bbox, y_frac, x_positions,
               color, img_w, img_h):
    """Add rivet dots at specific fractional x-positions along a y-line."""
    bx0, by0, bx1, by1 = bbox
    bh = by1 - by0 + 1
    bw = bx1 - bx0 + 1
    ly = by0 + int(bh * y_frac)
    for xf in x_positions:
        lx = bx0 + int(bw * xf)
        if is_opaque(pixels, lx, ly, fx, fy, img_w, img_h):
            set_pixel(pixels, fx, fy, lx, ly, color, img_w, img_h)


def apply_boots(img):
    """Tier 2: Armored boots on lower legs/feet."""
    pixels = img.load()
    w, h = img.size
    row_y = 0
    for _, frame_count in ROWS:
        for col in range(frame_count):
            fx, fy = col * FRAME_W, row_y
            bbox = find_silhouette_bbox(img, fx, fy)
            if bbox is None:
                continue
            # Boots cover bottom 25% of character
            paint_region(pixels, fx, fy, bbox, 0.78, 1.0,
                         METAL_MID, METAL_DARK, w, h)
            # Boot top edge highlight
            bx0, by0, bx1, by1 = bbox
            bh = by1 - by0 + 1
            edge_y = by0 + int(bh * 0.78)
            for lx in range(bx0 + 1, bx1):
                if is_opaque(pixels, lx, edge_y, fx, fy, w, h):
                    set_pixel(pixels, fx, fy, lx, edge_y, METAL_LIGHT, w, h)
        row_y += FRAME_H


def apply_leg_plates(img):
    """Tier 3: Full leg armor (greaves + cuisses) on top of boots."""
    apply_boots(img)
    pixels = img.load()
    w, h = img.size
    row_y = 0
    for _, frame_count in ROWS:
        for col in range(frame_count):
            fx, fy = col * FRAME_W, row_y
            bbox = find_silhouette_bbox(img, fx, fy)
            if bbox is None:
                continue
            # Leg plates cover from 55% to 78% of character height
            paint_region(pixels, fx, fy, bbox, 0.55, 0.78,
                         METAL_LIGHT, METAL_DARK, w, h)
            # Add plate lines for visual distinction
            add_plate_lines(pixels, fx, fy, bbox, 0.55, 0.78,
                            METAL_DARK, w, h, spacing=4)
            # Knee rivets
            add_rivets(pixels, fx, fy, bbox, 0.58,
                       [0.35, 0.65], RIVET, w, h)
        row_y += FRAME_H


def apply_chestplate(img):
    """Tier 4: Breastplate + shoulder guards on top of legs + boots."""
    apply_leg_plates(img)
    pixels = img.load()
    w, h = img.size
    row_y = 0
    for _, frame_count in ROWS:
        for col in range(frame_count):
            fx, fy = col * FRAME_W, row_y
            bbox = find_silhouette_bbox(img, fx, fy)
            if bbox is None:
                continue
            # Chestplate covers torso: 25% to 55%
            paint_region(pixels, fx, fy, bbox, 0.25, 0.55,
                         METAL_MID, METAL_DARK, w, h)
            # Horizontal plate lines on torso
            add_plate_lines(pixels, fx, fy, bbox, 0.30, 0.52,
                            METAL_DARK, w, h, spacing=3)
            # Shoulder guards: slightly above torso region, wider
            paint_region(pixels, fx, fy, bbox, 0.22, 0.30,
                         METAL_LIGHT, METAL_DARK, w, h, inset=0)
            # Center chest rivet/emblem
            add_rivets(pixels, fx, fy, bbox, 0.38,
                       [0.45, 0.55], RIVET, w, h)
        row_y += FRAME_H


def apply_full_armor(img):
    """Tier 5: Full knight armor (gauntlets + helm) on top of everything."""
    apply_chestplate(img)
    pixels = img.load()
    w, h = img.size
    row_y = 0
    for _, frame_count in ROWS:
        for col in range(frame_count):
            fx, fy = col * FRAME_W, row_y
            bbox = find_silhouette_bbox(img, fx, fy)
            if bbox is None:
                continue
            # Helm covers head: top 22% of character
            paint_region(pixels, fx, fy, bbox, 0.0, 0.22,
                         METAL_BRIGHT, METAL_DARK, w, h, inset=0)
            # Visor slit (dark line across the face area)
            bx0, by0, bx1, by1 = bbox
            bh = by1 - by0 + 1
            bw = bx1 - bx0 + 1
            visor_y = by0 + int(bh * 0.12)
            for lx in range(bx0 + int(bw * 0.25), bx0 + int(bw * 0.75)):
                if is_opaque(pixels, lx, visor_y, fx, fy, w, h):
                    set_pixel(pixels, fx, fy, lx, visor_y,
                              (30, 30, 40, 255), w, h)

            # Gauntlets on arms: paint the horizontal edges of the
            # torso region with brighter metal to suggest arm coverage.
            # Re-paint the outer columns of the torso area.
            torso_y_lo = by0 + int(bh * 0.28)
            torso_y_hi = by0 + int(bh * 0.52)
            for ly in range(torso_y_lo, torso_y_hi + 1):
                left, right = None, None
                for lx in range(bx0, bx1 + 1):
                    if is_opaque(pixels, lx, ly, fx, fy, w, h):
                        if left is None:
                            left = lx
                        right = lx
                if left is None:
                    continue
                # Paint 2-3 px from each edge as gauntlet
                for lx in range(left, min(left + 3, right + 1)):
                    set_pixel(pixels, fx, fy, lx, ly, METAL_BRIGHT, w, h)
                for lx in range(max(right - 2, left), right + 1):
                    set_pixel(pixels, fx, fy, lx, ly, METAL_BRIGHT, w, h)

            # Gold trim on helm crest and belt line for tier 5 distinction
            crest_y = by0 + int(bh * 0.02)
            for lx in range(bx0 + int(bw * 0.3), bx0 + int(bw * 0.7)):
                if is_opaque(pixels, lx, crest_y, fx, fy, w, h):
                    set_pixel(pixels, fx, fy, lx, crest_y, GOLD_ACCENT, w, h)
            belt_y = by0 + int(bh * 0.54)
            for lx in range(bx0 + 1, bx1):
                if is_opaque(pixels, lx, belt_y, fx, fy, w, h):
                    set_pixel(pixels, fx, fy, lx, belt_y, GOLD_ACCENT, w, h)
        row_y += FRAME_H


TIER_GENERATORS = {
    2: ("assets/player_armor_2.png", apply_boots),
    3: ("assets/player_armor_3.png", apply_leg_plates),
    4: ("assets/player_armor_4.png", apply_chestplate),
    5: ("assets/player_armor_5.png", apply_full_armor),
}


def main():
    parser = argparse.ArgumentParser(
        description="Generate armored player sprite sheets."
    )
    parser.add_argument(
        "--base",
        default="assets/player_spritesheet.png",
        help="Path to the base player sprite sheet",
    )
    args = parser.parse_args()

    # Resolve paths relative to the project root (parent of tools/)
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    base_path = project_root / args.base

    if not base_path.exists():
        print(f"ERROR: Base sprite sheet not found: {base_path}", file=sys.stderr)
        sys.exit(1)

    base_img = Image.open(base_path).convert("RGBA")
    print(f"Loaded base sheet: {base_path} ({base_img.size[0]}x{base_img.size[1]})")

    for tier, (out_rel, gen_func) in sorted(TIER_GENERATORS.items()):
        out_path = project_root / out_rel
        sheet = base_img.copy()
        gen_func(sheet)
        sheet.save(out_path)
        print(f"  Tier {tier}: {out_path.name}")

    print("Done — generated 4 armor sprite sheets.")


if __name__ == "__main__":
    main()
