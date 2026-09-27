#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/pm.h>

#if defined(CONFIG_ZMK_PM_SOFT_OFF)

/* ============================================================
 * MUSTAFA KEYBOARD - AUTO OFF
 *
 * 0      = OFF
 * 2000   = 2 seconds
 * 5000   = 5 seconds
 * 10000  = 10 seconds
 * 15000  = 15 seconds
 * 20000  = 20 seconds
 * 120000 = 2 minutes
 * 300000 = 5 minutes
 * 600000 = 10 minutes
 * 900000 = 15 minutes
 * 1200000 = 20 minutes
 * ============================================================ */

static uint32_t auto_off_timeout_ms;
static struct k_work_delayable auto_off_work;

extern void mustafa_start_shutdown_sequence(void);
extern void mustafa_cancel_shutdown_sequence(void);

static void auto_off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (auto_off_timeout_ms == 0) {
        return;
    }

    printk(
        "Mustafa Auto-Off: warning LED, then Soft Off\n"
    );

    /* Önce 2 saniyelik LED uyarısı; ardından startup_led.c Soft Off yapar. */
    mustafa_start_shutdown_sequence();
}

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

/*
 * Matrix position-state eventleri her tuş basma/bırakma olayında
 * gelir. Böylece seçilen Auto-Off süresi her tuş olayında yeniden başlar.
 */
static int auto_off_position_listener(const zmk_event_t *eh)
{
    struct zmk_position_state_changed *event;

    event = as_zmk_position_state_changed(eh);

    if (event == NULL) {
        return 0;
    }

    /* Uyarı LED'i sırasında kullanıcı tuşa basarsa kapanmayı iptal et. */
    mustafa_cancel_shutdown_sequence();

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

/* Called by control_service.c. */
void mustafa_auto_off_set(uint32_t timeout_ms)
{
    auto_off_timeout_ms = timeout_ms;

    k_work_cancel_delayable(&auto_off_work);
    mustafa_cancel_shutdown_sequence();

    if (auto_off_timeout_ms == 0) {
        printk("Mustafa Auto-Off: OFF\n");
        return;
    }

    printk(
        "Mustafa Auto-Off: %u ms\n",
        auto_off_timeout_ms
    );

    auto_off_reset_timer();
}

static int mustafa_auto_off_init(void)
{
    k_work_init_delayable(
        &auto_off_work,
        auto_off_work_handler
    );

    printk("Mustafa Auto-Off initialized\n");

    return 0;
}

SYS_INIT(
    mustafa_auto_off_init,
    APPLICATION,
    95
);

#endif /* CONFIG_ZMK_PM_SOFT_OFF */
