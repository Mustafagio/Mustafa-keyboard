# Mustafa Keyboard

64-key wireless Bluetooth mechanical keyboard with a Turkish Q layout.

🇹🇷 [Türkçe](README.md) | 🇬🇧 **English**

This project is a custom mechanical keyboard developed using a hand-wired design without a PCB.

## Features

- 64 mechanical switches
- Turkish Q layout
- ZMK firmware
- nRF52840-based controller
- Bluetooth Low Energy
- 5 Bluetooth profile/bond slots
- Bluetooth profile switching
- Persistent Auto-Off setting
- Soft Off
- Soft Off with FN + ESC
- Wake from Soft Off with ESC
- LED control
- 5 × 14 matrix
- PCB-free hand-wired design
- 3D-printed case

## Hardware

- Robiz nRF52840 Pro Micro V1.840
- 64 × mechanical switches
- Diodes
- 18650 Li-ion battery
- TP4056 charging module
- 3D-printed keyboard case
- Hand wiring

## Firmware

The firmware is based on ZMK Firmware.

ZMK Firmware:

https://zmk.dev/

Used board:

    nice_nano_v2

Used shield:

    mustafa_keyboard

Bluetooth device name:

    Mustafa KB

## Turkish Q Layout

The keyboard is configured according to the Turkish Q layout.

Supported Turkish characters:

- Ğ
- Ü
- Ş
- İ
- Ö
- Ç
- ı

Turkish character definitions on the ZMK side:

    boards/shields/mustafa_keyboard/keys_tr.h

Main keymap:

    boards/shields/mustafa_keyboard/mustafa_keyboard.keymap

## Bluetooth

The keyboard supports 5 different Bluetooth profiles.

Profile slots:

    Profile 0
    Profile 1
    Profile 2
    Profile 3
    Profile 4

Each profile can be paired with a different Bluetooth device.

Profiles can be selected using the Bluetooth profile keys on the keyboard.

This allows the keyboard to be used with multiple computers or devices.

## Auto-Off

The keyboard has an automatic power-off feature.

Supported timeouts:

- 2 minutes
- 5 minutes
- 10 minutes
- 15 minutes
- 20 minutes
- Off

The Auto-Off setting is stored persistently.

When the keyboard is restarted or the Bluetooth connection is re-established, the last used Auto-Off setting is preserved.

Auto-Off is managed on the firmware side by:

    src/auto_off.c

## Soft Off

Soft Off can be activated with:

    FN + ESC

The keyboard can be woken from Soft Off by pressing:

    ESC

## LED Control

Startup LED control and the shutdown warning are implemented in:

    src/startup_led.c

The keyboard control service is implemented in:

    src/control_service.c

## Matrix

The keyboard uses a 5 × 14 matrix.

Diode direction:

    COL2ROW

### Rows

| Row | GPIO |
|---|---|
| R0 | P0.06 |
| R1 | P0.08 |
| R2 | P0.17 |
| R3 | P0.20 |
| R4 | P0.22 |

### Columns

| Column | GPIO |
|---|---|
| C0 | P0.24 |
| C1 | P1.00 |
| C2 | P0.11 |
| C3 | P1.04 |
| C4 | P1.06 |
| C5 | P0.31 |
| C6 | P0.29 |
| C7 | P0.02 |
| C8 | P1.15 |
| C9 | P1.13 |
| C10 | P1.11 |
| C11 | P1.01 |
| C12 | P1.02 |
| C13 | P1.07 |

## Matrix GPIO Configuration

The matrix configuration is located in:

    boards/shields/mustafa_keyboard/mustafa_keyboard.overlay

The keymap is located in:

    boards/shields/mustafa_keyboard/mustafa_keyboard.keymap

## Project Structure

    Mustafa-keyboard/
    │
    ├── .github/
    │   └── workflows/
    │       └── build.yml
    │
    ├── boards/
    │   └── shields/
    │       └── mustafa_keyboard/
    │           ├── Kconfig.defconfig
    │           ├── Kconfig.shield
    │           ├── keys_tr.h
    │           ├── mustafa_keyboard.keymap
    │           └── mustafa_keyboard.overlay
    │
    ├── config/
    │   └── mustafa_keyboard.conf
    │
    ├── src/
    │   ├── auto_off.c
    │   ├── control_service.c
    │   └── startup_led.c
    │
    ├── zephyr/
    │   ├── module.yml
    │   └── build.yaml
    │
    ├── CMakeLists.txt
    └── README.md

## Firmware Build

The firmware can be built automatically using GitHub Actions.

Build target:

    nice_nano_v2

Shield:

    mustafa_keyboard

Build configuration is located in:

    zephyr/build.yaml

The `settings_reset` firmware is also included in the build configuration for resetting ZMK settings.

## GitHub Actions

After changes are made to the project, GitHub Actions automatically builds the firmware.

Workflow files are located in:

    .github/workflows/

When the build succeeds, the generated firmware file can be downloaded through GitHub Actions.

## Files

### mustafa_keyboard.overlay

Contains the keyboard GPIO and matrix configuration.

### mustafa_keyboard.keymap

Contains the ZMK keymap configuration for the keyboard keys.

### keys_tr.h

Contains custom key definitions for the Turkish Q layout.

### auto_off.c

Manages the Auto-Off functions and persistent Auto-Off setting.

### control_service.c

Manages the Bluetooth GATT control services between the keyboard and the control application.

### startup_led.c

Manages the startup LED and shutdown warning functions.

## Warning

This project is a custom-built keyboard.

The firmware configuration should be checked before changing GPIO connections or matrix wiring.

Changing ROW and COLUMN connections may make the hardware incompatible with the current firmware.

## Project Status

Working features:

- Turkish Q layout
- Bluetooth connection
- 5 Bluetooth profile slots
- Bluetooth profile switching
- Auto-Off
- Persistent Auto-Off setting
- Soft Off
- LED control
- 5 × 14 matrix
- PCB-free hand-wired design

The project is open for further development.

## Releases

Latest stable release:

**Mustafa Keyboard v1.0.0**

The release includes:

- Firmware (`.uf2`)
- Windows Control Center (`.zip`)

See the [Releases](https://github.com/Mustafagio/Mustafa-keyboard/releases) page for downloads.
