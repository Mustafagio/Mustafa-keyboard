# Mustafa Keyboard

64 tuşlu, Türkçe Q düzenine sahip, kablosuz Bluetooth mekanik klavye.

Bu proje PCB kullanılmadan, el kablolaması (hand-wiring) yöntemiyle geliştirilmiş özel yapım bir mekanik klavyedir.

## Özellikler

- 64 mekanik switch
- Türkçe Q düzeni
- ZMK firmware
- nRF52840 tabanlı kontrolcü
- Bluetooth Low Energy
- 5 Bluetooth profil/bond yuvası
- Bluetooth profil değiştirme
- Kalıcı Auto-Off ayarı
- Soft Off
- FN + ESC ile Soft Off
- ESC ile Soft Off'tan uyanma
- LED kontrolü
- 5 × 14 matrix
- PCB'siz hand-wired tasarım
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

Firmware, ZMK Firmware üzerine kuruludur.

ZMK Firmware:
https://zmk.dev/

Kullanılan board:

    nice_nano_v2

Kullanılan shield:

    mustafa_keyboard

Bluetooth cihaz adı:

    Mustafa KB

## Türkçe Q Layout

Klavye Türkçe Q düzenine göre yapılandırılmıştır.

Desteklenen Türkçe karakterler:

- Ğ
- Ü
- Ş
- İ
- Ö
- Ç
- ı

Türkçe karakterlerin ZMK tarafındaki tanımları:

    boards/shields/mustafa_keyboard/keys_tr.h

Ana keymap:

    boards/shields/mustafa_keyboard/mustafa_keyboard.keymap

## Bluetooth

Klavye 5 farklı Bluetooth profilini destekler.

Profil yuvaları:

    Profile 0
    Profile 1
    Profile 2
    Profile 3
    Profile 4

Her profil farklı bir Bluetooth cihazıyla eşleştirilebilir.

Profil seçimi klavye üzerindeki Bluetooth profil tuşlarıyla yapılabilir.

Bu sayede klavye birden fazla bilgisayar veya cihaz arasında kullanılabilir.

## Auto-Off

Klavye otomatik kapanma özelliğine sahiptir.

Desteklenen süreler:

- 2 dakika
- 5 dakika
- 10 dakika
- 15 dakika
- 20 dakika
- Kapalı

Auto-Off ayarı kalıcı olarak kaydedilir.

Klavye yeniden başlatıldığında veya Bluetooth bağlantısı yeniden kurulduğunda son kullanılan Auto-Off ayarı korunur.

Auto-Off firmware tarafında:

    src/auto_off.c

dosyası tarafından yönetilir.

## Soft Off

Soft Off özelliği:

    FN + ESC

ile etkinleştirilebilir.

Soft Off durumundan:

    ESC

tuşuna basılarak uyanılabilir.

## LED Kontrolü

Başlangıç LED kontrolü ve kapanma uyarısı:

    src/startup_led.c

dosyasında bulunur.

Klavye kontrol servisi:

    src/control_service.c

dosyasında bulunur.

## Matrix

Klavye 5 × 14 matrix kullanmaktadır.

Diyot yönü:

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

## Matrix GPIO Yapılandırması

Matrix yapılandırması:

    boards/shields/mustafa_keyboard/mustafa_keyboard.overlay

dosyasında bulunur.

Keymap:

    boards/shields/mustafa_keyboard/mustafa_keyboard.keymap

dosyasında bulunur.

## Proje Yapısı

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

## Firmware Derleme

Firmware GitHub Actions kullanılarak otomatik olarak derlenebilir.

Build hedefi:

    nice_nano_v2

Shield:

    mustafa_keyboard

Build yapılandırması:

    zephyr/build.yaml

dosyasında bulunmaktadır.

Ayrıca ZMK ayarlarını sıfırlamak için:

    settings_reset

firmware'i de build yapılandırmasında bulunmaktadır.

## GitHub Actions

Projeye yapılan değişikliklerden sonra GitHub Actions firmware'i otomatik olarak derler.

Workflow dosyaları:

    .github/workflows/

klasöründe bulunur.

Build başarılı olduğunda oluşturulan firmware dosyası GitHub Actions üzerinden indirilebilir.

## Dosyalar

### mustafa_keyboard.overlay

Klavye GPIO ve matrix yapılandırmasını içerir.

### mustafa_keyboard.keymap

Klavye tuşlarının ZMK keymap yapılandırmasını içerir.

### keys_tr.h

Türkçe Q düzeni için özel tuş tanımlarını içerir.

### auto_off.c

Auto-Off fonksiyonlarını ve kalıcı Auto-Off ayarını yönetir.

### control_service.c

Klavye ile kontrol uygulaması arasındaki Bluetooth GATT kontrol servislerini yönetir.

### startup_led.c

Başlangıç LED'i ve kapanma uyarısı fonksiyonlarını yönetir.

## Uyarı

Bu proje özel yapım bir klavyedir.

GPIO bağlantılarını veya matrix kablolamasını değiştirmeden önce firmware yapılandırması kontrol edilmelidir.

ROW ve COLUMN bağlantılarının değiştirilmesi mevcut firmware ile uyumsuzluğa neden olabilir.

## Proje Durumu

Çalışan özellikler:

- Türkçe Q layout
- Bluetooth bağlantısı
- 5 Bluetooth profil yuvası
- Bluetooth profil değiştirme
- Auto-Off
- Kalıcı Auto-Off ayarı
- Soft Off
- LED kontrolü
- 5 × 14 matrix
- PCB'siz hand-wired yapı

Proje geliştirmeye açıktır.
