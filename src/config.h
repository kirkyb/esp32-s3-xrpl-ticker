#pragma once

// I2C on GPIO5 / GPIO6. GPIO7 and GPIO8 are left unused as requested.
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6
#define I2C_FREQ_HZ 100000

// Three HT16K33 backpacks. Solder address jumpers:
//   Left   (x = 0..7)   default 0x70  (no jumpers)
//   Middle (x = 8..15)  0x71          (A0 bridged)
//   Right  (x = 16..23) 0x72          (A1 bridged)
#define HT16K33_ADDR_LEFT   0x70
#define HT16K33_ADDR_MID    0x71
#define HT16K33_ADDR_RIGHT  0x72

#define DISPLAY_WIDTH  24
#define DISPLAY_HEIGHT 8
#define DISPLAY_BRIGHTNESS 8   // 0..15

// Timing
#define WELCOME_MS          3500
#define PAGE_MS             5000
#define SCROLL_MS           90
#define PRICE_REFRESH_MS    60000
#define WIFI_RETRY_MS       20000

#define BLE_NAME "XRPL-Ticker"

// Preferences namespace
#define NVS_NAMESPACE "xrpltick"
