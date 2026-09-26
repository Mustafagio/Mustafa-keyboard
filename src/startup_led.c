#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/*
 * P0.15 onboard LED
 *
 * Devicetree:
 * startup_led: startup_led {
 *     gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;
 * };
 */

static const struct gpio_dt_spec startup_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(startup_led), gpios);


/*
 * LED'i kapatmak için kullanılacak delayed work.
 */
static struct k_work_delayable startup_led_off_work;


/*
 * 2 saniye sonunda LED'i kapat.
 */
static void startup_led_off(struct k_work *work)
{
    ARG_UNUSED(work);

    gpio_pin_set_dt(&startup_led, 0);
}


/*
 * Açılışta LED'i yak.
 *
 * LED:
 *   logical 1 = ON
 *   logical 0 = OFF
 *
 * Active LOW olduğu için gpio_pin_set_dt()
 * fiziksel seviyeyi otomatik olarak tersliyor.
 */
static int startup_led_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&startup_led)) {
        return -ENODEV;
    }

    /*
     * LED başlangıçta kapalı.
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
     * 2 saniye sonra kapat.
     */
    k_work_init_delayable(
        &startup_led_off_work,
        startup_led_off
    );

    k_work_schedule(
        &startup_led_off_work,
        K_MSEC(2000)
    );

    return 0;
}


/*
 * Uygulama başlarken çalışır.
 *
 * Soft Off'tan ESC ile uyanırken nRF52840
 * uygulamayı yeniden başlattığı için bu kod
 * tekrar çalışacak ve LED yine 2 saniye yanacaktır.
 */
SYS_INIT(
    startup_led_init,
    APPLICATION,
    0
);
