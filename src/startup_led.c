#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/* =========================================================
 * MUSTAFA KEYBOARD - STARTUP / SHUTDOWN LED
 *
 * Bu dosyanın görevi:
 * 1) Açılışta LED'i 2 saniye yakmak.
 * 2) Auto-Off başlamadan önce LED'i 2 saniye hızlı
 *    şekilde yanıp söndürmek.
 * 3) Uyarı tamamlandığında auto_off.c içindeki callback'i
 *    çağırmak.
 *
 * Soft Off işlemi BU DOSYADA YAPILMAZ.
 * Soft Off işlemini auto_off.c gerçekleştirir.
 * ========================================================= */

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

/* =========================================================
 * AÇILIŞ LED'İ
 * ========================================================= */

static struct k_work_delayable startup_led_off_work;

static void startup_led_off(struct k_work *work)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(&startup_led, 0);
}

/* =========================================================
 * AUTO-OFF UYARI LED'İ
 *
 * 2 saniye boyunca:
 * 100 ms ON / 100 ms OFF
 * Toplam 20 toggle
 * ========================================================= */

static struct k_work_delayable shutdown_led_work;

static int shutdown_blink_count = 0;

static void (*shutdown_complete_callback)(void) = NULL;

static void shutdown_led_blink(struct k_work *work)
{
    ARG_UNUSED(work);

    /*
     * 20 toggle tamamlandıktan sonra LED'i kapat ve
     * auto_off.c tarafından verilen callback'i çalıştır.
     */
    if (shutdown_blink_count >= 20) {
        void (*callback)(void) = shutdown_complete_callback;

        shutdown_complete_callback = NULL;

        gpio_pin_set_dt(&startup_led, 0);

        if (callback != NULL) {
            callback();
        }

        return;
    }

    gpio_pin_toggle_dt(&startup_led);

    shutdown_blink_count++;

    k_work_schedule(
        &shutdown_led_work,
        K_MSEC(100)
    );
}

/* =========================================================
 * AUTO-OFF UYARISINI BAŞLAT
 *
 * callback = 2 saniyelik LED uyarısı bittiğinde çağrılacak
 *           fonksiyon.
 * ========================================================= */

void mustafa_shutdown_warning_start(
    void (*callback)(void)
)
{
    if (!device_is_ready(startup_led.port)) {
        return;
    }

    k_work_cancel_delayable(&shutdown_led_work);

    shutdown_blink_count = 0;
    shutdown_complete_callback = callback;

    gpio_pin_set_dt(&startup_led, 0);

    k_work_schedule(
        &shutdown_led_work,
        K_NO_WAIT
    );
}

/* =========================================================
 * AUTO-OFF UYARISINI İPTAL ET
 *
 * Kullanıcı uyarı sırasında bir tuşa basarsa çağrılır.
 * ========================================================= */

void mustafa_shutdown_warning_cancel(void)
{
    k_work_cancel_delayable(&shutdown_led_work);

    shutdown_blink_count = 0;
    shutdown_complete_callback = NULL;

    if (device_is_ready(startup_led.port)) {
        gpio_pin_set_dt(&startup_led, 0);
    }
}

/* =========================================================
 * STARTUP LED INIT
 * ========================================================= */

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

    ret = gpio_pin_set_dt(
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

    /* Açılış LED'i 2 saniye yanar. */
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
