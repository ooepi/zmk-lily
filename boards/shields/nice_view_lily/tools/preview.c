/*
 * Renders example screens with the real drawing code into preview.pgm.
 * Build and run on a PC:  gcc -I.. -o preview preview.c ../nvl_render.c ../nvl_assets.c && ./preview
 */
#include <stdio.h>
#include "nvl_render.h"

#define SCALE 3
#define GAP 6

static const struct nvl_state examples[] = {
    {.bat_left = 82, .bat_right = 47, .right_connected = true, .output = NVL_OUT_BLE,
     .ble_connected = true, .ble_bonded = true, .profile = 0, .profile_bonded = 3, .layer_name = "BASE"},
    {.bat_left = 64, .bat_right = 63, .right_connected = true, .charging = true, .output = NVL_OUT_USB,
     .profile = 1, .profile_bonded = 3, .layer_name = "NAV"},
    {.bat_left = 30, .bat_right = 12, .right_connected = true, .output = NVL_OUT_BLE,
     .ble_connected = false, .ble_bonded = true, .profile = 1, .profile_bonded = 3, .layer_name = "PROG"},
    {.bat_left = 100, .bat_right = 0, .right_connected = false, .output = NVL_OUT_BLE,
     .ble_connected = false, .ble_bonded = false, .profile = 2, .profile_bonded = 1, .layer_name = "MOUS"},
    {.bat_left = 5, .bat_right = 91, .right_connected = true, .output = NVL_OUT_BLE,
     .ble_connected = true, .ble_bonded = true, .profile = 0, .profile_bonded = 7, .layer_name = "NUM"},
    {.bat_left = 55, .bat_right = 54, .right_connected = true, .output = NVL_OUT_BLE,
     .ble_connected = true, .ble_bonded = true, .profile = 2, .profile_bonded = 7, .layer_name = "QWER"},
    {.bat_left = 77, .bat_right = 80, .right_connected = true, .output = NVL_OUT_BLE,
     .ble_connected = true, .ble_bonded = true, .profile = 3, .profile_bonded = 7, .layer_name = "Gaming"},
};

#define N (sizeof(examples) / sizeof(examples[0]))

int main(void) {
    static uint8_t fb[N][NVL_H][NVL_W];
    for (unsigned i = 0; i < N; i++) {
        nvl_render(&examples[i], fb[i]);
    }

    int w = N * (NVL_W * SCALE + GAP) + GAP, h = NVL_H * SCALE + 2 * GAP;
    FILE *f = fopen("preview.pgm", "wb");
    fprintf(f, "P5 %d %d 255\n", w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int c = 255;
            int sy = (y - GAP) / SCALE, cell = x / (NVL_W * SCALE + GAP), sx = (x - GAP - cell * (NVL_W * SCALE + GAP)) / SCALE;
            if (y >= GAP && sy < NVL_H && x - cell * (NVL_W * SCALE + GAP) >= GAP && sx < NVL_W && cell < (int)N) {
                c = fb[cell][sy][sx] ? 230 : 24;
            }
            fputc(c, f);
        }
    }
    fclose(f);
    return 0;
}
