#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/sys/printk.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

#if defined(CONFIG_ZMK_BLE)

/* =========================================================
 * MUSTAFA CONTROL SERVICE UUID
 * ========================================================= */

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

/* auto_off.c içindeki fonksiyon */
extern void mustafa_auto_off_set(
    uint32_t timeout_ms
);

/* =========================================================
 * BLUE LED
 * ========================================================= */

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

static void notify_active_profile(
    uint8_t profile
);

/* =========================================================
 * CONTROL WRITE
 * ========================================================= */

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
        return BT_GATT_ERR(
            BT_ATT_ERR_INVALID_OFFSET
        );
    }

    if (len == 0) {
        return BT_GATT_ERR(
            BT_ATT_ERR_INVALID_ATTRIBUTE_LEN
        );
    }

    printk(
        "Mustafa Keyboard Control: %d byte\n",
        len
    );

    for (uint16_t i = 0; i < len; i++) {
        printk(
            "DATA[%d] = 0x%02X\n",
            i,
            data[i]
        );
    }

    /* =====================================================
     * 0x01 = LED ON
     * ===================================================== */

    if (data[0] == 0x01) {

        if (device_is_ready(blue_led.port)) {

            gpio_pin_set_dt(
                &blue_led,
                1
            );

            printk(
                "Mustafa LED: ON\n"
            );
        }

        return len;
    }

    /* =====================================================
     * 0x02 = LED OFF
     * ===================================================== */

    if (data[0] == 0x02) {

        if (device_is_ready(blue_led.port)) {

            gpio_pin_set_dt(
                &blue_led,
                0
            );

            printk(
                "Mustafa LED: OFF\n"
            );
        }

        return len;
    }

    /* =====================================================
     * 0x10 = BLUETOOTH PROFILE SELECT
     * ===================================================== */

    if (data[0] == 0x10) {

        if (len < 2) {
            return BT_GATT_ERR(
                BT_ATT_ERR_INVALID_ATTRIBUTE_LEN
            );
        }

        uint8_t profile = data[1];

        if (profile > 4) {
            return BT_GATT_ERR(
                BT_ATT_ERR_VALUE_NOT_ALLOWED
            );
        }

        int ret =
            zmk_ble_prof_select(profile);

        if (ret < 0) {
            return BT_GATT_ERR(
                BT_ATT_ERR_UNLIKELY
            );
        }

        printk(
            "Bluetooth profile selected: %d\n",
            profile
        );

        return len;
    }

    /* =====================================================
     * 0x21 = ACTIVE PROFILE REQUEST
     * ===================================================== */

    if (data[0] == 0x21) {

        int profile =
            zmk_ble_active_profile_index();

        if (profile < 0 || profile > 4) {
            return BT_GATT_ERR(
                BT_ATT_ERR_UNLIKELY
            );
        }

        notify_active_profile(
            (uint8_t)profile
        );

        return len;
    }

    /* =====================================================
     * 0x30 = AUTO-OFF
     *
     * 0x30 0x00 = KAPALI
     * 0x30 0x02 = 2 saniye
     * 0x30 0x05 = 5 saniye
     * 0x30 0x0A = 10 saniye
     * 0x30 0x0F = 15 saniye
     * 0x30 0x14 = 20 saniye
     * ===================================================== */

    if (data[0] == 0x30) {

        if (len < 2) {
            return BT_GATT_ERR(
                BT_ATT_ERR_INVALID_ATTRIBUTE_LEN
            );
        }

        uint8_t setting = data[1];

        uint32_t timeout_ms = 0;

        switch (setting) {

        case 0x00:
            timeout_ms = 0;
            break;

        case 0x02:
            timeout_ms = 2000;
            break;

        case 0x05:
            timeout_ms = 5000;
            break;

        case 0x0A:
            timeout_ms = 10000;
            break;

        case 0x0F:
            timeout_ms = 15000;
            break;

        case 0x14:
            timeout_ms = 20000;
            break;

        default:
            return BT_GATT_ERR(
                BT_ATT_ERR_VALUE_NOT_ALLOWED
            );
        }

        mustafa_auto_off_set(
            timeout_ms
        );

        printk(
            "Mustafa Auto-Off setting: 0x%02X (%u ms)\n",
            setting,
            timeout_ms
        );

        return len;
    }

    return len;
}

/* =========================================================
 * GATT SERVICE
 * ========================================================= */

BT_GATT_SERVICE_DEFINE(
    mustafa_control_service,

    BT_GATT_PRIMARY_SERVICE(
        BT_UUID_MUSTAFA_SERVICE
    ),

    BT_GATT_CHARACTERISTIC(
        BT_UUID_MUSTAFA_CONTROL,

        BT_GATT_CHRC_WRITE |
        BT_GATT_CHRC_WRITE_WITHOUT_RESP |
        BT_GATT_CHRC_NOTIFY,

        BT_GATT_PERM_WRITE,

        NULL,
        control_write,
        NULL
    ),

    BT_GATT_CCC(
        NULL,

        BT_GATT_PERM_READ |
        BT_GATT_PERM_WRITE
    )
);

/* =========================================================
 * ACTIVE PROFILE NOTIFY
 * ========================================================= */

static void notify_active_profile(
    uint8_t profile
)
{
    uint8_t data[2] =
    {
        0x20,
        profile
    };

    int ret =
        bt_gatt_notify(
            NULL,
            &mustafa_control_service.attrs[2],
            data,
            sizeof(data)
        );

    if (ret < 0) {

        printk(
            "Profile notify failed: %d\n",
            ret
        );
    }
}

/* =========================================================
 * ACTIVE PROFILE EVENT
 * ========================================================= */

static int active_profile_listener(
    const zmk_event_t *eh
)
{
    struct zmk_ble_active_profile_changed *event =
        as_zmk_ble_active_profile_changed(eh);

    if (event == NULL) {
        return 0;
    }

    if (event->index > 4) {
        return 0;
    }

    notify_active_profile(
        event->index
    );

    return 0;
}

ZMK_LISTENER(
    active_profile_listener,
    active_profile_listener
);

ZMK_SUBSCRIPTION(
    active_profile_listener,
    zmk_ble_active_profile_changed
);

/* =========================================================
 * INIT
 * ========================================================= */

static int mustafa_control_init(void)
{
    if (!device_is_ready(blue_led.port)) {
        return -ENODEV;
    }

    /*
     * ÖNEMLİ:
     * Burada LED yeniden configure edilmiyor.
     * startup_led.c zaten GPIO'yu configure ediyor.
     */

    printk(
        "Mustafa Control Service initialized\n"
    );

    return 0;
}

SYS_INIT(
    mustafa_control_init,
    APPLICATION,
    90
);

#endif
