#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

static void bond_cleanup_pairing_failed(struct bt_conn *conn,
                                        enum bt_security_err reason)
{
    struct bt_conn_info info;
    int err;

    /*
     * İlk aşamada sadece eski/geçersiz eşleştirme anahtarını
     * gösteren hatalarda bond temizliyoruz.
     *
     * Normal bağlantı kopmalarında veya genel AUTH_FAIL durumunda
     * bond silinmez.
     */
    if (reason != BT_SECURITY_ERR_PIN_OR_KEY_MISSING &&
        reason != BT_SECURITY_ERR_KEY_REJECTED) {
        return;
    }

    err = bt_conn_get_info(conn, &info);
    if (err) {
        printk("Bond cleanup: connection info alınamadı (%d)\n", err);
        return;
    }

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    printk("Bond cleanup: eski/gecersiz bond algılandı\n");

    err = bt_unpair(info.id, &info.le.dst);

    if (err == 0) {
        printk("Bond cleanup: bond temizlendi\n");
    } else {
        printk("Bond cleanup: bond temizlenemedi (%d)\n", err);
    }
}

static struct bt_conn_auth_info_cb bond_cleanup_auth_cb = {
    .pairing_failed = bond_cleanup_pairing_failed,
};

int mustafa_bond_cleanup_init(void)
{
    int err;

    err = bt_conn_auth_info_cb_register(&bond_cleanup_auth_cb);

    if (err) {
        printk("Bond cleanup: callback kaydı başarısız (%d)\n", err);
        return err;
    }

    printk("Bond cleanup: aktif\n");

    return 0;
}
