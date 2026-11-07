/*
 * ZMK glue for the nice_view_lily status screen: collects state from ZMK events and
 * copies the portrait framebuffer from nvl_render() onto the landscape nice!view.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/battery.h>

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#define NVL_CENTRAL 1
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
#include <zmk/split/central.h>
#endif

#include "nvl_render.h"

#define COLOR_BG (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white())
#define COLOR_FG (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black())

static struct nvl_state state;
static uint8_t fb[NVL_H][NVL_W];
static lv_color_t cbuf[NVL_H * NVL_W]; /* landscape: NVL_H wide, NVL_W tall */
static lv_obj_t *canvas;

static void redraw(void) {
    if (canvas == NULL) {
        return;
    }
    nvl_render(&state, fb);

    lv_color_t fg = COLOR_FG, bg = COLOR_BG;
    for (int py = 0; py < NVL_H; py++) {
        for (int px = 0; px < NVL_W; px++) {
            /* The screen is mounted rotated: portrait top = landscape right. */
            int x = IS_ENABLED(CONFIG_NICE_VIEW_LILY_FLIP) ? py : NVL_H - 1 - py;
            int y = IS_ENABLED(CONFIG_NICE_VIEW_LILY_FLIP) ? NVL_W - 1 - px : px;
            cbuf[y * NVL_H + x] = fb[py][px] ? fg : bg;
        }
    }
    lv_obj_invalidate(canvas);
}

/* ---- This half's battery ---- */

struct nvl_own_battery_state {
    uint8_t level;
    bool usb;
};

static void own_battery_update_cb(struct nvl_own_battery_state s) {
    state.bat_left = s.level;
    state.charging = s.usb;
    redraw();
}

static struct nvl_own_battery_state own_battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct nvl_own_battery_state){
        .level = ev ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(nvl_own_battery, struct nvl_own_battery_state, own_battery_update_cb,
                            own_battery_get_state)
ZMK_SUBSCRIPTION(nvl_own_battery, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(nvl_own_battery, zmk_usb_conn_state_changed);
#endif

/* ---- Other half's battery ---- */

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

struct nvl_peer_battery_state {
    uint8_t level;
};

static void peer_battery_update_cb(struct nvl_peer_battery_state s) {
    /* ZMK reports 0% when the other half disconnects. */
    state.bat_right = s.level;
    state.right_connected = s.level > 0;
    redraw();
}

static struct nvl_peer_battery_state peer_battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    uint8_t level = 0;
    if (ev) {
        level = ev->state_of_charge;
    } else {
        zmk_split_central_get_peripheral_battery_level(0, &level);
    }
    return (struct nvl_peer_battery_state){.level = level};
}

ZMK_DISPLAY_WIDGET_LISTENER(nvl_peer_battery, struct nvl_peer_battery_state, peer_battery_update_cb,
                            peer_battery_get_state)
ZMK_SUBSCRIPTION(nvl_peer_battery, zmk_peripheral_battery_state_changed);

#endif

#ifdef NVL_CENTRAL

/* ---- Output (USB / Bluetooth profile) ---- */

struct nvl_output_state {
    bool usb;
    bool connected;
    bool bonded;
    uint8_t profile;
    uint8_t bonded_mask;
};

static void output_update_cb(struct nvl_output_state s) {
    state.output = s.usb ? NVL_OUT_USB : NVL_OUT_BLE;
    state.ble_connected = s.connected;
    state.ble_bonded = s.bonded;
    state.profile = s.profile;
    state.profile_bonded = s.bonded_mask;
    redraw();
}

static struct nvl_output_state output_get_state(const zmk_event_t *_eh) {
    struct nvl_output_state s = {
        .usb = zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB,
    };
#if IS_ENABLED(CONFIG_ZMK_BLE)
    s.connected = zmk_ble_active_profile_is_connected();
    s.bonded = !zmk_ble_active_profile_is_open();
    s.profile = zmk_ble_active_profile_index();
    for (int i = 0; i < 3 && i < ZMK_BLE_PROFILE_COUNT; i++) {
        if (!zmk_ble_profile_is_open(i)) {
            s.bonded_mask |= 1 << i;
        }
    }
#endif
    return s;
}

ZMK_DISPLAY_WIDGET_LISTENER(nvl_output, struct nvl_output_state, output_update_cb, output_get_state)
ZMK_SUBSCRIPTION(nvl_output, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(nvl_output, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(nvl_output, zmk_ble_active_profile_changed);
#endif

/* ---- Layer ---- */

struct nvl_layer_state {
    uint8_t index;
    const char *name;
};

static void layer_update_cb(struct nvl_layer_state s) {
    state.layer_index = s.index;
    state.layer_name = s.name;
    redraw();
}

static struct nvl_layer_state layer_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct nvl_layer_state){
        .index = index,
        .name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(nvl_layer, struct nvl_layer_state, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(nvl_layer, zmk_layer_state_changed);

#endif /* NVL_CENTRAL */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    canvas = lv_canvas_create(screen);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, cbuf, NVL_H, NVL_W, LV_IMG_CF_TRUE_COLOR);

    nvl_own_battery_init();
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    nvl_peer_battery_init();
#endif
#ifdef NVL_CENTRAL
    nvl_output_init();
    nvl_layer_init();
#endif
    redraw();

    return screen;
}
