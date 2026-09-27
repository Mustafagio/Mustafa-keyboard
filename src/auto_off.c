#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <zmk/event_manager.h>
#include <zmk/activity.h>
#include <zmk/events/activity_state_changed.h>

#include <zephyr/pm/pm.h>

#if defined(CONFIG_ZMK_PM_SOFT_OFF)

/* ============================================================
 * AUTO OFF
 *
 * Test süreleri:
 *
 * 0  = Kapalı
 * 2  = 2 saniye
 * 10 = 10 saniye
 * 15 = 15 saniye
 * 20 = 20 saniye
 *
 * ============================================================ */

static uint32_t auto_off_timeout_ms = 0;

static struct k_work_delayable auto_off_work;


/* ============================================================
 * AUTO OFF ÇALIŞTIR
 * ============================================================ */

static void auto_off_work_handler(
    struct k_work *work)
{
    ARG_UNUSED(work);

    if (auto_off_timeout_ms == 0) {
        return;
    }

    printk(
        "Mustafa Auto-Off: Soft Off\n"
    );

    /*
     * ZMK/Zephyr güç yönetimi üzerinden
     * sistemi soft-off durumuna geçir.
     */

    pm_state_force(
        0,
        &(struct pm_state_info){
            .state = PM_STATE_SOFT_OFF,
            .substate_id = 0,
            .min_residency_us = 0,
            .exit_latency_us = 0
        }
    );
}


/* ============================================================
 * ZAMANLAYICIYI SIFIRLA
 * ============================================================ */

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


/* ============================================================
 * ACTIVITY EVENT
 *
 * Her klavye aktivitesinde zamanlayıcı sıfırlanır.
 * ============================================================ */

static int auto_off_activity_listener(
    const zmk_event_t *eh)
{
    struct zmk_activity_state_changed *event;

    event =
        as_zmk_activity_state_changed(eh);

    if (event == NULL) {
        return 0;
    }

    switch (event->state) {

    case ZMK_ACTIVITY_ACTIVE:

        auto_off_reset_timer();

        break;

    case ZMK_ACTIVITY_IDLE:

        /*
         * Idle olduğunda timer zaten
         * çalışıyor olacak.
         */

        break;

    case ZMK_ACTIVITY_SLEEP:

        /*
         * ZMK zaten uykuya geçiyorsa
         * bizim timer'a gerek yok.
         */

        k_work_cancel_delayable(
            &auto_off_work
        );

        break;

    default:
        break;
    }

    return 0;
}


ZMK_LISTENER(
    auto_off_activity_listener,
    auto_off_activity_listener
);


ZMK_SUBSCRIPTION(
    auto_off_activity_listener,
    zmk_activity_state_changed
);


/* ============================================================
 * SÜRE AYARLA
 *
 * control_service.c tarafından çağrılacak.
 * ============================================================ */

void mustafa_auto_off_set(
    uint32_t timeout_ms)
{
    auto_off_timeout_ms =
        timeout_ms;

    k_work_cancel_delayable(
        &auto_off_work
    );

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

    /*
     * Yeni süre seçildiği anda
     * sayaç başlasın.
     */

    auto_off_reset_timer();
}


/* ============================================================
 * BAŞLANGIÇ
 * ============================================================ */

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

#endif /* CONFIG_ZMK_PM_SOFT_OFF */
