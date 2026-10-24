# Bluetooth RC Car (ESP32 + Adafruit Bluefruit Control Pad)

A two-motor car driven over Bluetooth Low Energy from the Adafruit Bluefruit Connect app. Originally built for TAC 348 (Assignment 9) on a Particle Photon 2; this version is ported to an ESP32 using NimBLE-Arduino.

## How it works

The ESP32 advertises a Nordic UART Service (NUS), which is what the Bluefruit Connect app looks for. The Control Pad sends 5-byte packets:

```
'!'  'B'  <button>  <state>  <CRC>
```

| Button | Action |
|---|---|
| `5` (Up) | Drive forward |
| `6` (Down) | Drive backward |
| `7` (Left) | Pivot left |
| `8` (Right) | Pivot right |
| `1` | Toggle onboard LED (decorative element) |
| `2`, `3`, `4` | Unassigned |

`<state>` is `1` when pressed and `0` when released. The motors stop on release.

## Hardware

- ESP32 dev board
- TB6612FNG motor driver breakout
- 2x DC hobby motors and a car chassis
- Breadboard, jumper wires, and a 3-pin SPDT toggle switch
- Power: USB power bank into the ESP32; the driver's VM is fed from the ESP32 VIN/5V pin (a LiPo battery on VM is the more robust option)

## Wiring

| ESP32 | Motor driver |
|---|---|
| GPIO 25 | PWMA |
| GPIO 26 | AIN2 |
| GPIO 27 | AIN1 |
| GPIO 14 | BIN1 |
| GPIO 12 | BIN2 |
| GPIO 13 | PWMB |
| 3V3 | VCC |
| GND | GND |
| VIN / 5V | VM |

**Switch:** middle pin to STBY, one outer pin to 3V3, other outer pin unconnected. The switch is the master on/off for the driver.

Motors go to AO1/AO2 and BO1/BO2. All grounds (ESP32, driver, battery if used) must be common.

## Setup

1. Install the Arduino IDE.
2. Add the ESP32 board package (Boards Manager, search `esp32`, Espressif Systems).
3. Install the **NimBLE-Arduino** library (h2zero) from the Library Manager. This code targets version 2.x.
4. Open `ble_car_esp32/ble_car_esp32.ino`, select your board (use plain **ESP32 Dev Module** unless yours is a different variant) and port, then upload.
5. Open the Serial Monitor at 115200 baud and confirm it prints `Waiting for a BLE client connection...`.
6. In the Bluefruit Connect app, connect to `ESP32 BLE Car`, then open **Controller > Control Pad**.

## Troubleshooting notes

- **`marked 'override', but does not override`**: you have NimBLE-Arduino 2.x, which changed the callback signatures. The code in this repo already uses the 2.x signatures.
- **`This chip is ESP32, not ESP32-S3`**: the selected board does not match the chip. Pick the board that matches the chip.
- **Device shows as `<Unknown> Uart capable` in the scan list**: that is the ESP32; the name can be missing from the advertisement until you connect.
- **Hold the arrow buttons**: the car only moves while a button is held.

## Status

BLE connection and button parsing work (Button 1 toggles the LED). Motor drive on the arrow buttons is still being debugged.

## Credits

Starting code and assignment from TAC 348 (USC). Controller protocol from the Adafruit Bluefruit LE Connect documentation.
