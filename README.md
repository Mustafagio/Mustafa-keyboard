# Mustafa Keyboard

64 tuşlu, Türkçe Q düzenine sahip, kablosuz Bluetooth mekanik klavye.

Bu proje, PCB kullanılmadan el kablolaması (hand-wiring) yöntemiyle hazırlanmış özel yapım bir mekanik klavyedir.

## Özellikler

- 64 mekanik switch
- Türkçe Q klavye düzeni
- ZMK firmware
- nRF52840 tabanlı kontrolcü
- Bluetooth Low Energy
- 5 farklı Bluetooth profil/bond yuvası
- Profil değiştirme
- Kalıcı Auto-Off ayarı
- Soft Off
- FN + ESC ile Soft Off
- ESC ile Soft Off'tan uyanma
- Dahili RGB/LED kontrolü
- PCB'siz hand-wired matrix
- 3D baskı gövde

## Donanım

- Robiz nRF52840 Pro Micro V1.840
- 64 × mekanik switch
- Diyotlar
- 18650 Li-ion pil
- TP4056 şarj modülü
- 3D baskı klavye gövdesi
- El kablolaması

## Firmware

Firmware, [ZMK Firmware](https://zmk.dev/) üzerine kuruludur.

Kullanılan kart:

`nice_nano_v2`

Shield:

`mustafa_keyboard`

Bluetooth cihaz adı:

`Mustafa KB`

## Türkçe Q Layout

Klavye standart Türkçe Q düzenine göre yapılandırılmıştır.

Özel Türkçe karakterler:

- Ğ
- Ü
- Ş
- İ
- Ö
- Ç
- ı

## Bluetooth

Klavye 5 farklı Bluetooth profilini destekler.

Profil yuvaları:

- Profile 0
- Profile 1
- Profile 2
- Profile 3
- Profile 4

Bu sayede klavye birden fazla bilgisayar veya cihazla eşleştirilebilir.

## Auto-Off

Klavye üzerinde otomatik kapanma özelliği bulunmaktadır.

Desteklenen süreler:

- 2 dakika
- 5 dakika
- 10 dakika
- 15 dakika
- 20 dakika
- Kapalı

Auto-Off ayarı kalıcı olarak kaydedilir.

Klavye kapatılıp tekrar açıldığında son kullanılan Auto-Off ayarı korunur.

## Soft Off

Soft Off özelliği:

`FN + ESC`

ile etkinleştirilebilir.

Soft Off durumundan:

`ESC`

tuşuna basılarak uyanılabilir.

## Matrix

Klavye 5 × 14 matrix kullanmaktadır.

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

Diyot yönü:

`COL2ROW`

## Proje Yapısı

```text
Mustafa-keyboard/
│
├── config/
│   ├── mustafa_keyboard.conf
│   ├── mustafa_keyboard.keymap
│   └── ...
│
├── boards/
│   └── shields/
│       └── mustafa_keyboard/
│
├── src/
│   ├── auto_off.c
│   ├── control_service.c
│   └── startup_led.c
│
├── zephyr/
│   └── build.yaml
│
├── CMakeLists.txt
├── Kconfig.defconfig
├── Kconfig.shield
└── README.md
