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

/* ============================================================
 * MUSTAFA KEYBOARD SERVICE
 * ============================================================ */

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


/* ============================================================
 * CONTROL CHARACTERISTIC
 * ============================================================ */

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
 *
 * ZMK nice!nano v2:
 * P0.15 = blue_led
 * ============================================================ */

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);


/* ============================================================
 * FORWARD DECLARATION
 * ============================================================ */

static void notify_active_profile(uint8_t profile);


/* ============================================================
 * GATT WRITE CALLBACK
 *
 * Commands:
 *
 * 0x01             LED ON
 * 0x02             LED OFF
 *
 * 0x10 0x00        Bluetooth Profile 1
 * 0x10 0x01        Bluetooth Profile 2
 * 0x10 0x02        Bluetooth Profile 3
 * 0x10 0x03        Bluetooth Profile 4
 * 0x10 0x04        Bluetooth Profile 5
 *
 * 0x21              Aktif profili sor
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

    /* Offset kontrolü */

    if (offset != 0) {
        return BT_GATT_ERR(
            BT_ATT_ERR_INVALID_OFFSET
        );
    }

    /* Boş paket kontrolü */

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


    /* ========================================================
     * LED ON
     * ======================================================== */

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


    /* ========================================================
     * LED OFF
     * ======================================================== */

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


    /* ========================================================
     * BLUETOOTH PROFILE SELECT
     *
     * data[0] = 0x10
     * data[1] = profile 0-4
     * ======================================================== */

    if (data[0] == 0x10) {

        if (len < 2) {

            printk(
                "Bluetooth profile command requires 2 bytes\n"
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_INVALID_ATTRIBUTE_LEN
            );
        }

        uint8_t profile = data[1];


        /* Sadece 0-4 geçerli */

        if (profile > 4) {

            printk(
                "Invalid Bluetooth profile: %d\n",
                profile
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_VALUE_NOT_ALLOWED
            );
        }


        printk(
            "Selecting Bluetooth profile: %d\n",
            profile
        );


        int ret =
            zmk_ble_prof_select(
                profile
            );


        if (ret < 0) {

            printk(
                "Bluetooth profile selection failed: %d\n",
                ret
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_UNLIKELY
            );
        }


        printk(
            "Bluetooth profile selected: %d\n",
            profile
        );

        /*
         * zmk_ble_prof_select() başarılı olduğunda
         * ZMK kendi active_profile_changed event'ini
         * yayınlayacak.
         *
         * Event listener aşağıda bunu yakalayıp
         * PC'ye 0x20 + profil gönderecek.
         */

        return len;
    }


    /* ========================================================
     * ACTIVE PROFILE QUERY
     *
     * PC:
     *
     * 0x21
     *
     * Keyboard:
     *
     * 0x20 profile
     *
     * ======================================================== */

    if (data[0] == 0x21) {

        int profile =
            zmk_ble_active_profile_index();

        if (profile < 0 ||
            profile > 4) {

            printk(
                "Invalid active profile: %d\n",
                profile
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_UNLIKELY
            );
        }

        printk(
            "Active profile requested: %d\n",
            profile
        );

        notify_active_profile(
            (uint8_t)profile
        );

        return len;
    }


    /* ========================================================
     * UNKNOWN COMMAND
     * ======================================================== */

    printk(
        "Unknown command: 0x%02X\n",
        data[0]
    );

    return len;
}


/* ============================================================
 * GATT SERVICE
 *
 * WRITE + WRITE WITHOUT RESPONSE + NOTIFY
 * ============================================================ */

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


/* ============================================================
 * NOTIFY ACTIVE PROFILE
 *
 * 0x20 0x00 = Profile 1
 * 0x20 0x01 = Profile 2
 * 0x20 0x02 = Profile 3
 * 0x20 0x03 = Profile 4
 * 0x20 0x04 = Profile 5
 * ============================================================ */

static void notify_active_profile(
    uint8_t profile)
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

    if (ret == 0) {

        printk(
            "Profile notification sent: %d\n",
            profile
        );

    } else {

        printk(
            "Profile notification failed: %d\n",
            ret
        );
    }
}


/* ============================================================
 * ZMK ACTIVE PROFILE EVENT LISTENER
 *
 * Klavyenin kendi üzerinden:
 *
 * BT_SEL 0
 * BT_SEL 1
 * BT_SEL 2
 * BT_SEL 3
 * BT_SEL 4
 *
 * kullanıldığında burası çalışır.
 * ============================================================ */

static int active_profile_listener(
    const zmk_event_t *eh)
{
    struct zmk_ble_active_profile_changed *event;

    event =
        as_zmk_ble_active_profile_changed(eh);

    if (event == NULL) {
        return 0;
    }

    printk(
        "ZMK active profile changed: %d\n",
        event->index
    );

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


/* ============================================================
 * INITIALIZE LED GPIO
 * ============================================================ */

static int mustafa_control_init(void)
{
    if (!device_is_ready(blue_led.port)) {

        printk(
            "Mustafa LED GPIO not ready\n"
        );

        return -ENODEV;
    }


    int ret =
        gpio_pin_configure_dt(
            &blue_led,
            GPIO_OUTPUT_INACTIVE
        );


    if (ret < 0) {

        printk(
            "Failed to configure Mustafa LED: %d\n",
            ret
        );

        return ret;
    }


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

#endif /* CONFIG_ZMK_BLE */
