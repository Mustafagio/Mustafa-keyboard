#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/*
 * ZMK v0.3 nice!nano onboard LED
 *
 * nice_nano.dtsi:
 *
 * blue_led: led_0 {
 *     gpios = <&gpio0 15 GPIO_ACTIVE_HIGH>;
 * };
 *
 * Yani:
 * P0.15
 * HIGH = LED ON
 * LOW  = LED OFF
 */

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);


/*
 * 2 saniye sonunda LED'i kapatmak için
 * kullanılacak delayed work.
 */
static struct k_work_delayable startup_led_off_work;


/*
 * 2 saniye sonra çalışır ve LED'i kapatır.
 */
static void startup_led_off(struct k_work *work)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(&startup_led, 0);
}


/*
 * Sistem başlarken çalışır.
 */
static int startup_led_init(void)
{
    int ret;

    /*
     * LED GPIO'sunun hazır olup olmadığını kontrol et.
     */
    if (!gpio_is_ready_dt(&startup_led)) {
        return -ENODEV;
    }

    /*
     * P0.15'i çıkış olarak ayarla.
     * Başlangıçta LED kapalı.
     */
    ret = gpio_pin_configure_dt(
        &startup_led,
        GPIO_OUTPUT_INACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    /*
     * LED'i yak.
     */
    ret = gpio_pin_set_dt(&startup_led, 1);

    if (ret < 0) {
        return ret;
    }

    /*
     * Delayed work'ü hazırla.
     */
    k_work_init_delayable(
        &startup_led_off_work,
        startup_led_off
    );

    /*
     * 2000 ms = 2 saniye sonra LED'i kapat.
     */
    k_work_schedule(
        &startup_led_off_work,
        K_MSEC(2000)
    );

    return 0;
}


/*
 * Uygulama başlatılırken çalıştır.
 */
SYS_INIT(
    startup_led_init,
    APPLICATION,
    0
);
