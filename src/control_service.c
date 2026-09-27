#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>

/*
 * Mustafa Keyboard Control Service
 *
 * Service UUID:
 * 8e2f0000-7c31-4b9a-9d21-4f6e3a120001
 *
 * Control Characteristic:
 * 8e2f0001-7c31-4b9a-9d21-4f6e3a120001
 */

#define BT_UUID_MUSTAFA_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x8e2f0000, 0x7c31, 0x4b9a, \
                       0x9d, 0x21, 0x4f, 0x6e, \
                       0x3a, 0x12, 0x00, 0x01)

#define BT_UUID_MUSTAFA_CONTROL_VAL \
    BT_UUID_128_ENCODE(0x8e2f0001, 0x7c31, 0x4b9a, \
                       0x9d, 0x21, 0x4f, 0x6e, \
                       0x3a, 0x12, 0x00, 0x01)

#define BT_UUID_MUSTAFA_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_MUSTAFA_SERVICE_VAL)

#define BT_UUID_MUSTAFA_CONTROL \
    BT_UUID_DECLARE_128(BT_UUID_MUSTAFA_CONTROL_VAL)


static ssize_t control_write(
    struct bt_conn *conn,
    const struct bt_gatt_attr *attr,
    const void *buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    const uint8_t *data = buf;

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    printk("Mustafa Keyboard Control: %d byte\n", len);

    for (uint16_t i = 0; i < len; i++) {
        printk("DATA[%d] = 0x%02X\n", i, data[i]);
    }

    return len;
}


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
