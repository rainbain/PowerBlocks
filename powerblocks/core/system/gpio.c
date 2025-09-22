#/**
 * @file gpio.c
 * @brief Wii GPIO
 *
 * Control the GPIO controller on the Wii
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "gpio.h"

#define GPIO_OUT                 (*(volatile uint32_t*)0xCD8000C0)
#define GPIO_DIR                 (*(volatile uint32_t*)0xCD8000C4)
#define GPIO_IN                  (*(volatile uint32_t*)0xCD8000C8)
#define GPIO_INTLVL              (*(volatile uint32_t*)0xCD8000CC)
#define GPIO_INTFLAG             (*(volatile uint32_t*)0xCD8000D0)
#define GPIO_INTMASK             (*(volatile uint32_t*)0xCD8000D4)
#define GPIO_STRAPS              (*(volatile uint32_t*)0xCD8000D8)
#define GPIO_ENABLE              (*(volatile uint32_t*)0xCD8000DC)

void gpio_write(uint32_t bit, bool set) {
    if(set) {
        GPIO_OUT |= bit;
    } else {
        GPIO_OUT &= ~bit;
    }
}

bool gpio_read(uint32_t bit) {
    return (GPIO_IN & bit) > 0;
}

void gpio_set_direction(uint32_t bit, bool direction) {
    if(direction) {
        GPIO_DIR |= bit;
    } else {
        GPIO_DIR &= ~bit;
    }
}