#pragma once

#define EE_HANDS

// --- 右手ファーム用設定 ------------------------------------
// RP2040 + SERIAL_DRIVER = vendor 前提（rules.mk 側で設定）
// 右手側のTRRS配線: TX=GP6, RX=GP7

#define SERIAL_USART_FULL_DUPLEX
#define SERIAL_USART_TX_PIN GP11
#define SERIAL_USART_RX_PIN GP12

// --- 右手 Matrix ピン ---------------------------------
#undef MATRIX_ROW_PINS
#undef MATRIX_COL_PINS
#define MATRIX_ROW_PINS { GP6, GP5, GP4, GP10 }
#define MATRIX_COL_PINS { GP2, GP1, GP0, GP3, GP7, GP8 }

// SPI (PMW3360) ピン：右手側
#define SPI_DRIVER SPID1
#define SPI_SCK_PIN GP26
#define SPI_MISO_PIN GP28
#define SPI_MOSI_PIN GP15
#define PMW33XX_CS_PIN GP13

// --- Vial 関連 ----------------------------------------------
#define VIAL_KEYBOARD_UID {0x11, 0xAB, 0x70, 0x76, 0x0B, 0xAE, 0xCC, 0x1}
#define VIAL_TAP_DANCE_ENTRIES 8
#define VIAL_COMBO_ENTRIES 8

// --- RP2040 Flash サイズ ------------------------------------
#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
