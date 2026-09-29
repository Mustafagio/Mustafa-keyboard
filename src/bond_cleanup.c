#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zmk/ble.h>

#if defined(CONFIG_BT)

/* =========================================================
 * AUTOMATIC BOND CLEANUP
 *
 * Security error:
 *
 * 4 = BT_SECURITY_ERR_AUTH_REQUIREMENT
 * 9 = BT_SECURITY_ERR_UNSPECIFIED
 *
 * Bu hatalar geldiginde aktif profilin bond kaydini
 * ZMK'nin kendi bond temizleme API'si ile siliyoruz.
 * ========================================================= */

#define BOND_AUTO_CLEAR_SECURITY_ERROR_1 4
#define BOND_AUTO_CLEAR_SECURITY_ERROR_2 9


/* =========================================================
 * WORK STATE
 * ========================================================= */

static struct k_work_delayable bond_auto_clear_work;

static bool bond_auto_clear_pending = false;

static int bond_auto_clear_profile = -1;


/* =========================================================
 * AUTOMATIC BOND CLEAR WORK
 * ========================================================= */

static void bond_auto_clear_work_handler(
    struct k_work *work
)
{
    ARG_UNUSED(work);

    if (!bond_auto_clear_pending) {
        return;
    }

    bond_auto_clear_pending = false;


    /* -----------------------------------------------------
     * O sirada aktif profil degismisse islem yapma.
     * ----------------------------------------------------- */

    int active_profile =
        zmk_ble_active_profile_index();

    if (active_profile < 0 || active_profile > 4) {

        printk(
            "Bond cleanup: aktif profil gecersiz = %d\n",
            active_profile
        );

        bond_auto_clear_profile = -1;

        return;
    }


    if (active_profile != bond_auto_clear_profile) {

        printk(
            "Bond cleanup: profil degisti, otomatik temizleme iptal\n"
        );

        printk(
            "Bond cleanup: beklenen profil = %d, aktif profil = %d\n",
            bond_auto_clear_profile,
            active_profile
        );

        bond_auto_clear_profile = -1;

        return;
    }


    /* -----------------------------------------------------
     * ZMK'nin kendi bond temizleme mekanizmasi.
     *
     * zmk_ble_clear_bonds() void doner.
     * ----------------------------------------------------- */

    printk(
        "========================================\n"
    );

    printk(
        "Bond cleanup: otomatik bond temizleme basliyor\n"
    );

    printk(
        "Bond cleanup: Profil %d\n",
        active_profile
    );

    printk(
        "Bond cleanup: zmk_ble_clear_bonds()\n"
    );


    zmk_ble_clear_bonds();


    printk(
        "Bond cleanup: otomatik bond temizleme tamamlandi\n"
    );

    printk(
        "Bond cleanup: Profil %d temizlendi\n",
        active_profile
    );

    printk(
        "========================================\n"
    );


    bond_auto_clear_profile = -1;
}


/* =========================================================
 * PAIRING FAILED CALLBACK
 * ========================================================= */

static void bond_cleanup_pairing_failed(
    struct bt_conn *conn,
    enum bt_security_err reason
)
{
    ARG_UNUSED(conn);


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


    /* -----------------------------------------------------
     * Sadece 4 ve 9 numarali security error'larda
     * otomatik temizleme yap.
     *
     * 4 = AUTH_REQUIREMENT
     * 9 = UNSPECIFIED
     * ----------------------------------------------------- */

    if (
        reason != BOND_AUTO_CLEAR_SECURITY_ERROR_1 &&
        reason != BOND_AUTO_CLEAR_SECURITY_ERROR_2
    ) {

        printk(
            "Bond cleanup: bu hata icin otomatik temizleme yok\n"
        );

        printk(
            "========================================\n"
        );

        return;
    }


    /* -----------------------------------------------------
     * Aktif profil numarasini al.
     * ----------------------------------------------------- */

    int profile =
        zmk_ble_active_profile_index();


    if (profile < 0 || profile > 4) {

        printk(
            "Bond cleanup: gecersiz aktif profil = %d\n",
            profile
        );

        printk(
            "========================================\n"
        );

        return;
    }


    printk(
        "Bond cleanup: otomatik temizleme adayi Profil %d\n",
        profile
    );


    /* -----------------------------------------------------
     * Ayni anda birden fazla callback gelirse tekrar tekrar
     * temizleme planlama.
     * ----------------------------------------------------- */

    if (bond_auto_clear_pending) {

        printk(
            "Bond cleanup: zaten bekleyen otomatik temizleme var\n"
        );

        printk(
            "========================================\n"
        );

        return;
    }


    bond_auto_clear_profile = profile;

    bond_auto_clear_pending = true;


    printk(
        "Bond cleanup: Profil %d icin otomatik temizleme planlandi\n",
        profile
    );


    /* -----------------------------------------------------
     * Bluetooth callback icinde dogrudan bond temizleme
     * yapmak yerine work queue kullaniyoruz.
     *
     * 500 ms gecikme:
     * Security callback'in tamamlanmasina izin verir.
     * ----------------------------------------------------- */

    k_work_reschedule(
        &bond_auto_clear_work,
        K_MSEC(500)
    );


    printk(
        "========================================\n"
    );
}


/* =========================================================
 * BLUETOOTH AUTH CALLBACK
 * ========================================================= */

static struct bt_conn_auth_info_cb bond_cleanup_auth_cb = {
    .pairing_failed =
        bond_cleanup_pairing_failed,
};


/* =========================================================
 * INIT
 * ========================================================= */

static int bond_cleanup_init(void)
{
    /* Work item'i initialize et. */

    k_work_init_delayable(
        &bond_auto_clear_work,
        bond_auto_clear_work_handler
    );


    /* Bluetooth authentication callback'i kaydet. */

    int ret =
        bt_conn_auth_info_cb_register(
            &bond_cleanup_auth_cb
        );


    if (ret < 0) {

        printk(
            "Bond cleanup: auth callback register failed = %d\n",
            ret
        );

        return ret;
    }


    printk(
        "Bond cleanup: automatic bond cleanup initialized\n"
    );


    return 0;
}


/* =========================================================
 * SYSTEM INIT
 * ========================================================= */

SYS_INIT(
    bond_cleanup_init,
    APPLICATION,
    90
);


#endif
