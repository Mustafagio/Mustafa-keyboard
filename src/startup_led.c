#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <zmk/pm.h>

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

/* ---------------------------------------------------------
 * AÇILIŞ LED'İ
 *
 * Kart açıldığında LED 2 saniye sürekli yanar.
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
 * 2 saniye boyunca 100 ms ON / 100 ms OFF.
 * Animasyon tamamlanınca Soft Off yapılır.
 * --------------------------------------------------------- */

static struct k_work_delayable shutdown_led_work;
static int shutdown_blink_count;
static bool shutdown_sequence_active;

static void shutdown_led_blink(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!shutdown_sequence_active) {
        gpio_pin_set_dt(&startup_led, 0);
        return;
    }

    if (shutdown_blink_count >= 20) {
        gpio_pin_set_dt(&startup_led, 0);
        shutdown_sequence_active = false;

        /* LED uyarısı tamamlandı; şimdi gerçek Soft Off. */
        zmk_pm_soft_off();
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
 * AUTO-OFF MODÜLÜ TARAFINDAN ÇAĞRILIR
 * --------------------------------------------------------- */

void mustafa_start_shutdown_sequence(void)
{
    if (!device_is_ready(startup_led.port)) {
        /* LED kullanılamıyorsa Soft Off'u bekletme. */
        zmk_pm_soft_off();
        return;
    }

    k_work_cancel_delayable(&startup_led_off_work);
    k_work_cancel_delayable(&shutdown_led_work);

    shutdown_blink_count = 0;
    shutdown_sequence_active = true;

    gpio_pin_set_dt(&startup_led, 0);

    k_work_schedule(
        &shutdown_led_work,
        K_NO_WAIT
    );
}

/*
 * Uyarı animasyonu sırasında tuşa basılırsa Auto-Off tarafı bunu
 * çağırarak kapanış animasyonunu iptal eder.
 */
void mustafa_cancel_shutdown_sequence(void)
{
    shutdown_sequence_active = false;
    shutdown_blink_count = 0;

    k_work_cancel_delayable(&shutdown_led_work);
    gpio_pin_set_dt(&startup_led, 0);
}

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

    k_work_init_delayable(
        &startup_led_off_work,
        startup_led_off
    );

    k_work_init_delayable(
        &shutdown_led_work,
        shutdown_led_blink
    );

    /* Açılışta LED 2 saniye sürekli yanar. */
    ret = gpio_pin_set_dt(
        &startup_led,
        1
    );

    if (ret < 0) {
        return ret;
    }

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
