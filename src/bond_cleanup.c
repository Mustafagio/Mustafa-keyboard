#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zmk/ble.h>

#if defined(CONFIG_BT)

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

static struct k_work_delayable bond_debug_work;
static struct k_work_delayable bond_auto_clear_work;

static bool bond_auto_clear_pending;
static uint8_t bond_auto_clear_profile;

static uint8_t blink_count;
static uint8_t blink_state;


/* =========================================================
 * OTOMATİK BOND TEMİZLEME HATA KODU
 *
 * Şu ana kadar gerçek cihaz testinde:
 *
 *   SECURITY ERROR = 9
 *
 * ve hemen öncesinde:
 *
 *   Rej... pairing request to taken profile 4
 *
 * görülüyor.
 *
 * Sadece bu hata kodunda otomatik bond temizleme yapılacak.
 *
 * Diğer security error'larda bond'a dokunulmayacak.
 *
 * ========================================================= */

#define BOND_AUTO_CLEAR_SECURITY_ERROR 9


/* =========================================================
 * LED HATA KODU GÖSTERİMİ
 *
 * LED kaç kez yanıp sönerse Bluetooth security error
 * reason değeri odur.
 *
 * Örnek:
 *   reason = 4  -> 4 kez
 *   reason = 5  -> 5 kez
 *   reason = 10 -> 10 kez
 *
 * ========================================================= */

static void bond_debug_work_handler(
    struct k_work *work
)
{
    ARG_UNUSED(work);

    if (!device_is_ready(blue_led.port)) {
        return;
    }

    if (blink_state == 0) {

        gpio_pin_set_dt(
            &blue_led,
            1
        );

        blink_state = 1;

        k_work_reschedule(
            &bond_debug_work,
            K_MSEC(300)
        );

        return;
    }


    gpio_pin_set_dt(
        &blue_led,
        0
    );

    blink_state = 0;


    if (blink_count > 0) {
        blink_count--;
    }


    if (blink_count > 0) {

        k_work_reschedule(
            &bond_debug_work,
            K_MSEC(300)
        );

        return;
    }


    /*
     * Hata kodu gösterildikten sonra
     * 2 saniye bekle.
     *
     * Böylece aynı hata tekrar geldiğinde
     * LED dizisini ayırt etmek kolay olur.
     */

    k_work_reschedule(
        &bond_debug_work,
        K_MSEC(2000)
    );
}


static void bond_debug_start(
    uint8_t count
)
{
    if (!device_is_ready(blue_led.port)) {
        return;
    }


    /*
     * Güvenlik:
     *
     * Çok büyük bir reason değeri gelirse
     * LED'in dakikalarca yanıp sönmesini
     * engelliyoruz.
     */

    if (count == 0) {
        count = 1;
    }

    if (count > 20) {
        count = 20;
    }


    blink_count = count;
    blink_state = 0;


    k_work_reschedule(
        &bond_debug_work,
        K_MSEC(100)
    );
}


/* =========================================================
 * OTOMATİK BOND CLEAR WORK
 * ========================================================= */

static void bond_auto_clear_work_handler(
    struct k_work *work
)
{
    ARG_UNUSED(work);


    bond_auto_clear_pending = false;


    int profile =
        zmk_ble_active_profile_index();


    /*
     * Pairing failed ile work çalışması arasında
     * profil değişmişse yanlış profili temizlememek için
     * işlemi iptal ediyoruz.
     */

    if (profile < 0 || profile > 4) {

        printk(
            "Bond cleanup: otomatik temizleme iptal - gecersiz profil = %d\n",
            profile
        );

        return;
    }


    if ((uint8_t)profile != bond_auto_clear_profile) {

        printk(
            "Bond cleanup: otomatik temizleme iptal - profil degisti (%d -> %d)\n",
            bond_auto_clear_profile,
            profile
        );

        return;
    }


    printk(
        "Bond cleanup: Profil %d otomatik temizleniyor\n",
        profile
    );


    /*
     * ZMK'nin kendi bond temizleme mekanizmasını kullan.
     *
     * zmk_ble_clear_bonds() bu ZMK sürümünde
     * void döndürür.
     *
     * Aktif profilin bond bilgisini temizler
     * ve profili tekrar boş hale getirir.
     */

    zmk_ble_clear_bonds();


    printk(
        "Bond cleanup: Profil %d otomatik temizlendi\n",
        profile
    );
}


/* =========================================================
 * PAIRING FAILED
 * ========================================================= */

static void bond_cleanup_pairing_failed(
    struct bt_conn *conn,
    enum bt_security_err reason
)
{
    int profile;


    ARG_UNUSED(conn);


    printk(
        "========================================\n"
    );

    printk(
        "Bond cleanup: pairing_failed\n"
    );

    printk(
        "Bond cleanup: SECURITY ERROR = %d\n",
        reason
    );


    /*
     * GERÇEK HATA KODUNU LED İLE GÖSTER
     */

    bond_debug_start(
        (uint8_t)reason
    );


    /* =====================================================
     * SADECE SECURITY ERROR 9
     * ===================================================== */

    if (reason != BOND_AUTO_CLEAR_SECURITY_ERROR) {

        printk(
            "Bond cleanup: bu hata icin otomatik temizleme yok\n"
        );

        printk(
            "========================================\n"
        );

        return;
    }


    /* =====================================================
     * AKTİF PROFİLİ BUL
     * ===================================================== */

    profile =
        zmk_ble_active_profile_index();


    if (profile < 0 || profile > 4) {

        printk(
            "Bond cleanup: gecersiz aktif profil = %d\n",
            profile
        );

        printk(
            "========================================\n"
        );

        return;
    }


    /*
     * Aynı hata arka arkaya gelirse aynı anda
     * birden fazla temizleme işi planlamıyoruz.
     */

    if (bond_auto_clear_pending) {

        printk(
            "Bond cleanup: otomatik temizleme zaten bekliyor\n"
        );

        printk(
            "========================================\n"
        );

        return;
    }


    bond_auto_clear_profile =
        (uint8_t)profile;

    bond_auto_clear_pending = true;


    printk(
        "Bond cleanup: Profil %d icin otomatik temizleme planlandi\n",
        profile
    );


    /*
     * Pairing failed callback'i sırasında doğrudan
     * bond'u değiştirmiyoruz.
     *
     * 500 ms bekleyip work queue üzerinden temizliyoruz.
     * Böylece mevcut failed connection'ın kapanmasına
     * zaman tanıyoruz.
     */

    k_work_reschedule(
        &bond_auto_clear_work,
        K_MSEC(500)
    );


    printk(
        "========================================\n"
    );
}


/* =========================================================
 * AUTH CALLBACK
 * ========================================================= */

static struct bt_conn_auth_info_cb bond_cleanup_auth_cb = {
    .pairing_failed =
        bond_cleanup_pairing_failed,
};


/* =========================================================
 * INIT
 * ========================================================= */

static int mustafa_bond_cleanup_init(void)
{
    int err;


    k_work_init_delayable(
        &bond_debug_work,
        bond_debug_work_handler
    );


    k_work_init_delayable(
        &bond_auto_clear_work,
        bond_auto_clear_work_handler
    );


    if (!device_is_ready(blue_led.port)) {

        printk(
            "Bond cleanup: blue LED hazir degil\n"
        );
    }


    err =
        bt_conn_auth_info_cb_register(
            &bond_cleanup_auth_cb
        );


    if (err) {

        printk(
            "Bond cleanup: callback kaydi basarisiz (%d)\n",
            err
        );

        return err;
    }


    printk(
        "Bond cleanup: aktif - OTOMATIK TEMIZLEME MODU\n"
    );


    return 0;
}


/* =========================================================
 * SYSTEM INIT
 * ========================================================= */

SYS_INIT(
    mustafa_bond_cleanup_init,
    APPLICATION,
    90
);

#endif
