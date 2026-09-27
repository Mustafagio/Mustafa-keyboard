#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/pm.h>

#if defined(CONFIG_ZMK_PM_SOFT_OFF)

/* =========================================================
 * STARTUP LED API
 * ========================================================= */

extern void mustafa_shutdown_warning_start(
    void (*callback)(void)
);

extern void mustafa_shutdown_warning_cancel(void);

/* =========================================================
 * AUTO-OFF
 * ========================================================= */

static uint32_t auto_off_timeout_ms = 0;

static struct k_work_delayable auto_off_work;

static bool shutdown_warning_active = false;

/* =========================================================
 * SETTINGS
 *
 * Kalıcı ayar:
 *
 * mustafa/auto_off
 *
 * Değer:
 * 0       = Kapalı
 * 2000    = 2 saniye
 * 5000    = 5 saniye
 * 10000   = 10 saniye
 * 15000   = 15 saniye
 * 20000   = 20 saniye
 * ========================================================= */

static int auto_off_settings_set(
    const char *key,
    size_t len,
    settings_read_cb read_cb,
    void *cb_arg
)
{
    int rc;

    if (settings_name_steq(
            key,
            "auto_off",
            NULL
        )) {

        if (len != sizeof(auto_off_timeout_ms)) {
            return -EINVAL;
        }

        rc = read_cb(
            cb_arg,
            &auto_off_timeout_ms,
            sizeof(auto_off_timeout_ms)
        );

        if (rc >= 0) {
            printk(
                "Mustafa Auto-Off: Settings loaded: %u ms\n",
                auto_off_timeout_ms
            );

            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

static int auto_off_settings_commit(void)
{
    printk(
        "Mustafa Auto-Off: Settings commit: %u ms\n",
        auto_off_timeout_ms
    );

    if (auto_off_timeout_ms != 0) {

        k_work_reschedule(
            &auto_off_work,
            K_MSEC(auto_off_timeout_ms)
        );
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(
    mustafa_auto_off,
    "mustafa",
    NULL,
    auto_off_settings_set,
    auto_off_settings_commit,
    NULL
);

/* =========================================================
 * LED UYARISI BİTTİ
 * ========================================================= */

static void auto_off_soft_off_complete(void)
{
    if (auto_off_timeout_ms == 0) {

        shutdown_warning_active = false;

        return;
    }

    shutdown_warning_active = false;

    printk(
        "Mustafa Auto-Off: Soft Off\n"
    );

    /*
     * Soft Off işlemi SADECE burada yapılır.
     */
    zmk_pm_soft_off();
}

/* =========================================================
 * TIMER
 * ========================================================= */

static void auto_off_work_handler(
    struct k_work *work
)
{
    ARG_UNUSED(work);

    if (auto_off_timeout_ms == 0) {
        return;
    }

    shutdown_warning_active = true;

    printk(
        "Mustafa Auto-Off: 2 second warning\n"
    );

    mustafa_shutdown_warning_start(
        auto_off_soft_off_complete
    );
}

/* =========================================================
 * TIMER RESET
 * ========================================================= */

static void auto_off_reset_timer(void)
{
    if (auto_off_timeout_ms == 0) {

        k_work_cancel_delayable(
            &auto_off_work
        );

        return;
    }

    k_work_reschedule(
        &auto_off_work,
        K_MSEC(auto_off_timeout_ms)
    );
}

/* =========================================================
 * KEY EVENT
 *
 * Bir tuşa basılırsa:
 *
 * AUTO-OFF timer reset
 * LED uyarısı varsa iptal
 * ========================================================= */

static int auto_off_position_listener(
    const zmk_event_t *eh
)
{
    struct zmk_position_state_changed *event;

    event =
        as_zmk_position_state_changed(eh);

    if (event == NULL) {
        return 0;
    }

    if (shutdown_warning_active) {

        shutdown_warning_active = false;

        mustafa_shutdown_warning_cancel();
    }

    auto_off_reset_timer();

    return 0;
}

ZMK_LISTENER(
    auto_off_position_listener,
    auto_off_position_listener
);

ZMK_SUBSCRIPTION(
    auto_off_position_listener,
    zmk_position_state_changed
);

/* =========================================================
 * WPF AUTO-OFF AYARI
 * ========================================================= */

void mustafa_auto_off_set(
    uint32_t timeout_ms
)
{
    int rc;

    auto_off_timeout_ms =
        timeout_ms;

    /*
     * Önce mevcut timer'ı durdur.
     */
    k_work_cancel_delayable(
        &auto_off_work
    );

    shutdown_warning_active = false;

    mustafa_shutdown_warning_cancel();

    /*
     * Ayarı FLASH SETTINGS'e kaydet.
     *
     * Böylece Soft Off / yeniden başlatma sonrasında
     * değer kaybolmaz.
     */
    rc = settings_save_one(
        "mustafa/auto_off",
        &auto_off_timeout_ms,
        sizeof(auto_off_timeout_ms)
    );

    if (rc != 0) {

        printk(
            "Mustafa Auto-Off: Settings save FAILED: %d\n",
            rc
        );

    } else {

        printk(
            "Mustafa Auto-Off: Settings saved: %u ms\n",
            auto_off_timeout_ms
        );
    }

    if (auto_off_timeout_ms == 0) {

        printk(
            "Mustafa Auto-Off: OFF\n"
        );

        return;
    }

    printk(
        "Mustafa Auto-Off: %u ms\n",
        auto_off_timeout_ms
    );

    auto_off_reset_timer();
}

/* =========================================================
 * INIT
 * ========================================================= */

static int mustafa_auto_off_init(void)
{
    k_work_init_delayable(
        &auto_off_work,
        auto_off_work_handler
    );

    printk(
        "Mustafa Auto-Off initialized\n"
    );

    return 0;
}

SYS_INIT(
    mustafa_auto_off_init,
    APPLICATION,
    95
);

#endif