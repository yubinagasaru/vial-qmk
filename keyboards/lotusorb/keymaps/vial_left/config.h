#pragma once

#define MASTER_LEFT

// --- 右手ファーム用設定 ------------------------------------
// RP2040 + SERIAL_DRIVER = vendor 前提（rules.mk 側で設定）
// 右手側のTRRS配線: TX=GP6, RX=GP7

#define SERIAL_USART_FULL_DUPLEX
#define SERIAL_USART_TX_PIN GP7
#define SERIAL_USART_RX_PIN GP6

// --- 右手 Matrix ピン ---------------------------------
#undef MATRIX_ROW_PINS
#undef MATRIX_COL_PINS
#define MATRIX_ROW_PINS { GP12, GP13, GP14, GP8 }
#define MATRIX_COL_PINS { GP10, GP11, GP26, GP29, GP28, GP27 }

// SPI (PMW3360) ピン：右手側
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN  GP2
#define SPI_MISO_PIN GP4
#define SPI_MOSI_PIN GP3
#define PMW33XX_CS_PIN GP5

// --- Vial 関連 ----------------------------------------------
#define VIAL_KEYBOARD_UID {0x11, 0xAB, 0x70, 0x76, 0x0B, 0xAE, 0xCC, 0x1}
#define VIAL_TAP_DANCE_ENTRIES 8
#define VIAL_COMBO_ENTRIES 8

// --- RP2040 Flash サイズ ------------------------------------
#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
