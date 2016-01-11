/****************************************************************************
 *   $Id:: pwm_example.c                                                   $
 *   Project: NXP QN9020 PWM example
 *
 *   Description:
 *     This file contains PWM driver usage.
 *
****************************************************************************/

#include "pwm.h"
#include "gpio.h"
#include "system.h"



/****************************************************************************
*               PWM driver code example
* The PWM perid and pulse counter only has 8 bits, so the precision of duty cycle is 1/256.
*
* Note: PWM output is undef when disabled, the user can modify and call pwm_io_dis_config() to set 
* the PWM output to a fixed state.
*
****************************************************************************/


void pwm_io_config(void)
{
    uint32_t reg;
    uint32_t reg_mask;
    
    /**
     * PMCR0 register pin configure
     */
    reg = P07_SW_CLK_PIN_CTRL | P06_SW_DAT_PIN_CTRL;
    reg_mask = P07_MASK_PIN_CTRL | P06_MASK_PIN_CTRL;
    syscon_SetPMCR0WithMask(QN_SYSCON, reg_mask, reg);
    
    /**
     * PMCR1 register pin configure
     */
    reg = P27_PWM0_PIN_CTRL | P26_PWM1_PIN_CTRL;
    reg_mask = P27_MASK_PIN_CTRL | P26_MASK_PIN_CTRL;
    syscon_SetPMCR1WithMask(QN_SYSCON, reg_mask, reg);
    
    // pin pull ( 00 : High-Z,  01 : Pull-down,  10 : Pull-up,  11 : Reserved )
    syscon_SetPPCR0(QN_SYSCON, 0xAAAA5AAA);
    syscon_SetPPCR1(QN_SYSCON, 0x2AAAAAAA);
}

void pwm_io_dis_config(void)
{

    syscon_SetPMCR1(QN_SYSCON, P27_GPIO_23_PIN_CTRL         //P2.7 GPIO
                             | P26_GPIO_22_PIN_CTRL         //P2.6 GPIO
                             );
    gpio_set_direction_field(GPIO_P27|GPIO_P26, GPIO_INPUT);
}


int main (void)
{
    SystemInit();

    pwm_init(PWM_CH0);
    pwm_io_config();
    //P2.7 will output pwm wave with period for 1000us and pulse for 400us
    pwm_config(PWM_CH0, PWM_PSCAL_DIV, PWM_COUNT_US(1000, PWM_PSCAL_DIV), PWM_COUNT_US(500, PWM_PSCAL_DIV));
    pwm_enable(PWM_CH0, MASK_ENABLE);
    pwm_io_dis_config();

    pwm_init(PWM_CH1);
    pwm_io_config();
    //P2.6 will output pwm wave with period for 1000us and pulse for 500us
    pwm_config(PWM_CH1, 119, PWM_COUNT_US(1000, 119), PWM_COUNT_US(500, 119));
    pwm_enable(PWM_CH1, MASK_ENABLE);

    while (1)                                /* Loop forever */
    {
			int i;
			for(i=0;i<=1000;i++)
			{
				pwm_config(PWM_CH1, 119, PWM_COUNT_US(1000-i, 119), PWM_COUNT_US(500, 119));
				if(i%2)
					pwm_config(PWM_CH0, PWM_PSCAL_DIV, PWM_COUNT_US(1000-i, PWM_PSCAL_DIV), PWM_COUNT_US(500, PWM_PSCAL_DIV));
				else
					pwm_config(PWM_CH0, PWM_PSCAL_DIV, PWM_COUNT_US(i, PWM_PSCAL_DIV), PWM_COUNT_US(500, PWM_PSCAL_DIV));
				delay(2000);
			}
    }
}
