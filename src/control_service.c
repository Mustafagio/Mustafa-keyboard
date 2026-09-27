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
 * ONBOARD LED - P0.15
 * nice!nano / Robiz nRF52840 Pro Micro
 * ============================================================ */

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);


/* LED'i 1 saniye sonra kapatmak için work */
static struct k_work_delayable led_off_work;


static void led_off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(&blue_led, 0);
}


/* ============================================================
 * GATT WRITE
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

    printk("Mustafa Keyboard Control: %d byte\n", len);

    for (uint16_t i = 0; i < len; i++) {
        printk("DATA[%d] = 0x%02X\n", i, data[i]);
    }


    /* ========================================================
     * TEST KOMUTU
     *
     * PC'den:
     *
     * 0xAA
     *
     * gelirse LED'i 1 saniye yak.
     * ======================================================== */

    if (len >= 1 && data[0] == 0xAA) {

        printk("MUSTAFA TEST: 0xAA ALINDI!\n");

        if (device_is_ready(blue_led.port)) {

            gpio_pin_set_dt(&blue_led, 1);

            k_work_reschedule(
                &led_off_work,
                K_SECONDS(1)
            );
        }
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
        printk("Mustafa LED: GPIO hazir degil!\n");
        return -ENODEV;
    }

    gpio_pin_configure_dt(
        &blue_led,
        GPIO_OUTPUT_INACTIVE
    );

    k_work_init_delayable(
        &led_off_work,
        led_off_work_handler
    );

    printk("Mustafa Control Service hazir.\n");

    return 0;
}

SYS_INIT(
    mustafa_control_init,
    APPLICATION,
    90
);

#endif /* CONFIG_ZMK_BLE */
