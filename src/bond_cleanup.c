#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#if defined(CONFIG_BT)

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

static struct k_work_delayable bond_debug_work;

static uint8_t blink_count;
static uint8_t blink_state;


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
 * PAIRING FAILED
 * ========================================================= */

static void bond_cleanup_pairing_failed(
    struct bt_conn *conn,
    enum bt_security_err reason
)
{
    struct bt_conn_info info;
    int err;


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
     * CONNECTION INFO
     * ===================================================== */

    err = bt_conn_get_info(
        conn,
        &info
    );

    if (err) {

        printk(
            "Bond cleanup: connection info alinamadi (%d)\n",
            err
        );

        return;
    }


    if (info.type != BT_CONN_TYPE_LE) {

        printk(
            "Bond cleanup: LE olmayan connection\n"
        );

        return;
    }


    /*
     * ŞİMDİLİK HİÇBİR BOND SİLİNMİYOR.
     *
     * Bu sürüm sadece gerçek security error
     * kodunu tespit etmek için kullanılıyor.
     */

    printk(
        "Bond cleanup: test modu - bond silinmedi\n"
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
        "Bond cleanup: aktif - TEST MODU\n"
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
