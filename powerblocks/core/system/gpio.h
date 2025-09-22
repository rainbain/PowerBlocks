#/**
 * @file gpio.h
 * @brief Wii GPIO
 *
 * Control the GPIO controller on the Wii
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// There are more bits, these are just the one that the
// PowerPC side has access to.
#define GPIO_SLOT_LED   0x000020 // Blue LEDs around the disk reader
#define GPIO_SLOT_IN    0x000080 // High when tilt switch in disk reader is presses.
#define GPIO_SENSOR_BAR 0x000100 // Turns on the sensor bar.
#define GPIO_EJECT      0x000200 // Pulse to eject disk
#define GPIO_AVE_SCL    0x004000 // AVE Encoder I2C Clock
#define GPIO_AVE_SDA    0x008000 // AVE Encoder I2C SDA

/**
 * @brief Writes a GPIO output.
 *
 * Writes the bit to the GPIO controller.
 * 
 * @param bit Bit to set/clear
 */
extern void gpio_write(uint32_t bit, bool set);

/**
 * @brief Reads a GPIO Input
 * 
 * Reads the value of a GPIO Input
 * 
 * @param bit Bit to read value of
 */
extern bool gpio_read(uint32_t bit);

/**
 * @brief Sets the direction of the GPIO.
 * 
 * Sets the direction of a bit.
 * 
 * This is set to the expected value before the game runs.
 * 
 * @param bit Bit to set
 * @param Direction False = Input, True = Output
 */
extern void gpio_set_direction(uint32_t bit, bool direction);


/// TODO:
/// Implement GPIO Interrupts