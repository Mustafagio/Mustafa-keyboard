#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#if defined(CONFIG_BT)

static void bond_cleanup_pairing_failed(struct bt_conn *conn,
                                        enum bt_security_err reason)
{
    struct bt_conn_info info;
    int err;

    /*
     * Sadece eski/gecersiz eslestirme anahtarini
     * gosteren hatalarda bond temizlenir.
     *
     * Normal baglanti kopmalarinda veya genel AUTH_FAIL
     * durumunda bond silinmez.
     */
    if (reason != BT_SECURITY_ERR_PIN_OR_KEY_MISSING &&
        reason != BT_SECURITY_ERR_KEY_REJECTED) {
        return;
    }

    err = bt_conn_get_info(conn, &info);

    if (err) {
        printk("Bond cleanup: connection info alinamadi (%d)\n", err);
        return;
    }

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    printk("Bond cleanup: eski/gecersiz bond algilandi\n");

    /*
     * info.le.dst zaten bt_addr_le_t pointer'idir.
     * Bu nedenle burada '&' kullanilmaz.
     */
    err = bt_unpair(info.id, info.le.dst);

    if (err == 0) {
        printk("Bond cleanup: bond temizlendi\n");
    } else {
        printk("Bond cleanup: bond temizlenemedi (%d)\n", err);
    }
}

static struct bt_conn_auth_info_cb bond_cleanup_auth_cb = {
    .pairing_failed = bond_cleanup_pairing_failed,
};

static int mustafa_bond_cleanup_init(void)
{
    int err;

    err = bt_conn_auth_info_cb_register(&bond_cleanup_auth_cb);

    if (err) {
        printk("Bond cleanup: callback kaydi basarisiz (%d)\n", err);
        return err;
    }

    printk("Bond cleanup: aktif\n");

    return 0;
}

SYS_INIT(
    mustafa_bond_cleanup_init,
    APPLICATION,
    90
);

#endif
