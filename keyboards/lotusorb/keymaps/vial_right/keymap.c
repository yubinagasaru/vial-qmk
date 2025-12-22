#include QMK_KEYBOARD_H
#include "keymap_japanese.h"

#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#endif

// ----------------------
// デバッグ用（必要なら）
// ----------------------
void matrix_scan_user(void) {
    if (!is_keyboard_master()) {
        return;
    }
    static bool last[MATRIX_ROWS][MATRIX_COLS] = {0};

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            bool now = matrix_is_on(r, c);
            if (now != last[r][c]) {
                uprintf("r%d c%d: %s\n", r, c, now ? "DOWN" : "UP");
                last[r][c] = now;
            }
        }
    }
}

/* ─────────────────────────────
 * レイヤー定義
 * ──────────────────────────── */
enum layer_names {
    _BASE,
    _L1,
    _L2,
    _L3,
    _L4,
    _L5,
};

/* ─────────────────────────────
 * Vial 用 custom keycode 定義
 * ──────────────────────────── */
enum custom_keycodes {
    M_L = QK_KB_0,   // Vial: "M-L"       - Mouse Left Click
    M_R,             // Vial: "M-R"       - Mouse Right Click
    M_M,             // Vial: "M-M"       - Mouse Middle Click

    SCRL,            // Vial: "Scroll"    - Scroll mode
    VREV,            // Vial: "V-Rev"     - Reverse vertical scroll direction
    HREV,            // Vial: "H-Rev"     - Reverse horizontal scroll direction

    GATE_UP,         // Vial: "Gate-Up"   - Raise threshold
    GATE_DN,         // Vial: "Gate-Down" - Lower threshold

    CPI_TG,          // Vial: "CPI-TG"    - Toggle CPI value (RIGHT only)

    EE_RST           // Vial: "EE-RST"    - Reset EEPROM + reboot
};

/////////////////////////////
/// CPI 設定
/////////////////////////////
#ifndef COCOT_CPI_OPTIONS
#    define COCOT_CPI_OPTIONS { 1000, 1600, 2200, 2800, 3400 }
#endif
#ifndef COCOT_CPI_DEFAULT
#    define COCOT_CPI_DEFAULT 3
#endif

static uint16_t cpi_array[] = COCOT_CPI_OPTIONS;
#define CPI_OPTION_SIZE (sizeof(cpi_array) / sizeof(uint16_t))

#define LEFT_CPI_FIXED 1600

enum click_state {
    NONE = 0,
    WAITING,
    CLICKABLE,
    CLICKING,
    SCROLLING
};

typedef union {
    uint32_t raw;
    struct {
        int16_t to_clickable_movement;
        bool    mouse_scroll_v_reverse;
        bool    mouse_scroll_h_reverse;
        uint8_t cpi_idx; // 右CPIのインデックス（保存用）
    };
} user_config_t;

static user_config_t user_config;

static enum click_state state;
static uint16_t click_timer;

static uint16_t to_reset_time = 1000;
static const uint16_t click_layer = _L5;

static int16_t scroll_v_mouse_interval_counter;
static int16_t scroll_h_mouse_interval_counter;

static int16_t scroll_v_threshold = 50;
static int16_t scroll_h_threshold = 50;

#define LEFT_SCROLL_V_MULT 3
#define LEFT_SCROLL_H_MULT 2

static int16_t after_click_lock_movement = 0;

static const uint16_t ignore_disable_mouse_layer_keys[] = { KC_LGUI, KC_LCTL };
static int16_t mouse_movement;

static uint8_t mouse_buttons = 0;

static int16_t left_scroll_v_accum = 0;
static int16_t left_scroll_h_accum = 0;
static int16_t left_arrow_x_accum  = 0;
static int16_t left_arrow_y_accum  = 0;

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

static void enable_click_layer(void) {
    layer_on(click_layer);
    click_timer = timer_read();
    state       = CLICKABLE;
}

static void disable_click_layer(void) {
    state = NONE;
    layer_off(click_layer);
    scroll_v_mouse_interval_counter = 0;
    scroll_h_mouse_interval_counter = 0;
}

#ifdef SPLIT_KEYBOARD
typedef struct {
    uint8_t idx;
} right_cpi_msg_t;

// 「右half」で実行される：右センサーのCPIだけ変更
static void rpc_set_right_cpi_handler(uint8_t in_len, const void *in_data,
                                      uint8_t out_len, void *out_data) {
    (void)out_len;
    (void)out_data;

    if (in_len < sizeof(right_cpi_msg_t)) return;

    const right_cpi_msg_t *msg = (const right_cpi_msg_t *)in_data;
    const uint8_t idx = msg->idx;
    if (idx >= CPI_OPTION_SIZE) return;

    // 右halfだけが適用する（左は固定CPIなので触らない）
    if (!is_keyboard_left()) {
        user_config.cpi_idx = idx;
        eeconfig_update_user(user_config.raw);
        pointing_device_set_cpi(cpi_array[idx]);
        uprintf("RPC: RIGHT set idx=%u cpi=%u\n", idx, cpi_array[idx]);
    }
}

static void set_right_cpi_remote(uint8_t idx) {
    right_cpi_msg_t msg = { .idx = idx };
    transaction_rpc_send(RPC_SET_RIGHT_CPI, sizeof(msg), &msg);
}
#endif

// ----------------------
// 初期化
// ----------------------
void eeconfig_init_user(void) {
    user_config.raw                    = 0;
    user_config.to_clickable_movement  = 50;

    // ★ 初期値を「逆」にしたい、とのことだったのでここで設定
    user_config.mouse_scroll_v_reverse = true;
    user_config.mouse_scroll_h_reverse = false;

    user_config.cpi_idx                = COCOT_CPI_DEFAULT;
    eeconfig_update_user(user_config.raw);
}

void keyboard_post_init_user(void) {
    user_config.raw = eeconfig_read_user();

    state = NONE;
    scroll_v_mouse_interval_counter = 0;
    scroll_h_mouse_interval_counter = 0;
    mouse_movement = 0;

    if (user_config.cpi_idx >= CPI_OPTION_SIZE) {
        user_config.cpi_idx = COCOT_CPI_DEFAULT;
    }

#ifdef SPLIT_KEYBOARD
    // ★ RPC handler 登録（これが無いと slave 側で受けられない）
    transaction_register_rpc(RPC_SET_RIGHT_CPI, rpc_set_right_cpi_handler);
#endif

    // ★ 起動時：左は固定、右は保存値
    if (is_keyboard_left()) {
        pointing_device_set_cpi(LEFT_CPI_FIXED);
        uprintf("BOOT: LEFT fixed CPI=%u\n", (unsigned)LEFT_CPI_FIXED);
    } else {
        pointing_device_set_cpi(cpi_array[user_config.cpi_idx]);
        uprintf("BOOT: RIGHT idx=%u cpi=%u\n", user_config.cpi_idx, cpi_array[user_config.cpi_idx]);
    }

    uprintf("PMW init called\n");
}

// ----------------------
// 回転補正（そのまま）
// ----------------------
static void rotate_right(report_mouse_t *rep) {
    int16_t x0 = rep->x;
    int16_t y0 = rep->y;

    int16_t x1 = x0;
    int16_t y1 = -y0;

    const float cos30 = 0.866f;
    const float sin30 = 0.5f;

    float rx = (float)x1 * cos30 - (float)y1 * sin30;
    float ry = (float)x1 * sin30 + (float)y1 * cos30;

    rep->x = (int16_t)rx;
    rep->y = (int16_t)ry;
}

static void rotate_left(report_mouse_t *rep) {
    int16_t x0 = rep->x;
    int16_t y0 = rep->y;

    int16_t x1 = -x0;
    int16_t y1 = -y0;

    int16_t x2 = x1;
    int16_t y2 = -y1;

    const float cos30 = 0.866f;
    const float sin30 = 0.5f;

    float rx = (float)x2 * cos30 + (float)y2 * sin30;
    float ry = (float)y2 * cos30 - (float)x2 * sin30;

    rep->x = (int16_t)rx;
    rep->y = (int16_t)ry;
}

// ----------------------
// キー入力処理
// ----------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case M_L:
        case M_R:
        case M_M: {
            uint8_t btn = 1u << (keycode - M_L);
            if (record->event.pressed) {
                mouse_buttons |= btn;
                state = CLICKING;
                after_click_lock_movement = 30;
                enable_click_layer();
            } else {
                mouse_buttons &= ~btn;
            }
            return false;
        }

        case SCRL:
            if (record->event.pressed) {
                state = SCROLLING;
            } else {
                enable_click_layer();
            }
            return false;

        case GATE_UP:
            if (record->event.pressed) {
                user_config.to_clickable_movement += 5;
                eeconfig_update_user(user_config.raw);
            }
            return false;

        case GATE_DN:
            if (record->event.pressed) {
                user_config.to_clickable_movement -= 5;
                if (user_config.to_clickable_movement < 5) user_config.to_clickable_movement = 5;
                eeconfig_update_user(user_config.raw);
            }
            return false;

        case VREV:
            if (record->event.pressed) {
                user_config.mouse_scroll_v_reverse = !user_config.mouse_scroll_v_reverse;
                eeconfig_update_user(user_config.raw);
            }
            return false;

        case HREV:
            if (record->event.pressed) {
                user_config.mouse_scroll_h_reverse = !user_config.mouse_scroll_h_reverse;
                eeconfig_update_user(user_config.raw);
            }
            return false;

        case CPI_TG:
            if (record->event.pressed) {
                // ★ 右だけ変更
                user_config.cpi_idx = (user_config.cpi_idx + 1) % CPI_OPTION_SIZE;
                eeconfig_update_user(user_config.raw);

                const uint8_t  idx = user_config.cpi_idx;
                const uint16_t cpi = cpi_array[idx];

                if (!is_keyboard_left()) {
                    // masterが右（=右halfで実行中）ならローカル適用
                    pointing_device_set_cpi(cpi);
                    uprintf("CPI_TG: RIGHT(local) idx=%u cpi=%u\n", idx, cpi);
                } else {
#ifdef SPLIT_KEYBOARD
                    // masterが左なら、右halfへRPCで適用させる
                    set_right_cpi_remote(idx);
                    uprintf("CPI_TG: LEFT->RIGHT rpc idx=%u cpi=%u\n", idx, cpi);
#else
                    uprintf("CPI_TG: split not enabled\n");
#endif
                }
            }
            return false;

        case EE_RST:
            if (record->event.pressed) {
                eeconfig_init();
                reset_keyboard();
            }
            return false;

        default:
            if (record->event.pressed) {
                if (state == CLICKING || state == SCROLLING) {
                    enable_click_layer();
                    return false;
                }

                for (int i = 0; i < (int)(sizeof(ignore_disable_mouse_layer_keys) / sizeof(ignore_disable_mouse_layer_keys[0])); i++) {
                    if (keycode == ignore_disable_mouse_layer_keys[i]) {
                        enable_click_layer();
                        return true;
                    }
                }
                disable_click_layer();
            }
            break;
    }
    return true;
}

// ----------------------
// miniZone 本体（オートマウスレイヤー制御）
// ----------------------
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    int16_t current_h = mouse_report.h;
    int16_t current_v = mouse_report.v;

    if (current_x != 0 || current_y != 0) {
        switch (state) {
            case CLICKABLE:
                click_timer = timer_read();
                break;

            case CLICKING:
                after_click_lock_movement -= my_abs(current_x) + my_abs(current_y);

                if (after_click_lock_movement > 0) {
                    current_x = 0;
                    current_y = 0;
                }
                break;

            case SCROLLING: {
                int8_t rep_v = 0;
                int8_t rep_h = 0;

                // 縦・横のどっちを優先するか判定
                if (my_abs(current_y) * 2 > my_abs(current_x)) {
                    scroll_v_mouse_interval_counter += current_y;
                    while (my_abs(scroll_v_mouse_interval_counter) > scroll_v_threshold) {
                        if (scroll_v_mouse_interval_counter < 0) {
                            scroll_v_mouse_interval_counter += scroll_v_threshold;
                            rep_v += scroll_v_threshold;
                        } else {
                            scroll_v_mouse_interval_counter -= scroll_v_threshold;
                            rep_v -= scroll_v_threshold;
                        }
                    }
                } else {
                    scroll_h_mouse_interval_counter += current_x;
                    while (my_abs(scroll_h_mouse_interval_counter) > scroll_h_threshold) {
                        if (scroll_h_mouse_interval_counter < 0) {
                            scroll_h_mouse_interval_counter += scroll_h_threshold;
                            rep_h += scroll_h_threshold;
                        } else {
                            scroll_h_mouse_interval_counter -= scroll_h_threshold;
                            rep_h -= scroll_h_threshold;
                        }
                    }
                }

                int8_t steps_h = rep_h / scroll_h_threshold;
                int8_t steps_v = rep_v / scroll_v_threshold;

                current_h = steps_h * (user_config.mouse_scroll_h_reverse ? -1 : 1);
                current_v = -steps_v * (user_config.mouse_scroll_v_reverse ? -1 : 1);

                current_x = 0;
                current_y = 0;
            } break;

            case WAITING:
                mouse_movement += my_abs(current_x) + my_abs(current_y);

                if (mouse_movement >= user_config.to_clickable_movement) {
                    mouse_movement = 0;
                    enable_click_layer();
                }
                break;

            default:
                click_timer    = timer_read();
                state          = WAITING;
                mouse_movement = 0;
        }
    } else {
        switch (state) {
            case CLICKING:
            case SCROLLING:
                break;

            case CLICKABLE:
                if (timer_elapsed(click_timer) > to_reset_time) {
                    disable_click_layer();
                }
                break;

            case WAITING:
                if (timer_elapsed(click_timer) > 50) {
                    mouse_movement = 0;
                    state          = NONE;
                }
                break;

            default:
                mouse_movement = 0;
                state          = NONE;
        }
    }

    mouse_report.x = current_x;
    mouse_report.y = current_y;
    mouse_report.h = current_h;
    mouse_report.v = current_v;

    return mouse_report;
}

// ----------------------
// 左トラボ → スクロール変換
// ----------------------
static void apply_scroll_from_left(report_mouse_t *out, report_mouse_t left_report) {
    int16_t lx = left_report.x;
    int16_t ly = left_report.y;

    if (!lx && !ly) {
        return;
    }

    int8_t steps_v = 0;
    int8_t steps_h = 0;

    // 縦方向
    left_scroll_v_accum += ly;
    while (my_abs(left_scroll_v_accum) > scroll_v_threshold) {
        if (left_scroll_v_accum < 0) {
            left_scroll_v_accum += scroll_v_threshold;
            steps_v++;
        } else {
            left_scroll_v_accum -= scroll_v_threshold;
            steps_v--;
        }
    }

    // 横方向
    left_scroll_h_accum += lx;
    while (my_abs(left_scroll_h_accum) > scroll_h_threshold) {
        if (left_scroll_h_accum < 0) {
            left_scroll_h_accum += scroll_h_threshold;
            steps_h++;
        } else {
            left_scroll_h_accum -= scroll_h_threshold;
            steps_h--;
        }
    }

    // 左トラボだけ倍率
    int16_t delta_h = steps_h * LEFT_SCROLL_H_MULT *
                      (user_config.mouse_scroll_h_reverse ? -1 : 1);
    int16_t delta_v = -steps_v * LEFT_SCROLL_V_MULT *
                      (user_config.mouse_scroll_v_reverse ? -1 : 1);

    out->h += delta_h;
    out->v += delta_v;
}

// ----------------------
// 左トラボ → 矢印キー変換（_L1 用）
// ----------------------
static void apply_arrows_from_left(report_mouse_t left_report) {
    int16_t lx = left_report.x;
    int16_t ly = left_report.y;

    if (!lx && !ly) {
        return;
    }

    const int16_t arrow_threshold = 40;

    if (my_abs(lx) >= my_abs(ly)) {
        left_arrow_y_accum = 0;
        left_arrow_x_accum += lx;

        while (left_arrow_x_accum >= arrow_threshold) {
            tap_code(KC_RIGHT);
            left_arrow_x_accum -= arrow_threshold;
        }
        while (left_arrow_x_accum <= -arrow_threshold) {
            tap_code(KC_LEFT);
            left_arrow_x_accum += arrow_threshold;
        }
    } else {
        left_arrow_x_accum = 0;
        left_arrow_y_accum += ly;

        while (left_arrow_y_accum >= arrow_threshold) {
            tap_code(KC_DOWN);
            left_arrow_y_accum -= arrow_threshold;
        }
        while (left_arrow_y_accum <= -arrow_threshold) {
            tap_code(KC_UP);
            left_arrow_y_accum += arrow_threshold;
        }
    }
}

// ----------------------
// COMBINED 用：左右トラボ統合
// ----------------------
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report,
                                                  report_mouse_t right_report) {
    // 左右それぞれ回転補正
    rotate_left(&left_report);
    rotate_right(&right_report);

    bool left_active  = left_report.x || left_report.y || left_report.h || left_report.v;
    bool right_active = right_report.x || right_report.y || right_report.h || right_report.v;

    uint8_t top_layer = get_highest_layer(layer_state);

    // センサー由来レポートは x/y/h/v だけ使い、buttons は mouse_buttons 統一管理
    report_mouse_t merged = (report_mouse_t){0};
    merged.buttons = mouse_buttons;

    // L1 のとき：左=矢印、右=カーソル
    if (top_layer == _L1) {
        if (left_active) {
            apply_arrows_from_left(left_report);
        }
        if (right_active) {
            merged.x = right_report.x;
            merged.y = right_report.y;
            merged.h = right_report.h;
            merged.v = right_report.v;
        }
        return merged;
    }

    // 右トラボ = カーソル
    if (right_active) {
        merged.x = right_report.x;
        merged.y = right_report.y;
        merged.h = right_report.h;
        merged.v = right_report.v;
    }

    // 左トラボ = スクロール
    if (left_active) {
        apply_scroll_from_left(&merged, left_report);
    }

    // BASE のときだけ miniZone のオートレイヤー制御
    if (top_layer == _BASE) {
        merged = pointing_device_task_user(merged);
    }

    return merged;
}

// =========================
// キーマップ
// =========================
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [_BASE] = LAYOUT(
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,     KC_T,
        KC_LGUI, KC_A,    KC_S,    KC_D,    KC_F,     KC_G,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,     KC_B,
        KC_LCTL, KC_LALT, LT(_L3, KC_GRV),  LT(_L2, KC_SPC),  MO(_L1),  KC_DEL,

        KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,     KC_MINS,
        KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,  KC_QUOTE,
        KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_INT1,
        KC_BSPC, M_L,     KC_ENT,  KC_LBRC, KC_LEFT,  LT(_L4, KC_RGHT)
    ),

    [_L1] = LAYOUT(
        KC_TAB,  KC_1,    KC_2,    KC_3,    KC_4,     KC_5,
        KC_ENT,  KC_6,    KC_7,    KC_8,    KC_9,     KC_0,
        KC_LSFT, KC_SLSH, KC_ASTR, KC_MINS, KC_PLUS,  KC_DOT,
        KC_LCTL, KC_LALT, _______, _______, _______,  _______,

        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,     KC_MINS,
        _______, _______, KC_RBRC, KC_NONUS_HASH, KC_SCLN,  KC_QUOTE,
        _______, _______, KC_COMM, KC_DOT,  KC_SLSH,  KC_INT1,
        KC_BSPC, _______, KC_ENT,  _______, KC_LEFT,  KC_RGHT
    ),

    [_L2] = LAYOUT(
        KC_TAB,  _______, LCTL(KC_W), _______, _______, _______,
        KC_LGUI, LCTL(KC_A), _______, M_M,    _______, _______,
        KC_LSFT, LCTL(KC_Z), LCTL(KC_X), LCTL(KC_C), LCTL(KC_V), _______,
        KC_LCTL, KC_LALT, _______, _______, _______, _______,

        SCRL,    M_R,     KC_UP,   _______, _______, _______,
        _______, KC_LEFT, KC_DOWN, KC_RGHT, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, M_L,     KC_ENT, _______, _______, _______
    ),

    [_L3] = LAYOUT(
        KC_TAB,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,
        KC_LGUI, KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,
        KC_LSFT, KC_F11,  KC_F12, _______, _______, _______,
        KC_LCTL, KC_LALT,_______, KC_SPC,  _______, KC_DEL,

        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        KC_BSPC, M_L,     KC_ENT, _______, _______, _______
    ),

    [_L4] = LAYOUT(
        _______, _______, _______,  GATE_UP,   _______,   _______,
        _______, _______, VREV,     CPI_TG,    HREV,      _______,
        _______, _______, _______,  GATE_DN,   _______,   _______,
        _______, _______, _______,  _______,   _______,   _______,

        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______
    ),

    [_L5] = LAYOUT(
        KC_TAB,  _______, _______, _______, _______, _______,
        KC_LGUI, _______, _______, M_M,    _______, _______,
        KC_LSFT, _______, _______, _______, _______, _______,
        KC_LCTL, KC_LALT, _______, _______, _______, _______,

        SCRL,    M_R,     _______, _______, _______, _______,
        M_M,     _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,
        _______, M_L,     KC_ENT, _______, _______, _______
    ),
};
