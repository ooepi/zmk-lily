"""Generate nvl_assets.c (pixel font, layer icons, charging bolt) for the nice_view_lily screen.

Run from anywhere:  python gen_assets.py
Edit the pixel art below ('#' = lit pixel), re-run, and rebuild the firmware.
"""

from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "nvl_assets.c"

# 5x7 font. Each glyph is 7 rows of 5 columns.
FONT = {
    " ": "..... ..... ..... ..... ..... ..... .....",
    "0": ".###. #...# #..## #.#.# ##..# #...# .###.",
    "1": "..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###.",
    "2": ".###. #...# ....# ...#. ..#.. .#... #####",
    "3": "##### ...#. ..#.. ...#. ....# #...# .###.",
    "4": "...#. ..##. .#.#. #..#. ##### ...#. ...#.",
    "5": "##### #.... ####. ....# ....# #...# .###.",
    "6": "..##. .#... #.... ####. #...# #...# .###.",
    "7": "##### ....# ...#. ..#.. .#... .#... .#...",
    "8": ".###. #...# #...# .###. #...# #...# .###.",
    "9": ".###. #...# #...# .#### ....# ...#. .##..",
    "A": ".###. #...# #...# ##### #...# #...# #...#",
    "B": "####. #...# #...# ####. #...# #...# ####.",
    "C": ".###. #...# #.... #.... #.... #...# .###.",
    "D": "###.. #..#. #...# #...# #...# #..#. ###..",
    "E": "##### #.... #.... ####. #.... #.... #####",
    "F": "##### #.... #.... ####. #.... #.... #....",
    "G": ".###. #...# #.... #.### #...# #...# .####",
    "H": "#...# #...# #...# ##### #...# #...# #...#",
    "I": ".###. ..#.. ..#.. ..#.. ..#.. ..#.. .###.",
    "J": "..### ...#. ...#. ...#. ...#. #..#. .##..",
    "K": "#...# #..#. #.#.. ##... #.#.. #..#. #...#",
    "L": "#.... #.... #.... #.... #.... #.... #####",
    "M": "#...# ##.## #.#.# #.#.# #...# #...# #...#",
    "N": "#...# #...# ##..# #.#.# #..## #...# #...#",
    "O": ".###. #...# #...# #...# #...# #...# .###.",
    "P": "####. #...# #...# ####. #.... #.... #....",
    "Q": ".###. #...# #...# #...# #.#.# #..#. .##.#",
    "R": "####. #...# #...# ####. #.#.. #..#. #...#",
    "S": ".#### #.... #.... .###. ....# ....# ####.",
    "T": "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..",
    "U": "#...# #...# #...# #...# #...# #...# .###.",
    "V": "#...# #...# #...# #...# #...# .#.#. ..#..",
    "W": "#...# #...# #...# #.#.# #.#.# #.#.# .#.#.",
    "X": "#...# #...# .#.#. ..#.. .#.#. #...# #...#",
    "Y": "#...# #...# .#.#. ..#.. ..#.. ..#.. ..#..",
    "Z": "##### ....# ...#. ..#.. .#... #.... #####",
    "%": "##... ##..# ...#. ..#.. .#... #..## ...##",
    "-": "..... ..... ..... ##### ..... ..... .....",
    "_": "..... ..... ..... ..... ..... ..... #####",
    "?": ".###. #...# ....# ...#. ..#.. ..... ..#..",
    "!": "..#.. ..#.. ..#.. ..#.. ..#.. ..... ..#..",
}

ICON = 24  # icons are 24x24


def blank():
    return [[0] * ICON for _ in range(ICON)]


def from_art(art):
    rows = [r for r in art.strip("\n").split("\n")]
    assert len(rows) == ICON, f"icon needs {ICON} rows, has {len(rows)}"
    grid = blank()
    for y, row in enumerate(rows):
        row = row.strip()
        assert len(row) == ICON, f"row {y} is {len(row)} wide: {row!r}"
        for x, ch in enumerate(row):
            grid[y][x] = 1 if ch == "#" else 0
    return grid


def rrect(grid, x, y, w, h, r, val=1):
    """Filled rounded rectangle (same corner maths as the C renderer)."""
    for yy in range(h):
        for xx in range(w):
            dx = max(r - xx - 1, xx - (w - r), 0)
            dy = max(r - yy - 1, yy - (h - r), 0)
            if dx and dy:
                # distance from corner circle centre, measured at pixel centres
                if (2 * dx - 1) ** 2 + (2 * dy - 1) ** 2 > (2 * r - 1) ** 2 + 2:
                    continue
            if 0 <= x + xx < ICON and 0 <= y + yy < ICON:
                grid[y + yy][x + xx] = val


def rect(grid, x, y, w, h, val=1):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            grid[yy][xx] = val


def icon_base():
    return from_art("""
........................
...........##...........
..........####..........
.........######.........
........###..###........
.......###....###.......
......###......###......
.....###........###.....
....###..........###....
...###............###...
..###..............###..
.###................###.
....##............##....
....##............##....
....##............##....
....##....####....##....
....##....####....##....
....##....####....##....
....##....####....##....
....##....####....##....
....################....
....################....
........................
........................
""")


def icon_nav():
    g = blank()
    for i in range(5):  # four arrow heads, 2..10 px wide
        half = i + 1
        for k in range(12 - half, 12 + half):
            g[1 + i][k] = 1  # up
            g[22 - i][k] = 1  # down
            g[k][1 + i] = 1  # left
            g[k][22 - i] = 1  # right
    rrect(g, 8, 8, 8, 8, 2)
    rrect(g, 10, 10, 4, 4, 1, 0)
    return g


def icon_prog():
    brace = [
        ".....###",
        "....####",
        "....##..",
        "....##..",
        "....##..",
        "....##..",
        "....##..",
        "...##...",
        ".###....",
        "##......",
        "##......",
        ".###....",
        "...##...",
        "....##..",
        "....##..",
        "....##..",
        "....##..",
        "....##..",
        "....####",
        ".....###",
    ]
    g = blank()
    for y, row in enumerate(brace):
        for x, ch in enumerate(row):
            if ch == "#":
                g[2 + y][x] = 1  # left brace, cols 0-7
                g[2 + y][23 - x] = 1  # right brace, mirrored
    rrect(g, 10, 6, 4, 4, 1)  # semicolon dot
    rrect(g, 10, 13, 4, 4, 1)  # semicolon comma head
    rect(g, 11, 17, 2, 2)
    rect(g, 10, 19, 2, 1)
    return g


def icon_mous():
    g = blank()
    rrect(g, 5, 1, 14, 22, 7)
    rrect(g, 7, 3, 10, 18, 5, 0)
    rect(g, 6, 10, 12, 2)  # button line
    rect(g, 11, 2, 2, 8)  # split between buttons
    rrect(g, 10, 4, 4, 5, 2, 1)  # scroll wheel
    return g


def icon_num():
    g = blank()
    for row in range(3):
        for col in range(3):
            if row == 2 and col < 2:
                continue
            rrect(g, 1 + col * 8, 1 + row * 8, 6, 6, 2)
    rrect(g, 1, 17, 14, 6, 2)  # wide 0 key
    return g


def icon_qwer():
    g = blank()
    rrect(g, 0, 5, 24, 11, 5)  # body
    rrect(g, 0, 9, 9, 11, 4)  # left grip
    rrect(g, 15, 9, 9, 11, 4)  # right grip
    rect(g, 3, 10, 6, 2, 0)  # d-pad
    rect(g, 5, 8, 2, 6, 0)
    rrect(g, 16, 7, 3, 3, 1, 0)  # buttons
    rrect(g, 19, 10, 3, 3, 1, 0)
    return g


ICONS = {
    "BASE": icon_base,
    "NAV": icon_nav,
    "PROG": icon_prog,
    "MOUS": icon_mous,
    "NUM": icon_num,
    "QWER": icon_qwer,
}

# Charging bolt: '#' = outline, 'o' = cut-out (drawn in background colour).
BOLT = """
.....######
.....#oooo#
....#oooo#.
....#ooo#..
...#oooo#..
...#ooo#...
..#oooo####
..#ooooooo#
.####oooo#.
....#ooo#..
...#ooo#...
...#oo#....
..#ooo#....
..#oo#.....
.#oo#......
.#o#.......
.##........
"""


def rows_to_c(grid, width):
    vals = []
    for row in grid:
        v = 0
        for x in range(width):
            if row[x]:
                v |= 1 << (width - 1 - x)
        vals.append(f"0x{v:0{(width + 3) // 4}x}")
    return vals


def main():
    out = []
    out.append("/* Generated by tools/gen_assets.py - edit the pixel art there, not here. */")
    out.append('#include "nvl_assets.h"')
    out.append("")
    out.append("/* 5x7 font for ASCII 32..95, bit 4 = left column. */")
    out.append("const uint8_t nvl_font[64][7] = {")
    for code in range(32, 96):
        ch = chr(code)
        art = FONT.get(ch)
        if art is None:
            out.append(f"    {{0}}, /* {ch!r} */")
            continue
        rows = art.split()
        assert len(rows) == 7 and all(len(r) == 5 for r in rows), ch
        vals = []
        for r in rows:
            v = sum(1 << (4 - i) for i, c in enumerate(r) if c == "#")
            vals.append(f"0x{v:02x}")
        out.append(f"    {{{', '.join(vals)}}}, /* {ch!r} */")
    out.append("};")
    out.append("")

    out.append("const struct nvl_icon nvl_icons[] = {")
    for name, fn in ICONS.items():
        vals = rows_to_c(fn(), ICON)
        out.append(f'    {{"{name}", {{')
        for i in range(0, ICON, 6):
            out.append("        " + ", ".join(vals[i : i + 6]) + ",")
        out.append("    }},")
    out.append("};")
    out.append("const int nvl_icon_count = sizeof(nvl_icons) / sizeof(nvl_icons[0]);")
    out.append("")

    bolt = [r for r in BOLT.strip("\n").split("\n")]
    w = max(len(r) for r in bolt)
    assert len(bolt) <= 20 and w <= 16
    line = [[1 if (x < len(r) and r[x] == "#") else 0 for x in range(16)] for r in bolt]
    hole = [[1 if (x < len(r) and r[x] == "o") else 0 for x in range(16)] for r in bolt]
    out.append(f"const struct nvl_sprite nvl_bolt = {{{w}, {len(bolt)},")
    out.append("    {" + ", ".join(rows_to_c(line, 16)) + "},")
    out.append("    {" + ", ".join(rows_to_c(hole, 16)) + "}};")
    out.append("")

    OUT.write_text("\n".join(out), newline="\n")
    print(f"wrote {OUT}")

    # ASCII preview of the icons, handy when editing.
    for name, fn in ICONS.items():
        print(name)
        for row in fn():
            print("".join("#" if p else "." for p in row))


if __name__ == "__main__":
    main()
