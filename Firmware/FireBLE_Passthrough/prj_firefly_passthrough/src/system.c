/**
 ****************************************************************************************
 *
 * @file system.c
 *
 * @brief User system setup and initial configuration source file.
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#include "system.h"
#include "uart.h"
#include "gpio.h"
#include "spi.h"
#include "timer.h"
#include "pwm.h"
#include "dma.h"
#include "serialflash.h"
#include "adc.h"
#include "analog.h"
#include "led.h"
#include "lib.h"
#include "usr_design.h"

/**
 ****************************************************************************************
 * @brief  GPIO functionn configuration.
 *
 * Set Pins as gpio pin or function pin
 *****************************************************************************************
 */
static void SystemIOCfg(void)
{
    // pin mux
    syscon_SetPMCR0(QN_SYSCON, P00_UART0_TXD_PIN_CTRL
                    | P01_GPIO_1_PIN_CTRL
                    | P02_GPIO_2_PIN_CTRL
                    | P03_GPIO_3_PIN_CTRL
                    | P04_GPIO_4_PIN_CTRL
                    | P05_GPIO_5_PIN_CTRL
#if		!(FB_SWD)
                    | P06_GPIO_6_PIN_CTRL
                    | P07_GPIO_7_PIN_CTRL
#else
                    | P06_SW_DAT_PIN_CTRL
                    | P07_SW_CLK_PIN_CTRL
#endif
#if (!QN_COM)
                    | P10_GPIO_8_PIN_CTRL
                    | P11_GPIO_9_PIN_CTRL
#else
                    | P10_UART1_RXD_PIN_CTRL
                    | P11_UART1_TXD_PIN_CTRL
#endif
                    | P12_GPIO_10_PIN_CTRL
                    | P13_GPIO_11_PIN_CTRL
                    | P14_GPIO_12_PIN_CTRL
                    | P15_GPIO_13_PIN_CTRL
                    | P16_GPIO_14_PIN_CTRL
                    | P17_UART0_RXD_PIN_CTRL
                   );
    syscon_SetPMCR1(QN_SYSCON, P20_GPIO_16_PIN_CTRL
                    | P21_GPIO_17_PIN_CTRL
                    | P22_GPIO_18_PIN_CTRL
                    | P23_GPIO_19_PIN_CTRL
                    | P24_GPIO_20_PIN_CTRL
                    | P25_GPIO_21_PIN_CTRL
                    | P26_GPIO_22_PIN_CTRL
                    | P27_GPIO_23_PIN_CTRL
                    | P30_GPIO_24_PIN_CTRL
                    | P31_GPIO_25_PIN_CTRL
#if (defined(CFG_HCI_SPI))
                    | P32_SPI0_DIN_PIN_CTRL        //P3.2 spi1 data in
                    | P33_SPI0_DAT_PIN_CTRL        //P3.3 spi1 data out
                    | P34_SPI0_CLK_PIN_CTRL        //P3.4 spi1 clk
                    | P35_SPI0_CS0_PIN_CTRL        //P3.5 spi1 cs
#else
                    | P32_GPIO_26_PIN_CTRL
                    | P33_GPIO_27_PIN_CTRL
                    | P34_GPIO_28_PIN_CTRL
                    | P35_GPIO_29_PIN_CTRL
#endif
                    | P36_GPIO_30_PIN_CTRL
                   );

    // pin select
    syscon_SetPMCR2(QN_SYSCON, 0);

    // driver ability
    syscon_SetPDCR(QN_SYSCON, 0x0); // 0 : low driver, 1 : high driver

    // pin pull ( 00 : High-Z,  01 : Pull-down,  10 : Pull-up,  11 : Reserved )
#if		(FB_SWD)
    syscon_SetPPCR0(QN_SYSCON, 0xAAAA5AAA);
#else
    syscon_SetPPCR0(QN_SYSCON, 0xAAAAAAAA);
#endif
    syscon_SetPPCR1(QN_SYSCON, 0xAAAAAAAA);
}

// add pto_test for FS_QN9021,test all GPIO is useable.
#if defined(CFG_ALL_GPIO_TEST)

//Test Mode : test all gpio normal working 
void all_gpio_test(void);

//all test pin in QN9021
#define	TEST_PIN	(GPIO_P03 | GPIO_P06 | GPIO_P07 | GPIO_P10 | GPIO_P11 | GPIO_P13 | GPIO_P23 | GPIO_P24 | GPIO_P26 | GPIO_P27 | GPIO_P30 | GPIO_P31)
//LED blink delay time
#define DELAY_TIME		500000

#endif



/**
 ****************************************************************************************
 * @brief  Setup the microcontroller system.
 *
 *  Initialize the system clock and pins.
 *****************************************************************************************
 */
void SystemInit(void)
{
    /*
     **************************
     * Sub module clock setting
     **************************
     */

    // Disable all peripheral clock, will be enabled in the driver initilization.
    timer_clock_off(QN_TIMER0);
    timer_clock_off(QN_TIMER1);
    timer_clock_off(QN_TIMER2);
    timer_clock_off(QN_TIMER3);
    uart_clock_off(QN_UART0);
    uart_clock_off(QN_UART1);
    spi_clock_off(QN_SPI0);
    usart_reset((uint32_t) QN_SPI1);
    spi_clock_off(QN_SPI1);
    flash_clock_off();
    gpio_clock_off();
    adc_clock_off();
    dma_clock_off();
    pwm_clock_off();

    // Configure sytem clock.
    syscon_set_sysclk_src(CLK_XTAL, __XTAL);
    syscon_set_ahb_clk(__AHB_CLK);
    syscon_set_ble_clk(__BLE_CLK);
    syscon_set_apb_clk(__APB_CLK);
    syscon_set_timer_clk(__TIMER_CLK);
    syscon_set_usart_clk((uint32_t)QN_UART0, __USART_CLK);
    syscon_set_usart_clk((uint32_t)QN_UART1, __USART_CLK);
    clk32k_enable(__32K_TYPE);


// if pull down GPIO_P12 when power on,it will enter the test mode
#if defined(CFG_ALL_GPIO_TEST)
    //set GPIO_P12 direction to GPIO_INPUT
    syscon_SetPMCR0(QN_SYSCON,P12_GPIO_10_PIN_CTRL);
    gpio_init(gpio_interrupt_callback);
    gpio_pull_set(GPIO_P12, GPIO_PULL_UP);
    gpio_set_direction_field(GPIO_P12, (uint32_t)GPIO_INPUT);

    //check if it's pull down
    if (gpio_read_pin(GPIO_P12) == GPIO_LOW)
    {
        //set a flag to enter test mode until device reset
        app_env.test_flag = TRUE;
    }
    else
        app_env.test_flag = FALSE;
#endif
    /*
     **************************
     * IO configuration
     **************************
     */

    SystemIOCfg();

    /*
     **************************
     * Peripheral setting
     **************************
     */

    // GPIO initialization for led, button & test control pin.
    gpio_init(gpio_interrupt_callback);

    // LED
    led_init();

    // Test controll pin is input to check work mode
#if (defined(QN_TEST_CTRL_PIN))
    gpio_pull_set(QN_TEST_CTRL_PIN, GPIO_PULL_UP);
    gpio_set_direction_field(QN_TEST_CTRL_PIN, (uint32_t)GPIO_INPUT);

#if (defined(CFG_HCI_UART))
    // Initialize HCI UART port
    uart_init(QN_HCI_PORT, USARTx_CLK(0), UART_9600);
    uart_tx_enable(QN_HCI_PORT, MASK_ENABLE);
    uart_rx_enable(QN_HCI_PORT, MASK_ENABLE);
#elif (defined(CFG_HCI_SPI))
    // Initialize HCI SPI port
    spi_init(QN_HCI_PORT, SPI_BITRATE(1000000), SPI_8BIT, SPI_SLAVE_MOD);
    gpio_set_direction_field(CFG_HCI_SPI_WR_CTRL_PIN, (uint32_t)GPIO_OUTPUT);
    gpio_write_pin(CFG_HCI_SPI_WR_CTRL_PIN, GPIO_HIGH);
#endif
#endif

#if defined(QN_COM_UART)
    // Initialize User UART port
    uart_init(QN_COM_UART, USARTx_CLK(0), UART_9600);
    uart_tx_enable(QN_COM_UART, MASK_ENABLE);
    uart_rx_enable(QN_COM_UART, MASK_ENABLE);
#endif

#if (QN_DBG_PRINT)
    // Initialize Debug UART port
    uart_init(QN_DEBUG_UART, USARTx_CLK(0), UART_9600);
    uart_tx_enable(QN_DEBUG_UART, MASK_ENABLE);
    uart_rx_enable(QN_DEBUG_UART, MASK_ENABLE);
#endif

// if enter test mode flag had been seted,enter a loop to test all GPIO.
#if		(defined(CFG_ALL_GPIO_TEST))
    if (app_env.test_flag == TRUE)
		{
        //get a warnning to user
        //QPRINTF("\r\n@@@You pull down the GPIO_level of GPIO_P12 when power on,so it will enter the test mode!");
        while(1)
        {
            all_gpio_test();
        }
		}
#endif
}

//function  declare of GPIO_TEST
#if defined(CFG_ALL_GPIO_TEST)
void delay_5()
{
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);
    delay(10000);

}

/*
****************************************************************************************
* @brief Test gpio work status
*
*****************************************************************************************/
void all_gpio_test(void)
{
    // pin mux
    syscon_SetPMCR0(QN_SYSCON, P00_UART0_TXD_PIN_CTRL
                    | P01_GPIO_1_PIN_CTRL
                    | P02_GPIO_2_PIN_CTRL
                    | P03_GPIO_3_PIN_CTRL
                    | P04_GPIO_4_PIN_CTRL
                    | P05_GPIO_5_PIN_CTRL
                    | P06_GPIO_6_PIN_CTRL
                    | P07_GPIO_7_PIN_CTRL

                    | P10_GPIO_8_PIN_CTRL
                    | P11_GPIO_9_PIN_CTRL
                    | P12_GPIO_10_PIN_CTRL
                    | P13_GPIO_11_PIN_CTRL
                    | P14_GPIO_12_PIN_CTRL
                    | P15_GPIO_13_PIN_CTRL
                    | P16_GPIO_14_PIN_CTRL
                    | P17_GPIO_15_PIN_CTRL
                   );
    syscon_SetPMCR1(QN_SYSCON, P20_GPIO_16_PIN_CTRL
                    | P21_GPIO_17_PIN_CTRL
                    | P22_GPIO_18_PIN_CTRL
                    | P23_GPIO_19_PIN_CTRL
                    | P24_GPIO_20_PIN_CTRL
                    | P25_GPIO_21_PIN_CTRL
                    | P26_GPIO_22_PIN_CTRL
                    | P27_GPIO_23_PIN_CTRL

                    | P30_GPIO_24_PIN_CTRL
                    | P31_GPIO_25_PIN_CTRL
                    | P32_GPIO_26_PIN_CTRL
                    | P33_GPIO_27_PIN_CTRL
                    | P34_GPIO_28_PIN_CTRL
                    | P35_GPIO_29_PIN_CTRL
                    | P36_GPIO_30_PIN_CTRL
                   );


    // driver ability
    syscon_SetPDCR(QN_SYSCON, 0x0); // 0 : low driver, 1 : high driver

    // pin pull ( 00 : High-Z,  01 : Pull-down,  10 : Pull-up,  11 : Reserved )
    syscon_SetPPCR0(QN_SYSCON, 0xAAAAAAAA);
    syscon_SetPPCR1(QN_SYSCON, 0xAAAAAAAA);

    gpio_init(gpio_interrupt_callback);

    gpio_set_direction_field(TEST_PIN,(uint32_t)GPIO_OUTPUT);
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_LOW);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_HIGH);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_LOW);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_HIGH);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_LOW);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_HIGH);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_LOW);
    delay_5();
    gpio_write_pin_field(TEST_PIN,(uint32_t)GPIO_HIGH);
    delay_5();
}
#endif

/// @} SYSTEM_CONTROLLER

