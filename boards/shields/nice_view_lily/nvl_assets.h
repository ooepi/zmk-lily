#pragma once

#include <stdint.h>

#define NVL_ICON_SIZE 24

struct nvl_icon {
    const char *name; /* layer display-name this icon belongs to */
    uint32_t rows[NVL_ICON_SIZE]; /* bit 23 = left column */
};

/* Two-layer sprite: `line` pixels in the foreground colour, `hole` pixels in the background. */
struct nvl_sprite {
    uint8_t w, h;
    uint16_t line[20]; /* bit 15 = left column */
    uint16_t hole[20];
};

extern const uint8_t nvl_font[64][7];
extern const struct nvl_icon nvl_icons[];
extern const int nvl_icon_count;
extern const struct nvl_sprite nvl_bolt;
