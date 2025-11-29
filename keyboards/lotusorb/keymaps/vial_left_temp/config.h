#pragma once


#define MASTER_LEFT

// --- 左手ファーム用設定 ------------------------------------
// RP2040 + SERIAL_DRIVER = vendor 前提（rules.mk 側で設定）
// 左手側のTRRS配線: TX=GP11, RX=GP12

#define SERIAL_USART_FULL_DUPLEX
#define SERIAL_USART_TX_PIN GP11
#define SERIAL_USART_RX_PIN GP12

// --- 左手 Matrix ピン上書き ---------------------------------

#undef MATRIX_ROW_PINS
#undef MATRIX_COL_PINS
#define MATRIX_ROW_PINS { GP6, GP5, GP4, GP10 }
#define MATRIX_COL_PINS { GP27, GP28, GP29, GP26, GP11, GP12 }

// --- 左手 PMW3360 センサー設定 ----------------------------- 

#define PMW33XX_CS_PIN    GP13
#define PMW33XX_MOSI_PIN  GP15
#define PMW33XX_MISO_PIN  GP4
#define PMW33XX_SCLK_PIN  GP2


// --- Vial 関連 ----------------------------------------------
#define VIAL_KEYBOARD_UID {0x27, 0xdb, 0x27, 0x97, 0xcf, 0x02, 0x66, 0x6c}
#define VIAL_TAP_DANCE_ENTRIES 8
#define VIAL_COMBO_ENTRIES 8

// --- RP2040 Flash サイズ ------------------------------------
#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
