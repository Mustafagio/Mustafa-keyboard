#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/* =========================================================
 * MUSTAFA KEYBOARD STARTUP LED
 *
 * Görevleri:
 *
 * 1. Açılışta LED 2 saniye yanar.
 * 2. Auto-Off öncesinde LED 2 saniye hızlı yanıp söner.
 * 3. Uyarı tamamlanınca auto_off.c callback çağrılır.
 *
 * Soft Off burada yapılmaz.
 * ========================================================= */

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(
        DT_NODELABEL(blue_led),
        gpios
    );

/* =========================================================
 * STARTUP LED
 * ========================================================= */

static struct k_work_delayable startup_led_off_work;

static void startup_led_off(
    struct k_work *work
)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(
        &startup_led,
        0
    );
}

/* =========================================================
 * SHUTDOWN WARNING
 * ========================================================= */

static struct k_work_delayable shutdown_led_work;

static int shutdown_blink_count = 0;

static void (*shutdown_complete_callback)(
    void
) = NULL;

/* =========================================================
 * LED BLINK
 * ========================================================= */

static void shutdown_led_blink(
    struct k_work *work
)
{
    ARG_UNUSED(work);

    /*
     * 20 toggle tamamlandı.
     */
    if (shutdown_blink_count >= 20) {

        void (*callback)(void) =
            shutdown_complete_callback;

        shutdown_complete_callback =
            NULL;

        gpio_pin_set_dt(
            &startup_led,
            0
        );

        if (callback != NULL) {
            callback();
        }

        return;
    }

    gpio_pin_toggle_dt(
        &startup_led
    );

    shutdown_blink_count++;

    k_work_schedule(
        &shutdown_led_work,
        K_MSEC(100)
    );
}

/* =========================================================
 * WARNING START
 * ========================================================= */

void mustafa_shutdown_warning_start(
    void (*callback)(void)
)
{
    if (!device_is_ready(
        startup_led.port
    )) {
        return;
    }

    k_work_cancel_delayable(
        &shutdown_led_work
    );

    shutdown_blink_count = 0;

    shutdown_complete_callback =
        callback;

    gpio_pin_set_dt(
        &startup_led,
        0
    );

    k_work_schedule(
        &shutdown_led_work,
        K_NO_WAIT
    );
}

/* =========================================================
 * WARNING CANCEL
 * ========================================================= */

void mustafa_shutdown_warning_cancel(void)
{
    k_work_cancel_delayable(
        &shutdown_led_work
    );

    shutdown_blink_count = 0;

    shutdown_complete_callback =
        NULL;

    if (device_is_ready(
        startup_led.port
    )) {

        gpio_pin_set_dt(
            &startup_led,
            0
        );
    }
}

/* =========================================================
 * INIT
 * ========================================================= */

static int startup_led_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(
        &startup_led
    )) {
        return -ENODEV;
    }

    ret =
        gpio_pin_configure_dt(
            &startup_led,
            GPIO_OUTPUT_INACTIVE
        );

    if (ret < 0) {
        return ret;
    }

    /*
     * Açılış LED'i ON
     */
    ret =
        gpio_pin_set_dt(
            &startup_led,
            1
        );

    if (ret < 0) {
        return ret;
    }

    k_work_init_delayable(
        &startup_led_off_work,
        startup_led_off
    );

    k_work_init_delayable(
        &shutdown_led_work,
        shutdown_led_blink
    );

    /*
     * 2 saniye sonra startup LED OFF.
     */
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
