/**
 ****************************************************************************************
 *
 * @file led.c
 *
 * @brief led driver for qn evb.
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup  LED
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "gpio.h"
#include "led.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief   LED initilization
 ****************************************************************************************
 */
void led_init()
{
    // gpio P0.5/P0.4/P0.3/P0.2/P0.1 are output to control led 1~5
    gpio_set_direction_field(LED1_PIN|LED2_PIN, (uint32_t)GPIO_OUTPUT);
    // all led are off
    led_set(1, LED_OFF);
    led_set(2, LED_OFF);
}

/**
 ****************************************************************************************
 * @brief   Set led on/off
 * @param[in]    idx    1~5 -> led 1~5
 * @param[in]    enable on/off
 * @description
 *  This function switchs on/off led individually.
 ****************************************************************************************
 */
void led_set(uint32_t idx, enum led_st enable)
{
    enum gpio_pin reg;
    
    switch(idx)
    {
        case 1:
            reg = LED1_PIN;
            break;
        case 2:
            reg = LED2_PIN;
            break;
        default:
            return;
    }

    gpio_write_pin(reg, (enum gpio_level)enable);
}

/**
 ****************************************************************************************
 * @brief   Get led status
 * @param[in]    idx    1~5 -> led 1~5
 * @return       led on/off
 * @description
 *  This function get led status individually.
 ****************************************************************************************
 */
enum led_st led_get(uint32_t idx)
{
    enum gpio_pin reg;
    
    switch(idx)
    {
        case 1:
            reg = LED1_PIN;
            break;
        case 2:
            reg = LED2_PIN;
            break;
        default:
            return LED_OFF;
    }

    return (enum led_st)gpio_read_pin(reg);
}

/// @} LED



