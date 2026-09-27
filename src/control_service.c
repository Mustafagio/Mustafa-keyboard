#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/sys/printk.h>

#if defined(CONFIG_ZMK_BLE)

#define BT_UUID_MUSTAFA_SERVICE_VAL \
    BT_UUID_128_ENCODE( \
        0x8e2f0000, \
        0x7c31, \
        0x4b9a, \
        0x9d21, \
        0x4f6e3a120001 \
    )

#define BT_UUID_MUSTAFA_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_MUSTAFA_SERVICE_VAL)

#define BT_UUID_MUSTAFA_CONTROL_VAL \
    BT_UUID_128_ENCODE( \
        0x8e2f0001, \
        0x7c31, \
        0x4b9a, \
        0x9d21, \
        0x4f6e3a120001 \
    )

#define BT_UUID_MUSTAFA_CONTROL \
    BT_UUID_DECLARE_128(BT_UUID_MUSTAFA_CONTROL_VAL)


/* ============================================================
 * ONBOARD LED
 * P0.15
 * ============================================================ */

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);


/* ============================================================
 * GATT WRITE
 *
 * Komutlar:
 *
 * 0x01 = LED AÇ
 * 0x02 = LED KAPAT
 * ============================================================ */

static ssize_t control_write(
    struct bt_conn *conn,
    const struct bt_gatt_attr *attr,
    const void *buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    const uint8_t *data = buf;

    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len == 0) {
        return 0;
    }

    printk(
        "Mustafa Control: COMMAND = 0x%02X\n",
        data[0]
    );


    /* ========================================================
     * LED AÇ
     * ======================================================== */

    if (data[0] == 0x01) {

        printk("Mustafa LED: ON\n");

        if (device_is_ready(blue_led.port)) {
            gpio_pin_set_dt(&blue_led, 1);
        }
    }


    /* ========================================================
     * LED KAPAT
     * ======================================================== */

    else if (data[0] == 0x02) {

        printk("Mustafa LED: OFF\n");

        if (device_is_ready(blue_led.port)) {
            gpio_pin_set_dt(&blue_led, 0);
        }
    }


    /* ========================================================
     * BILINMEYEN KOMUT
     * ======================================================== */

    else {

        printk(
            "Mustafa Control: UNKNOWN COMMAND 0x%02X\n",
            data[0]
        );
    }


    return len;
}


/* ============================================================
 * GATT SERVICE
 * ============================================================ */

BT_GATT_SERVICE_DEFINE(
    mustafa_control_service,

    BT_GATT_PRIMARY_SERVICE(
        BT_UUID_MUSTAFA_SERVICE
    ),

    BT_GATT_CHARACTERISTIC(
        BT_UUID_MUSTAFA_CONTROL,
        BT_GATT_CHRC_WRITE |
        BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE,
        NULL,
        control_write,
        NULL
    )
);


/* ============================================================
 * INIT
 * ============================================================ */

static int mustafa_control_init(void)
{
    if (!device_is_ready(blue_led.port)) {
        printk(
            "Mustafa LED: GPIO hazir degil!\n"
        );

        return -ENODEV;
    }

    gpio_pin_configure_dt(
        &blue_led,
        GPIO_OUTPUT_INACTIVE
    );

    printk(
        "Mustafa Control Service hazir.\n"
    );

    return 0;
}


SYS_INIT(
    mustafa_control_init,
    APPLICATION,
    90
);

#endif /* CONFIG_ZMK_BLE */
