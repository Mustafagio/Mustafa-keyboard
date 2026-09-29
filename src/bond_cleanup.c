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

/*
 * LED hata teşhisi
 *
 * 1 = PIN_OR_KEY_MISSING
 * 2 = KEY_REJECTED
 * 3 = AUTH_FAIL
 *
 * Diğer hata kodlarında:
 * 4 = OTHER
 *
 * Bu sürümde OTHER durumunda gerçek
 * enum değerini de printk ile yazdırıyoruz.
 */

static void bond_debug_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!device_is_ready(blue_led.port)) {
        return;
    }

    if (blink_state == 0) {
        gpio_pin_set_dt(&blue_led, 1);
        blink_state = 1;

        k_work_reschedule(
            &bond_debug_work,
            K_MSEC(250)
        );

        return;
    }

    gpio_pin_set_dt(&blue_led, 0);
    blink_state = 0;

    if (blink_count > 0) {
        blink_count--;
    }

    if (blink_count > 0) {
        k_work_reschedule(
            &bond_debug_work,
            K_MSEC(250)
        );
    }
}

static void bond_debug_start(uint8_t count)
{
    if (!device_is_ready(blue_led.port)) {
        return;
    }

    blink_count = count;
    blink_state = 0;

    k_work_reschedule(
        &bond_debug_work,
        K_MSEC(100)
    );
}

static void bond_cleanup_pairing_failed(
    struct bt_conn *conn,
    enum bt_security_err reason
)
{
    struct bt_conn_info info;
    int err;

    printk(
        "Bond cleanup: pairing_failed reason = %d\n",
        reason
    );

    switch (reason) {

    case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:

        printk(
            "Bond cleanup: PIN_OR_KEY_MISSING\n"
        );

        bond_debug_start(1);

        break;

    case BT_SECURITY_ERR_KEY_REJECTED:

        printk(
            "Bond cleanup: KEY_REJECTED\n"
        );

        bond_debug_start(2);

        break;

    case BT_SECURITY_ERR_AUTH_FAIL:

        printk(
            "Bond cleanup: AUTH_FAIL\n"
        );

        bond_debug_start(3);

        break;

    default:

        printk(
            "Bond cleanup: OTHER SECURITY ERROR = %d\n",
            reason
        );

        bond_debug_start(4);

        break;
    }

    /*
     * Şimdilik hiçbir bond'u silmiyoruz.
     *
     * Önce gerçek hata kodunu kesin olarak
     * tespit edeceğiz.
     */

    if (reason != BT_SECURITY_ERR_PIN_OR_KEY_MISSING &&
        reason != BT_SECURITY_ERR_KEY_REJECTED) {

        return;
    }

    err = bt_conn_get_info(conn, &info);

    if (err) {

        printk(
            "Bond cleanup: connection info alinamadi (%d)\n",
            err
        );

        return;
    }

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    printk(
        "Bond cleanup: eski/gecersiz bond algilandi\n"
    );

    /*
     * Bu test sürümünde gerçek silme işlemi
     * yapılmıyor.
     *
     * Sadece bilgi topluyoruz.
     */

    printk(
        "Bond cleanup: bond temizleme testi\n"
    );
}

static struct bt_conn_auth_info_cb bond_cleanup_auth_cb = {
    .pairing_failed = bond_cleanup_pairing_failed,
};

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

    err = bt_conn_auth_info_cb_register(
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
        "Bond cleanup: aktif\n"
    );

    return 0;
}

SYS_INIT(
    mustafa_bond_cleanup_init,
    APPLICATION,
    90
);

#endif
