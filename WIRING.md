# Breadboard wiring

Three Adafruit-style HT16K33 LED backpacks share one I2C bus and act as a single 24 x 8 display.

GPIO7 and GPIO8 on the ESP32-S3 Super Mini are not used.

## Address jumpers on the backpacks

On the back of each backpack, solder the A0 / A1 / A2 pads as follows so every board has a unique address.

| Position on the desk | Role in the 24-wide display | Solder | I2C address |
| --- | --- | --- | --- |
| Left | columns 0 to 7 | none | 0x70 |
| Middle | columns 8 to 15 | A0 only | 0x71 |
| Right | columns 16 to 23 | A1 only | 0x72 |

If you are using mini 0.8 inch 8x8 backpacks they only have A0 and A1, which is enough.

## Power warning

The ESP32-S3 is not 5 V tolerant on its GPIO pins. The backpacks often have onboard I2C pull-ups to VIN. If you feed the backpacks 5 V, those pull-ups can put 5 V onto SDA and SCL and damage the ESP32.

Power the backpacks from the Super Mini 3V3 pin for the breadboard build. Keep brightness moderate. If you later want 5 V LED power for extra brightness, add a bidirectional I2C level shifter and pull SDA/SCL up to 3.3 V only.

## Parts

- ESP32-S3 Super Mini
- 3 x HT16K33 8x8 LED matrix backpacks
- Full-size or half-size breadboard (full-size is easier)
- Dupont jumper wires
- USB-C cable for power and programming
- Optional: extra 3.3 V regulator if the matrices are very bright and the onboard regulator gets warm

## Pin map

| ESP32-S3 Super Mini | Signal | All three backpacks |
| --- | --- | --- |
| 3V3 | 3.3 V | VIN (and VCC if a separate pad exists) |
| GND | Ground | GND |
| GPIO5 | I2C SDA | SDA |
| GPIO6 | I2C SCL | SCL |

Do not use GPIO7 or GPIO8.

USB-C on the Super Mini supplies the board. Do not also feed 5 V into the 5V pin from a second supply unless the grounds are common and you know the board can take it.

## Breadboard layout

Place the Super Mini on the left of the board. Place the three backpacks in a row to its right, left to right: 0x70, 0x71, 0x72. Face the matrices the same way up so the text reads in one line.

```
  3V3 rail  ===============================================
               |          |          |          |
             ESP32      BP 0x70    BP 0x71    BP 0x72
             3V3--VIN    VIN        VIN        VIN
             GND--GND    GND        GND        GND
             IO5--SDA    SDA        SDA        SDA
             IO6--SCL    SCL        SCL        SCL
               |          |          |          |
  GND rail  ===============================================
```

Use the breadboard side rails for 3V3 and GND. Run one SDA wire and one SCL wire along the board and drop short jumpers down to each backpack.

## First power-up checks

1. No solder bridges on the address pads except the ones listed above.
2. VIN is 3.3 V, not 5 V.
3. USB serial at 115200 baud. You should see `ESP32-S3 XRPL LED Ticker`.
4. If a backpack is missing you will see `HT16K33 0x7x not found`.
5. On success the three matrices show a centred `XRPL` welcome screen, then `SETUP BT` until you finish Bluetooth setup.

## Current draw

A fully lit 8x8 at high brightness can draw tens of milliamps. Three of them plus Wi-Fi is usually fine from USB-C. If the board browns out, lower `DISPLAY_BRIGHTNESS` in `src/config.h` (0 to 15, default 8).
