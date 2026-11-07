#pragma once

/*
 * Draws the whole status screen into a 1-bit portrait framebuffer (68 wide, 160 tall).
 * Plain C with no ZMK or LVGL dependencies, so it can also be compiled on a PC for previews.
 */

#include <stdbool.h>
#include <stdint.h>

#define NVL_W 68
#define NVL_H 160

enum nvl_output {
    NVL_OUT_USB,
    NVL_OUT_BLE,
};

struct nvl_state {
    uint8_t bat_left;      /* 0-100, this (main) half */
    uint8_t bat_right;     /* 0-100, other half */
    bool right_connected;  /* other half linked to this one */
    bool charging;         /* this half is on USB power */
    enum nvl_output output;
    bool ble_connected;    /* active profile has a live connection */
    bool ble_bonded;       /* active profile is paired */
    uint8_t profile;       /* active profile, 0-based */
    uint8_t profile_bonded; /* bit n set = profile n is paired */
    uint8_t layer_index;
    const char *layer_name; /* may be NULL */
};

/* fb[y][x] is set to 1 for lit pixels, 0 for background. */
void nvl_render(const struct nvl_state *s, uint8_t fb[NVL_H][NVL_W]);
