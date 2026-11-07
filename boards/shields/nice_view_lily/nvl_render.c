#include "nvl_render.h"
#include "nvl_assets.h"

#include <string.h>

static uint8_t (*fb)[NVL_W];

static void px(int x, int y, uint8_t c) {
    if (x >= 0 && x < NVL_W && y >= 0 && y < NVL_H) {
        fb[y][x] = c;
    }
}

static void fill_rect(int x, int y, int w, int h, uint8_t c) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            px(xx, yy, c);
        }
    }
}

/* Is pixel (xx, yy), relative to the box's top-left, inside a w*h box with corner radius r? */
static bool in_rrect(int xx, int yy, int w, int h, int r) {
    if (xx < 0 || yy < 0 || xx >= w || yy >= h) {
        return false;
    }
    int dx = r - xx - 1;
    if (xx - (w - r) > dx) {
        dx = xx - (w - r);
    }
    int dy = r - yy - 1;
    if (yy - (h - r) > dy) {
        dy = yy - (h - r);
    }
    if (dx <= 0 || dy <= 0) {
        return true;
    }
    int ex = 2 * dx - 1, ey = 2 * dy - 1, er = 2 * r - 1;
    return ex * ex + ey * ey <= er * er + 2;
}

static void fill_rrect(int x, int y, int w, int h, int r, uint8_t c) {
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            if (in_rrect(xx, yy, w, h, r)) {
                px(x + xx, y + yy, c);
            }
        }
    }
}

/* 1px outline; dashed draws every other pair of pixels. */
static void outline_rrect(int x, int y, int w, int h, int r, bool dashed) {
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            bool ring = in_rrect(xx, yy, w, h, r) &&
                        !in_rrect(xx - 1, yy - 1, w - 2, h - 2, r > 1 ? r - 1 : 0);
            if (ring && (!dashed || (((x + xx + y + yy) >> 1) & 1) == 0)) {
                px(x + xx, y + yy, 1);
            }
        }
    }
}

static bool in_circle(int xx, int yy, int d) {
    int ex = 2 * xx + 1 - d, ey = 2 * yy + 1 - d;
    return ex * ex + ey * ey <= d * d + d / 2;
}

static void draw_circle(int x, int y, int d, bool filled, bool dashed) {
    for (int yy = 0; yy < d; yy++) {
        for (int xx = 0; xx < d; xx++) {
            if (!in_circle(xx, yy, d)) {
                continue;
            }
            bool ring = !in_circle(xx - 1, yy - 1, d - 2) || xx == 0 || yy == 0;
            if (filled || (ring && (!dashed || (((xx + yy) >> 1) & 1) == 0))) {
                px(x + xx, y + yy, 1);
            }
        }
    }
}

static const uint8_t *glyph(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        ch = ch - 'a' + 'A';
    }
    if (ch < 32 || ch > 95) {
        ch = '?';
    }
    return nvl_font[ch - 32];
}

static int text_width(const char *s, int scale) {
    int n = strlen(s);
    return n ? (n * 6 - 1) * scale : 0;
}

static void draw_text(int x, int y, const char *s, int scale, uint8_t c) {
    for (; *s; s++, x += 6 * scale) {
        const uint8_t *g = glyph(*s);
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (g[row] & (1 << (4 - col))) {
                    fill_rect(x + col * scale, y + row * scale, scale, scale, c);
                }
            }
        }
    }
}

static void draw_text_centered(int cx, int y, const char *s, int scale, uint8_t c) {
    draw_text(cx - text_width(s, scale) / 2, y, s, scale, c);
}

static void draw_sprite(int x, int y, const struct nvl_sprite *sp) {
    for (int row = 0; row < sp->h; row++) {
        for (int col = 0; col < sp->w; col++) {
            uint16_t bit = 1 << (15 - col);
            if (sp->line[row] & bit) {
                px(x + col, y + row, 1);
            } else if (sp->hole[row] & bit) {
                px(x + col, y + row, 0);
            }
        }
    }
}

static void itoa_small(int v, char *out) {
    if (v >= 100) {
        *out++ = '0' + v / 100;
        v %= 100;
        *out++ = '0' + v / 10;
    } else if (v >= 10) {
        *out++ = '0' + v / 10;
    }
    *out++ = '0' + v % 10;
    *out = 0;
}

/* ---- Batteries ---- */

#define BAT_Y 10
#define BAT_W 20
#define BAT_H 42

static void draw_battery(int x, uint8_t level, bool present, bool charging, const char *label) {
    int cx = x + BAT_W / 2;

    if (present) {
        fill_rrect(cx - 4, BAT_Y - 4, 8, 6, 1, 1); /* nub */
        fill_rrect(x, BAT_Y, BAT_W, BAT_H, 4, 1);
        fill_rrect(x + 1, BAT_Y + 1, BAT_W - 2, BAT_H - 2, 3, 0);

        int inner_h = BAT_H - 4;
        int h = (inner_h * (level > 100 ? 100 : level) + 50) / 100;
        if (h > 0) {
            fill_rrect(x + 2, BAT_Y + 2 + inner_h - h, BAT_W - 4, h, h < 4 ? h / 2 : 2, 1);
        }
        if (charging) {
            draw_sprite(cx - nvl_bolt.w / 2, BAT_Y + (BAT_H - nvl_bolt.h) / 2, &nvl_bolt);
        }
    } else {
        outline_rrect(cx - 4, BAT_Y - 4, 8, 6, 1, true);
        outline_rrect(x, BAT_Y, BAT_W, BAT_H, 4, true);
    }

    draw_text_centered(cx, 55, label, 2, 1);

    char pct[5];
    if (present) {
        itoa_small(level, pct);
    } else {
        strcpy(pct, "--");
    }
    draw_text_centered(cx, 72, pct, 1, 1);
}

/* ---- Connection pill ---- */

static void draw_output(const struct nvl_state *s) {
    char text[6];
    bool filled;

    if (s->output == NVL_OUT_USB) {
        strcpy(text, "USB");
        filled = true;
    } else if (!s->ble_bonded) {
        strcpy(text, "PAIR");
        filled = false;
    } else {
        strcpy(text, "BT");
        filled = s->ble_connected;
    }
    if (s->output == NVL_OUT_BLE && s->profile >= 3) {
        /* Profiles 4+ have no circle below, so name the profile in the pill. */
        int n = strlen(text);
        text[n] = '1' + s->profile;
        text[n + 1] = 0;
    }

    const int x = 14, y = 82, w = 40, h = 11;
    if (filled) {
        fill_rrect(x, y, w, h, 5, 1);
    } else {
        outline_rrect(x, y, w, h, 5, false);
    }
    draw_text_centered(x + w / 2, y + 2, text, 1, filled ? 0 : 1);
}

/* ---- Profile circles ---- */

static void draw_profiles(const struct nvl_state *s) {
    for (int i = 0; i < 3; i++) {
        int x = 7 + i * 21, y = 97, d = 13;
        char num[2] = {'1' + i, 0};
        bool active = s->output == NVL_OUT_BLE && s->profile == i;
        bool bonded = s->profile_bonded & (1 << i);

        if (active || (s->output == NVL_OUT_USB && s->profile == i)) {
            draw_circle(x, y, d, true, false);
            draw_text(x + 4, y + 3, num, 1, 0);
        } else {
            draw_circle(x, y, d, false, !bonded);
            draw_text(x + 4, y + 3, num, 1, 1);
        }
    }
}

/* ---- Layer ---- */

static bool name_eq(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
        char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
        if (ca != cb) {
            return false;
        }
    }
    return *a == *b;
}

static void draw_layer(const struct nvl_state *s) {
    const int bx = 17, by = 115, bs = 34;
    char name[9];

    if (s->layer_name && s->layer_name[0]) {
        strncpy(name, s->layer_name, 8);
        name[8] = 0;
    } else {
        strcpy(name, "LAYER");
        itoa_small(s->layer_index, name + 5);
    }

    fill_rrect(bx, by, bs, bs, 7, 1);

    const struct nvl_icon *icon = NULL;
    for (int i = 0; s->layer_name && i < nvl_icon_count; i++) {
        if (name_eq(nvl_icons[i].name, s->layer_name)) {
            icon = &nvl_icons[i];
        }
    }

    if (icon) {
        int ix = bx + (bs - NVL_ICON_SIZE) / 2, iy = by + (bs - NVL_ICON_SIZE) / 2;
        for (int row = 0; row < NVL_ICON_SIZE; row++) {
            for (int col = 0; col < NVL_ICON_SIZE; col++) {
                if (icon->rows[row] & (1UL << (NVL_ICON_SIZE - 1 - col))) {
                    px(ix + col, iy + row, 0);
                }
            }
        }
    } else {
        /* No icon for this layer: show its first two letters big. */
        char initials[3] = {name[0], name[1], 0};
        if (!s->layer_name) {
            itoa_small(s->layer_index, initials);
        }
        draw_text_centered(bx + bs / 2, by + 10, initials, 2, 0);
    }

    draw_text_centered(NVL_W / 2, 152, name, 1, 1);
}

void nvl_render(const struct nvl_state *s, uint8_t buf[NVL_H][NVL_W]) {
    fb = buf;
    memset(buf, 0, NVL_H * NVL_W);

    draw_battery(7, s->bat_left, true, s->charging, "L");
    draw_battery(41, s->bat_right, s->right_connected, false, "R");
    draw_output(s);
    draw_profiles(s);
    draw_layer(s);
}
