#include QMK_KEYBOARD_H

#ifdef POINTING_DEVICE_ENABLE
#    include "print.h"

void pointing_device_init_user(void) {
    uprintf("PMW init called\n");
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    uprintf("motion x=%d y=%d\n", mouse_report.x, mouse_report.y);
    return mouse_report;
}
#endif

enum layer_names {
    _BASE,
    _L1,
    _L2,
    _L3,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [_BASE] = LAYOUT(
        KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,     KC_T,
        KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,     KC_G,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,     KC_B,
        KC_LCTL, KC_LALT, KC_GRV,  KC_SPC,  MO(_L1),  KC_DEL,

        KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,     KC_MINS,
        KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,  KC_ENT,
        KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_BSLS,
        KC_BSPC, KC_ENT,  KC_ENT,  KC_ENT,  KC_LEFT,  KC_RGHT
    ),

    [_L1] = LAYOUT(
        KC_TAB,  KC_1,    KC_2,    KC_3,    KC_4,     KC_5,
        KC_ENT,  KC_6,    KC_7,    KC_8,    KC_9,     KC_0,
        KC_LSFT, KC_SLSH, KC_ASTR, KC_MINS, KC_PLUS,  KC_DOT,
        KC_LCTL, _______, _______, _______, _______,  _______,

        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,     KC_MINS,
        _______, KC_RBRC, KC_TILD, KC_HASH, KC_PLUS,  KC_ASTR,
        _______, _______, KC_COMM, KC_DOT,  KC_SLSH,  KC_BSLS,
        KC_BSPC, KC_ENT,  KC_ENT,  KC_ENT,  KC_LEFT,  KC_RGHT
    ),

    [_L2] = LAYOUT(
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,

        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______
    ),

    [_L3] = LAYOUT(
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,

        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______
    ),
};
