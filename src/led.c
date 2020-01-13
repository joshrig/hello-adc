#include <atmel_start.h>

#include "led.h"



typedef struct
{
    led_mode_t mode;
    uint8_t    val;

#define NLEDS 5
    uint32_t leds[NLEDS];

} led_state_t;



static led_state_t led_state = {
    .val = 0, 
    .leds = {
        GPIO(GPIO_PORTC, 18),
        GPIO(GPIO_PORTB, 14),
        GPIO(GPIO_PORTC, 10),
        GPIO(GPIO_PORTD, 11),
        GPIO(GPIO_PORTD, 10),
    }
};




void led_init(led_mode_t mode)
{
    led_state_t *me = &led_state;


    me->mode = mode;
    switch (me->mode)
    {
    case kLEDCountMode:
        // initialize the LEDS
        for (int i = 0; i < NLEDS; i++)
        {
            gpio_set_pin_level    (me->leds[i], true);
            gpio_set_pin_direction(me->leds[i], GPIO_DIRECTION_OUT);
            gpio_set_pin_function (me->leds[i], GPIO_PIN_FUNCTION_OFF);
        }
        break;
    case kLEDBlinkMode:
        gpio_set_pin_level    (me->leds[0], true);
        gpio_set_pin_direction(me->leds[0], GPIO_DIRECTION_OUT);
        gpio_set_pin_function (me->leds[0], GPIO_PIN_FUNCTION_OFF);
        break;
    }
}


void led_update(void)
{
    led_state_t *me = &led_state;


    switch (me->mode)
    {
    case kLEDCountMode:
        for (int i = 0; i < NLEDS; i++)
            gpio_set_pin_level(me->leds[i], !((me->val >> i) & 0x01));

        me->val++;
        break;

    case kLEDBlinkMode:
        me->val ^= 1;
        gpio_set_pin_level(me->leds[0], me->val);
        break;
    }
        
}


const char *led_get_mode_string(void)
{
    led_state_t *me = &led_state;


    switch (me->mode)
    {
    case kLEDCountMode:
        return "kLEDCountMode";
    case kLEDBlinkMode:
        return "kLEDBlinkMode";
    }

    return "UNKNOWN_MODE";
}
