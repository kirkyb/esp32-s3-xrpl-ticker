# ESP32-S3 XRPL LED Ticker

Firmware for an ESP32-S3 Super Mini that drives three HT16K33 LED backpacks as one 24 x 8 display. It shows a live XRPL (or Xahau) token price, the quantity held on your address, and the fiat value of that holding.

First boot presents a welcome screen. Setup is done over Bluetooth. Wi-Fi, XRPL address, token and fiat currency are stored in flash. A `RESET ALL` command wipes them.

GPIO7 and GPIO8 are not used.

## What it shows

The three matrices are treated as a single display. Short strings are centred. Longer strings scroll.

Pages rotate about every 5 seconds:

1. Token price in the chosen fiat, formatted to 3 significant figures (for example `XRP 2.15 GBP`)
2. Quantity of that token on the configured address (`QTY 1.25K XRP`)
3. Current value of those tokens (`VAL 2688 GBP`)

Prices refresh every 60 seconds.

Built-in tokens:

| Token | Network | Notes |
| --- | --- | --- |
| XRP | XRP Ledger | Native balance via `account_info` |
| CSC | XRP Ledger | CasinoCoin IOU, issuer preloaded |
| XAH | Xahau | Native XAH balance on the Xahau network |
| SOLO | XRP Ledger | Issuer preloaded |
| RLUSD | XRP Ledger | Issuer preloaded |

Any other XRPL IOU works if you set `SET TOKEN FOO` and `SET ISSUER r....`. Price then comes from the XRPL DEX book against XRP, converted to fiat.

XRP and XAH spot prices use CoinGecko, with Coinbase as a fallback for XRP.

## Hardware

- ESP32-S3 Super Mini
- 3 x HT16K33 8x8 LED matrix backpacks (Adafruit or compatible)
- Breadboard and jumper wires

Full breadboard instructions are in [WIRING.md](WIRING.md).

I2C pins used:

- SDA = GPIO5
- SCL = GPIO6

Backpack addresses: `0x70` (left), `0x71` (A0, middle), `0x72` (A1, right).

Power the backpacks from 3.3 V. Do not hang 5 V pull-ups on the ESP32 I2C pins.

## Build and flash

### PlatformIO (recommended)

```bash
git clone https://github.com/kirkyb/esp32-s3-xrpl-ticker.git
cd esp32-s3-xrpl-ticker
pio run -t upload
pio device monitor -b 115200
```

### Arduino IDE

1. Install ESP32 board support (Espressif).
2. Board: `ESP32S3 Dev Module`.
3. Settings: USB CDC On Boot = Enabled, Flash 4 MB or match your module, PSRAM disabled unless your Super Mini has it.
4. Libraries: Adafruit LED Backpack, Adafruit GFX, Adafruit BusIO, NimBLE-Arduino, ArduinoJson.
5. Open the `.cpp` / `.h` files under `src/` as a sketch folder, or copy them into a sketch named `esp32-s3-xrpl-ticker`.
6. Upload at 115200 serial.

## Bluetooth setup

On first power the display shows `XRPL`, then `SETUP BT`.

1. On your phone install a BLE UART app such as nRF Connect or Serial Bluetooth Terminal.
2. Connect to the device named `XRPL-Ticker`.
3. Use the Nordic UART service. Write text commands ending with a newline.

Commands:

```
HELP
GET
SET WIFI MySsid|MyPassword
SET ADDR rYourXrplAddressHere
SET TOKEN XRP
SET FIAT GBP
SET ISSUER NONE
SAVE
RESET ALL
REBOOT
```

`SET WIFI` uses a pipe so the password may contain spaces.

Example first-time session:

```
SET WIFI HomeNet|correct horse battery
SET ADDR rPT1Sjq2YGrBMTttX4GZHjKu9dyfzbpAYe
SET TOKEN XRP
SET FIAT GBP
SAVE
REBOOT
```

After a successful save the board joins Wi-Fi, shows `ONLINE`, and starts the price / quantity / value cycle.

`RESET ALL` clears every stored field and reboots into setup.

Bluetooth stays available after setup so you can change token, fiat or wipe the device without reflashing.

## Configuration stored in flash

All of the following live in NVS (non-volatile storage) on the ESP32:

- Wi-Fi SSID and password
- XRPL / Xahau address
- Token ticker
- Fiat code (GBP, USD, EUR, ...)
- Optional IOU issuer

Nothing is hardcoded in the firmware besides the known-token table in `src/tokens.h`.

## Troubleshooting

| Display | Meaning |
| --- | --- |
| XRPL | Welcome screen |
| SETUP BT | Waiting for Bluetooth configuration |
| WIFI | Joining the configured network |
| WIFI ERR | Join failed. Check SSID/password over BLE |
| ONLINE | Wi-Fi up, about to fetch |
| PRICE ERR | Spot price API failed |
| ADDR ERR | Address lookup failed. Price still shown, quantity is 0 |
| RESET | Factory reset in progress |

If a matrix stays blank, check its address jumper and the I2C wiring. Serial will print `HT16K33 0x7x not found`.

If the board resets under load, lower `DISPLAY_BRIGHTNESS` in `src/config.h`.

Public HTTPS endpoints (`s1.ripple.com`, CoinGecko, Coinbase, Xahau) must be reachable from your Wi-Fi. Captive-portal networks will not work.

## Licence

MIT. See [LICENSE](LICENSE).
