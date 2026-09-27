#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <zmk/event_manager.h>
#include <zmk/activity.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/pm.h>

#if defined(CONFIG_ZMK_PM_SOFT_OFF)

/* ============================================================
 * MUSTAFA KEYBOARD - AUTO OFF
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
     * ZMK'nin kendi Soft Off mekanizmasını kullan.
     *
     * Bu, FN + ESC ile çalışan Soft Off
     * mekanizmasının aynısıdır.
     */

    zmk_pm_soft_off();
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
 * Klavye aktif olduğunda zamanlayıcı yeniden başlar.
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

        /*
         * Her tuş aktivitesinde
         * otomatik kapanma süresini
         * yeniden başlat.
         */

        auto_off_reset_timer();

        break;


    case ZMK_ACTIVITY_IDLE:

        /*
         * Timer zaten çalışıyor.
         */

        break;


    case ZMK_ACTIVITY_SLEEP:

        /*
         * ZMK başka bir güç durumuna
         * geçtiyse bizim timer'ı iptal et.
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
 * AUTO OFF SÜRESİNİ AYARLA
 *
 * control_service.c tarafından çağrılacak.
 *
 * Örnek:
 *
 * 0      = Kapalı
 * 2000   = 2 saniye
 * 10000  = 10 saniye
 * 15000  = 15 saniye
 * 20000  = 20 saniye
 * ============================================================ */

void mustafa_auto_off_set(
    uint32_t timeout_ms)
{
    auto_off_timeout_ms =
        timeout_ms;

    /*
     * Eski timer'ı iptal et.
     */

    k_work_cancel_delayable(
        &auto_off_work
    );


    /*
     * Kapalı seçildiyse
     * hiçbir timer başlatma.
     */

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
     * Yeni süreyle timer'ı başlat.
     */

    auto_off_reset_timer();
}


/* ============================================================
 * INITIALIZE
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
