#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/sys/printk.h>

#include <zmk/ble.h>


#if defined(CONFIG_ZMK_BLE)


/* =========================================================
 * MUSTAFA SERVICE UUID
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
    BT_UUID_DECLARE_128( \
        BT_UUID_MUSTAFA_SERVICE_VAL \
    )


/* =========================================================
 * MUSTAFA CONTROL UUID
 * ========================================================= */

#define BT_UUID_MUSTAFA_CONTROL_VAL \
    BT_UUID_128_ENCODE( \
        0x8e2f0001, \
        0x7c31, \
        0x4b9a, \
        0x9d21, \
        0x4f6e3a120001 \
    )

#define BT_UUID_MUSTAFA_CONTROL \
    BT_UUID_DECLARE_128( \
        BT_UUID_MUSTAFA_CONTROL_VAL \
    )


/* =========================================================
 * LED
 * ========================================================= */

extern void mustafa_led_set(bool on);


/* =========================================================
 * GATT WRITE
 *
 * Komutlar:
 *
 * 0x01              = LED AÇ
 * 0x02              = LED KAPAT
 *
 * 0x10 0x00         = Profil 1
 * 0x10 0x01         = Profil 2
 * 0x10 0x02         = Profil 3
 * 0x10 0x03         = Profil 4
 * 0x10 0x04         = Profil 5
 *
 * ========================================================= */

static ssize_t control_write(
    struct bt_conn *conn,
    const struct bt_gatt_attr *attr,
    const void *buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags
)
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
        return 0;
    }


    printk(
        "Mustafa Control: COMMAND = 0x%02X\n",
        data[0]
    );


    /* =====================================================
     * LED AÇ
     * ===================================================== */

    if (data[0] == 0x01) {

        printk(
            "Mustafa LED: ON\n"
        );

        mustafa_led_set(true);

        return len;
    }


    /* =====================================================
     * LED KAPAT
     * ===================================================== */

    if (data[0] == 0x02) {

        printk(
            "Mustafa LED: OFF\n"
        );

        mustafa_led_set(false);

        return len;
    }


    /* =====================================================
     * BLUETOOTH PROFİL
     *
     * Beklenen veri:
     *
     * [0] = 0x10
     * [1] = 0..4
     * ===================================================== */

    if (data[0] == 0x10) {

        if (len < 2) {

            printk(
                "Mustafa BT: Eksik profil komutu\n"
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_INVALID_ATTRIBUTE_LEN
            );
        }


        uint8_t profile = data[1];


        if (profile > 4) {

            printk(
                "Mustafa BT: Gecersiz profil %d\n",
                profile
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_VALUE_NOT_ALLOWED
            );
        }


        printk(
            "Mustafa BT: Profil %d seciliyor\n",
            profile + 1
        );


        int ret =
            zmk_ble_prof_select(profile);


        if (ret < 0) {

            printk(
                "Mustafa BT: Profil secme hatasi %d\n",
                ret
            );

            return BT_GATT_ERR(
                BT_ATT_ERR_UNLIKELY
            );
        }


        printk(
            "Mustafa BT: Profil %d secildi\n",
            profile + 1
        );


        return len;
    }


    /* =====================================================
     * BİLİNMEYEN KOMUT
     * ===================================================== */

    printk(
        "Mustafa Control: "
        "UNKNOWN COMMAND 0x%02X\n",
        data[0]
    );


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
        BT_GATT_CHRC_WRITE_WITHOUT_RESP,

        BT_GATT_PERM_WRITE,

        NULL,

        control_write,

        NULL
    )
);


#endif /* CONFIG_ZMK_BLE */
