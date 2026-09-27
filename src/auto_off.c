#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/pm.h>

#if defined(CONFIG_ZMK_PM_SOFT_OFF)

/* =========================================================
 * STARTUP_LED.C API
 * ========================================================= */

extern void mustafa_shutdown_warning_start(
    void (*callback)(void)
);

extern void mustafa_shutdown_warning_cancel(void);

/* =========================================================
 * AUTO-OFF DURUMU
 * ========================================================= */

static uint32_t auto_off_timeout_ms = 0;

static struct k_work_delayable auto_off_work;

static bool shutdown_warning_active = false;

/* =========================================================
 * 2 SANİYELİK LED UYARISI BİTTİKTEN SONRA
 * ========================================================= */

static void auto_off_soft_off_complete(void)
{
    /*
     * Auto-Off bu sırada kapatılmışsa Soft Off yapma.
     */
    if (auto_off_timeout_ms == 0) {
        shutdown_warning_active = false;
        return;
    }

    shutdown_warning_active = false;

    printk(
        "Mustafa Auto-Off: Soft Off\n"
    );

    /* Soft Off işleminin sahibi auto_off.c'dir. */
    zmk_pm_soft_off();
}

/* =========================================================
 * AUTO-OFF TIMER
 * ========================================================= */

static void auto_off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (auto_off_timeout_ms == 0) {
        return;
    }

    /*
     * Süre doldu.
     * Önce 2 saniyelik LED uyarısını başlat.
     * LED animasyonu tamamlanınca callback Soft Off yapacak.
     */
    shutdown_warning_active = true;

    printk(
        "Mustafa Auto-Off: 2 second warning\n"
    );

    mustafa_shutdown_warning_start(
        auto_off_soft_off_complete
    );
}

/* =========================================================
 * AUTO-OFF TIMER'I YENİDEN BAŞLAT
 * ========================================================= */

static void auto_off_reset_timer(void)
{
    if (auto_off_timeout_ms == 0) {
        k_work_cancel_delayable(&auto_off_work);
        return;
    }

    k_work_reschedule(
        &auto_off_work,
        K_MSEC(auto_off_timeout_ms)
    );
}

/* =========================================================
 * TUŞ / MATRIX EVENT
 *
 * Herhangi bir key event geldiğinde:
 * - Eğer LED uyarısı çalışıyorsa iptal edilir.
 * - Auto-Off sayacı yeniden başlar.
 * ========================================================= */

static int auto_off_position_listener(const zmk_event_t *eh)
{
    struct zmk_position_state_changed *event;

    event = as_zmk_position_state_changed(eh);

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
 * WPF TARAFINDAN AUTO-OFF AYARI
 *
 * timeout_ms:
 * 0      = Kapalı
 * 2000   = 2 saniye
 * 5000   = 5 saniye
 * 10000  = 10 saniye
 * 15000  = 15 saniye
 * 20000  = 20 saniye
 * ========================================================= */

void mustafa_auto_off_set(uint32_t timeout_ms)
{
    auto_off_timeout_ms = timeout_ms;

    /* Eski timer'ı iptal et. */
    k_work_cancel_delayable(&auto_off_work);

    /* Devam eden LED uyarısı varsa onu da iptal et. */
    shutdown_warning_active = false;
    mustafa_shutdown_warning_cancel();

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
