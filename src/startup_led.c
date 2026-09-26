#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <zmk/event_manager.h>
#include <zmk/activity.h>
#include <zmk/events/activity_state_changed.h>

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

/* ---------------------------------------------------------
 * AÇILIŞ LED'İ
 * --------------------------------------------------------- */

static struct k_work_delayable startup_led_off_work;

static void startup_led_off(struct k_work *work)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(&startup_led, 0);
}


/* ---------------------------------------------------------
 * OTOMATİK KAPANMA LED ANİMASYONU
 *
 * 2 saniye boyunca hızlı yanıp söner.
 * 100 ms ON / 100 ms OFF
 * --------------------------------------------------------- */

static struct k_work_delayable shutdown_led_work;

static int shutdown_blink_count = 0;

static void shutdown_led_blink(struct k_work *work)
{
    ARG_UNUSED(work);

    if (shutdown_blink_count >= 20) {
        gpio_pin_set_dt(&startup_led, 0);
        return;
    }

    gpio_pin_toggle_dt(&startup_led);

    shutdown_blink_count++;

    k_work_schedule(
        &shutdown_led_work,
        K_MSEC(100)
    );
}


/* ---------------------------------------------------------
 * ACTIVITY EVENT
 *
 * Klavye 5 dakika kullanılmadığında:
 *
 * ACTIVE
 *   ↓
 * IDLE
 *   ↓
 * LED hızlı yanıp söner
 *   ↓
 * 2 saniye sonra ZMK deep sleep
 * --------------------------------------------------------- */

static int led_activity_listener(const zmk_event_t *eh)
{
    struct zmk_activity_state_changed *event;

    event = as_zmk_activity_state_changed(eh);

    if (event == NULL) {
        return 0;
    }

    switch (event->state) {

    case ZMK_ACTIVITY_ACTIVE:

        /*
         * Bir tuşa basıldı.
         * Olası kapanma animasyonunu iptal et.
         */

        k_work_cancel_delayable(&shutdown_led_work);

        shutdown_blink_count = 0;

        gpio_pin_set_dt(&startup_led, 0);

        break;


    case ZMK_ACTIVITY_IDLE:

        /*
         * 5 dakika boyunca hiçbir tuşa basılmadı.
         * Kapanış animasyonunu başlat.
         */

        shutdown_blink_count = 0;

        k_work_cancel_delayable(&shutdown_led_work);

        k_work_schedule(
            &shutdown_led_work,
            K_NO_WAIT
        );

        break;


    case ZMK_ACTIVITY_SLEEP:

        /*
         * ZMK artık power-off durumuna geçiyor.
         */

        k_work_cancel_delayable(&shutdown_led_work);

        gpio_pin_set_dt(&startup_led, 0);

        break;


    default:
        break;
    }

    return 0;
}


ZMK_LISTENER(
    led_activity_listener,
    led_activity_listener
);

ZMK_SUBSCRIPTION(
    led_activity_listener,
    zmk_activity_state_changed
);


/* ---------------------------------------------------------
 * STARTUP LED INIT
 * --------------------------------------------------------- */

static int startup_led_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&startup_led)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &startup_led,
        GPIO_OUTPUT_INACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    /*
     * Açılışta LED'i yak.
     */

    ret = gpio_pin_set_dt(
        &startup_led,
        1
    );

    if (ret < 0) {
        return ret;
    }

    /*
     * 2 saniye sonra kapat.
     */

    k_work_init_delayable(
        &startup_led_off_work,
        startup_led_off
    );

    k_work_init_delayable(
        &shutdown_led_work,
        shutdown_led_blink
    );

    k_work_schedule(
        &startup_led_off_work,
        K_MSEC(2000)
    );

    return 0;
}


SYS_INIT(
    startup_led_init,
    APPLICATION,
    0
);
