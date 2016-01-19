/**
 ****************************************************************************************
 *
 * @file at_command.c
 *
 * @brief 
 *
 * Copyright (C) Firefly 2015-2016
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */
 
 /**
 ****************************************************************************************
 * @addtogroup  USR
 * @{
 ****************************************************************************************
 */
 
 /*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "at_command.h"
#include "app_env.h"
#include "uart.h"
#include "lib.h"
#include "sleep.h"
#include "nvds.h"
#include "pwm.h"
#include "i2c.h"
#include "usr_spi.h"
#include	"ke_mem.h"

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */
#define LED_ON_DUR_ADV_FAST        2
#define LED_OFF_DUR_ADV_FAST       (uint16_t)((GAP_ADV_FAST_INTV2*0.625)/10)
#define LED_ON_DUR_ADV_SLOW        2
#define LED_OFF_DUR_ADV_SLOW       (uint16_t)((GAP_ADV_SLOW_INTV*0.625)/10)
#define LED_ON_DUR_CON          0xffff
#define LED_OFF_DUR_CON                   0
#define LED_ON_DUR_IDLE                   0
#define LED_OFF_DUR_IDLE                  0xffff

#define SPI_RX_MAX									40


/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern uint8_t baudrate;
uint8_t i2c_buff[4];
extern bool i2c_is_finish(void);
struct spi_data_tag
{
		uint8_t rx_len;
		uint8_t tx_len;
		uint8_t rx_data[SPI_RX_MAX];
		uint8_t tx_data[SPI_RX_MAX];
}spi_data;


uint32_t at_HexToString(uint8_t *str,uint8_t *hex,uint32_t hex_len);

/*
****************************************************************************************
* @brief             set_baudrate
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @return            None
* @description       ����ģ�鲨���ʣ���ʽ��AT+BAUDx,xȡֵΪ0~16�AAT+BAUD� ��AT+BAUD��ʾ��ѯ��ǰ������
*										UART_1200     = 0,     //!< Set baud rate to 1200 when UART clock is 8MHz //
*    								UART_2400     = 1,     //!< Set baud rate to 2400 when UART clock is 8MHz //
*    								UART_4800     = 2,     //!< Set baud rate to 4800 when UART clock is 8MHz //
*    								UART_9600     = 3,     //!< Set baud rate to 9600 when UART clock is 8MHz //
*    								UART_14400    = 4,     //!< Set baud rate to 14400 when UART clock is 8MHz //
*    								UART_19200    = 5,     //!< Set baud rate to 19200 when UART clock is 8MHz //
*    								UART_28800    = 6,     //!< Set baud rate to 28800 when UART clock is 8MHz //
*    								UART_38400    = 7,     //!< Set baud rate to 38400 when UART clock is 8MHz //
*    								UART_57600    = 8,     //!< Set baud rate to 57600 when UART clock is 8MHz //
*    								UART_64000    = 9,     //!< Set baud rate to 64000 when UART clock is 8MHz //
*    								UART_76800    = 10,    //!< Set baud rate to 76800 when UART clock is 8MHz //
*    								UART_115200   = 11,    //!< Set baud rate to 115200 when UART clock is 8MHz //
*    								UART_128000   = 12,    //!< Set baud rate to 128000 when UART clock is 8MHz //
*    								UART_230400   = 13,    //!< Set baud rate to 230400 when UART clock is 8MHz //
*    								UART_345600   = 14,    //!< Set baud rate to 345600 when UART clock is 8MHz //
*    								UART_460800   = 15,    //!< Set baud rate to 460800 when UART clock is 8MHz //
*    								UART_500000   = 16,    //!< Set baud rate to 500000 when UART clock is 8MHz //
*****************************************************************************************/
int set_baudrate( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	const int8_t *pcom;
	uint32_t pxParameterStringLength;
	int len;
	
	//the baudrate inquire with the follow byte is "?" or "\0"
	if(pcCommandString[commpare_length+1] == '?' || pcCommandString[commpare_length+1] == '\0')
	{
		len = commpare_length+1;
		memcpy(pcWriteBuffer, pcCommandString, len);
		len += sprintf((char *)pcWriteBuffer + len,":%d\r\nOK\r\n",baudrate);	
	}
	//set baudrate  with the follow byte is "="
	else if(pcCommandString[commpare_length+1] == '=')
	{
		// get parameter and fill into *pcom
		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
		if(pxParameterStringLength != 0)
		{
			baudrate = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
			if(baudrate > 16)
			{
				baudrate = 3;
			}		
			ke_timer_set(APP_COM_AT_BAUDRATE_CHANGE, TASK_APP, 1);	
			len = sprintf((char *)pcWriteBuffer,"OK\r\n");	
		}
		else
		{
			len = sprintf((char *)pcWriteBuffer,"ERR\r\n");
		}
	}
	else
	{
		len = sprintf((char *)pcWriteBuffer,"ERR\r\n");		
	}
	return len;
}

/*
****************************************************************************************
* @brief           	 get_version
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @return            None
* @description       ��ȡ�汾�ţ������ʽ��AT+VERSION OR AT+VERSION?
*****************************************************************************************/
int get_version( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	int len;
	len = commpare_length+1;
	memcpy(pcWriteBuffer, pcCommandString, len);
	len += sprintf((char *)pcWriteBuffer + len,":%s %s\r\nOK\r\n",SOFTWARE_VERSION,RELEASE_DATE);
	return len;
}

/*
****************************************************************************************
* @brief             get_name
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @return            None
* @description       ��ȡ�豸���������ʽ��AT+NAME OR AT+NAME?
*****************************************************************************************/
int get_name( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
		nvds_tag_len_t name_length = 31 - 5; 
		int len;
		len = commpare_length+1;
		memcpy(pcWriteBuffer, pcCommandString, len);
		pcWriteBuffer[len++] = ':';

    if (NVDS_OK != nvds_get(NVDS_TAG_DEVICE_NAME, &name_length, pcWriteBuffer+len))
    {
        // NVDS is empty, use default name
        name_length = strlen(QN_LOCAL_NAME);
        strcpy((char *)pcWriteBuffer+len, QN_LOCAL_NAME);			
    }
    else
    {
        name_length--; // Discard '\0'
    }
		len+=name_length;
		len+=sprintf((char *)pcWriteBuffer+len,"\r\nOK\r\n");
	return len;
}

/*
****************************************************************************************
* @brief             ble_discon
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @return            None
* @description       �Ͽ���ǰ���ӣ������ʽ��AT+DISCON
*****************************************************************************************/
int ble_discon( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	app_gap_discon_req(0);
	return sprintf((char*)pcWriteBuffer,"OK\r\n");
}

/*
****************************************************************************************
* @brief             ble_adv
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @return            None
* @description       �������߹رչ㲥�������ʽ��AT+ADV=0  �����㲥    AT+ADV=1   �رչ㲥
*****************************************************************************************/
int ble_adv( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	const int8_t *pcom;
	int32_t arg1;
	uint32_t pxParameterStringLength;

	pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 1, 0, &pxParameterStringLength);
	arg1 = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
	if(arg1 == 0)
	{
		usr_led1_set(LED_ON_DUR_IDLE,LED_OFF_DUR_IDLE);
		app_gap_adv_stop_req();
	}
	else if(arg1 == 1)
	{
				usr_led1_set(LED_ON_DUR_ADV_FAST,LED_ON_DUR_ADV_FAST);
				app_gap_adv_start_req(GAP_GEN_DISCOVERABLE|GAP_UND_CONNECTABLE,
						app_env.adv_data, app_set_adv_data(GAP_GEN_DISCOVERABLE),
						app_env.scanrsp_data, app_set_scan_rsp_data(app_get_local_service_flag()),
						GAP_ADV_FAST_INTV1, GAP_ADV_FAST_INTV2);
				ke_timer_set(APP_ADV_INTV_UPDATE_TIMER,TASK_APP,30);
	}
	else
	{
		return sprintf((char*)pcWriteBuffer,"ERR\r\n");
	}
	return sprintf((char*)pcWriteBuffer,"OK\r\n");
}

uint8_t pwm_output(enum PWM_CH pwmch,uint32_t periodcount, uint32_t pulsecount)
{
				uint16_t pscal = (8 * periodcount) / 255;
				if (pscal >= 0x3FF)
					return 0;
//				if (pscal < 0x200)
//					pscal = pscal * 2;
//				else
//					pscal = pscal + (0x3FF - pscal) / 2;
				if ((periodcount >= 1000) && (pulsecount >= 1000))
					return pwm_config(pwmch,pscal,PWM_COUNT_MS((periodcount/1000),pscal),PWM_COUNT_MS((pulsecount/1000),pscal));
				else
					if ((periodcount >= 1000) && (pulsecount < 1000))
						return pwm_config(pwmch,pscal,PWM_COUNT_MS((periodcount/1000),pscal),PWM_COUNT_US(pulsecount,pscal));
					else
						if ((periodcount < 1000) && (pulsecount < 1000))
							return pwm_config(pwmch,pscal,PWM_COUNT_US(periodcount,pscal),PWM_COUNT_US(pulsecount,pscal));
						else
							return 0;

}

void pwm_io_config(enum PWM_CH ch)
{
	if(ch == PWM_CH0)
	{
    syscon_SetPMCR1WithMask(QN_SYSCON,P27_MASK_PIN_CTRL, P27_PWM0_PIN_CTRL);//P2.7 pwm0
		gpio_pull_set(GPIO_P27, GPIO_PULL_UP);
	}
	else if(ch == PWM_CH1)
	{
		syscon_SetPMCR1WithMask(QN_SYSCON,P26_MASK_PIN_CTRL, P26_PWM1_PIN_CTRL);//P2.6 pwm1
		gpio_pull_set(GPIO_P26, GPIO_PULL_UP);
	}
}

/*
****************************************************************************************
* @brief             ble_pwm
* @param[in]         pcCommandString            ����Ͳ�����ŵ�ַ
* @param[in]         pcWriteBuffer              д����������������͵�����
* @param[in]         commpare_length            ������ռ����
* @response          None
* @return
* @description       ���û��߹ر�PWM�������ʽ��AT_PWM=
*****************************************************************************************/
int ble_pwm( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	//ȡ����ָ��ͳ�������
	const int8_t *pcom;
	uint32_t pxParameterStringLength;

	//PWM����
	enum PWM_CH ch;
	//uint16_t pscal;
	uint16_t periodcount;
	uint16_t pulsecount;
	
  uint8_t parm_num = at_get_parameters_numbers((const uint8_t*)pcCommandString + commpare_length + 2);

	//���������Я������
	if(pcCommandString[commpare_length+1] == '=')
	{
		//���Я��3��������������ȷ����������ֱ�Ϊͨ�������ڡ�ռ�ձȡ�
		if(parm_num == 3)
		{
			// pwm_ch?
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
			if(pxParameterStringLength == 1 )
			{ 
				if(pcom[0] == '0')
				{
					ch = PWM_CH0;
				}
				else if(pcom[0] == '1')
				{
					ch = PWM_CH1;
				}else
				{
					goto pram_err;
				}
			}
			else
			{
				goto pram_err;
			}	
			
//			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//			if(pxParameterStringLength > 0)
//			{
//				pscal = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
//				if(pscal > 0x3FF)
//				{
//					goto pram_err;
//				}
//			}
//			else
//			{
//				goto pram_err;
//			}

			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
			if(pxParameterStringLength > 0)
			{
				periodcount = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
				if(periodcount > 30000)
				{
					goto pram_err;
				}
			}
			else
			{
				goto pram_err;
			}			
			
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
			if(pxParameterStringLength > 0)
			{
				pulsecount = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
				if(pulsecount > 1000)
				{
					goto pram_err;
				}
			}
			else
			{
				goto pram_err;
			}

			//Parameters without error
			pwm_init(ch);
			pwm_io_config(ch);	
			if (pwm_output(ch,periodcount,periodcount*pulsecount/1000) == 0)
				goto output_err;
			pwm_enable(ch, MASK_ENABLE);			
	
		}
		//���ֻ��һ���������Ǿʹ���ָʾ�ر���һ��ͨ��
		else if(parm_num == 1)
		{
			if(pcCommandString[commpare_length + 2] == '0')
			{
				pwm_enable(PWM_CH0, MASK_DISABLE);
			}
			else if((pcCommandString[commpare_length + 2] == '1'))
			{
				pwm_enable(PWM_CH1, MASK_DISABLE);
			}
			else
			{
				goto pram_err;
			}
		}
		else
		{
			goto pram_err;
		}
	}
	else
	{
		goto pram_err;
	}
	
	return sprintf((char*)pcWriteBuffer,"OK\r\n");
	
pram_err:	
	return sprintf((char*)pcWriteBuffer,"PRAM ERR\r\n");
output_err:
	return sprintf((char*)pcWriteBuffer,"OUTPUT ERR\r\n");	
}

#define I2C_READ_MAX_LEN		20
static uint8_t i2c_mode = 0;
int ble_i2c( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	const int8_t *pcom;
	uint32_t pxParameterStringLength;
	
	uint8_t len;
	uint8_t i2c_addr;
//	uint8_t i2c_reg;
//	uint8_t i2c_data;

  uint8_t parm_num = at_get_parameters_numbers((const uint8_t*)pcCommandString + commpare_length + 2);

	if(pcCommandString[commpare_length+1] == '=')
	{
		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
		if(pxParameterStringLength == 1)
		{ 
			syscon_SetPMCR1WithMask(QN_SYSCON,P23_MASK_PIN_CTRL | P24_MASK_PIN_CTRL,P23_I2C_SDA_PIN_CTRL | P24_I2C_SCL_PIN_CTRL);	
			i2c_init(I2C_SCL_RATIO(100000), i2c_buff, 4);
			
			//I2C mode switch and R/W
			if(pcom[0] == 'M')
			{
				//i2c_mode
				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
				if(pxParameterStringLength > 0)
				{
					i2c_mode = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
				}
				else
				{
					goto pram_err;
				}
			}
			else if(pcom[0] == 'W')
			{
				//i2c_addr
				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
				if(pxParameterStringLength > 0)
				{
					i2c_addr = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
				}
				else
				{
					goto pram_err;
				}

				if (i2c_mode ==0)
				{
						//i2c_reg
						uint8_t i2c_reg;
						pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
						if(pxParameterStringLength > 0)
						{
							i2c_reg = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
						}
						else
						{
							goto pram_err;
						}
						if (parm_num == 4)
						{
							//i2c_data
							uint8_t i2c_data;
							pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 3, &pxParameterStringLength);
							if(pxParameterStringLength > 0)
							{
								i2c_data = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
							}
							else
							{
								goto pram_err;
							}

							if (I2C_BYTE_WRITE(i2c_addr, i2c_reg, i2c_data) < 0)
								goto write_err;
						}
						else if (parm_num > 4)
						{
							//i2c_len
							uint8_t i2c_len;
							pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 3, &pxParameterStringLength);
							if(pxParameterStringLength > 0)
							{
								i2c_len = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
							}
							else
							{
								goto pram_err;
							}
							if (parm_num == (4 + i2c_len))
							{
									uint8_t i2c_data[i2c_len];
									for (uint8_t i = 0;i<i2c_len;i++)
									{
										pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2,4+i, &pxParameterStringLength);
										if(pxParameterStringLength > 0)
										{
											i2c_data[i] = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
										}
										else
										{
											goto pram_err;
										}
									}
									
									if (I2C_nBYTE_WRITE(i2c_addr, i2c_reg,&i2c_data[0],i2c_len) < 0)
										goto write_err;
							}	
						}

							
						if(!i2c_is_finish())
						{
							goto pram_err;
						}
				}
				else if (i2c_mode == 1)
				{
					if (parm_num == 3)
					{
						//i2c_data
						uint8_t i2c_data;
						pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
						if(pxParameterStringLength > 0)
						{
							i2c_data = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
						}
						else
						{
							goto pram_err;
						}
						
						I2C_CMD_WRITE(i2c_addr,&i2c_data,1);
					}
					if  (parm_num > 3)
					{
							//i2c_len
							uint8_t i2c_len;
							pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 3, &pxParameterStringLength);
							if(pxParameterStringLength > 0)
							{
								i2c_len = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
							}
							else
							{
								goto pram_err;
							}
							if (parm_num == (i2c_len + 3))
							{
									uint8_t i2c_data[i2c_len];
									for (uint8_t i = 0;i<i2c_len;i++)
									{
										pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2,4+i, &pxParameterStringLength);
										if(pxParameterStringLength > 0)
										{
											i2c_data[i] = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
										}
										else
										{
											goto pram_err;
										}
									}
									
									 I2C_CMD_WRITE(i2c_addr,&i2c_data[0],i2c_len);
							}
							else
							{
								goto pram_err;
							}
						}
							
						if(!i2c_is_finish())
						{
							goto pram_err;
						}
				}
			}
			else if(pcom[0] == 'R')
			{
				//i2c_addr
				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
				if(pxParameterStringLength > 0)
				{
					i2c_addr = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
				}
				else
				{
					goto pram_err;
				}

				if (i2c_mode == 0)
				{
					uint8_t i2c_reg;
					//i2c_reg
					pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
					if(pxParameterStringLength > 0)
					{
						i2c_reg = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
					}
					else
					{
						goto pram_err;
					}
					if (parm_num == 3)
					{
						uint8_t i2c_data = I2C_BYTE_READ(i2c_addr,i2c_reg);
						
						if(i2c_is_finish())
						{
							len = commpare_length+1;
							memcpy(pcWriteBuffer, pcCommandString, len);
							len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\nOK\r\n",i2c_data);	
							return len;
						}
						goto pram_err; 
					}
					else if (parm_num == 4)
					{
						//i2c_len
						uint8_t i2c_len;
						pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 3, &pxParameterStringLength);
						if(pxParameterStringLength > 0)
						{
							i2c_len = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
						}
						else
						{
							goto pram_err;
						}
						uint8_t i2c_data[i2c_len];
						
						if (I2C_nBYTE_READ(i2c_addr,i2c_reg,&i2c_data[0],i2c_len) < 0)
							goto read_err;
						if(i2c_is_finish())
						{
							len = commpare_length+1;
							memcpy(pcWriteBuffer, pcCommandString,len);
							for (uint8_t i = 0; i < i2c_len;i++)
								len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\n",i2c_data[i]);
							len += sprintf((char *)pcWriteBuffer + len,"%s\r\n","OK");
							return len;
						}
						goto pram_err; 
					}
				}
				else if (i2c_mode == 1)
				{
				   if (parm_num == 2)
					 {
							uint8_t i2c_data;
						 
							if (I2C_DATA_READ(i2c_addr,&i2c_data,1) < 0)
								goto read_err;
						 
							if(i2c_is_finish())
							{
								len = commpare_length+1;
								memcpy(pcWriteBuffer, pcCommandString, len);
								len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\nOK\r\n",i2c_data);	
								return len;
							}
							goto pram_err; 

					 }
					 else if (parm_num == 3)
					 {
							//i2c_len
							uint8_t i2c_len;
							pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
							if(pxParameterStringLength > 0)
							{
								i2c_len = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
							}
							else
							{
								goto pram_err;
							}
							uint8_t i2c_data[i2c_len];
							if (I2C_DATA_READ(i2c_addr,&i2c_data[0],i2c_len) < 0)
								goto read_err;
							
							if(i2c_is_finish())
							{
								len = commpare_length+1;
								memcpy(pcWriteBuffer, pcCommandString,len);
								for (uint8_t i = 0; i < i2c_len;i++)
									len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\n",i2c_data[i]);
								len += sprintf((char *)pcWriteBuffer + len,"%s\r\n","OK");
								return len;
							}
							goto pram_err; 
					 }
				}
			}
		}
	}
/*
//	if(pcCommandString[commpare_length+1] == '=')
//	{
//		//i2c_addr
//		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//		if(pxParameterStringLength > 0)
//		{
//			i2c_addr = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
//		}
//		else
//		{
//			goto pram_err;
//		}
//		
//		//i2c_reg
//		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
//		if(pxParameterStringLength > 0)
//		{
//			i2c_reg  = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
//		}
//		else
//		{
//			goto pram_err;
//		}
//		
//		// i2c R&W?
//		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
//		if(pxParameterStringLength == 1)
//		{ 
//			syscon_SetPMCR1WithMask(QN_SYSCON,P23_MASK_PIN_CTRL | P24_MASK_PIN_CTRL,P23_I2C_SDA_PIN_CTRL | P24_I2C_SCL_PIN_CTRL);	
//			i2c_init(I2C_SCL_RATIO(100000), i2c_buff, 4);
//			if(pcom[0] == 'R')
//			{
//				if (parm_num == 3)
//				i2c_data = I2C_BYTE_READ(i2c_addr,i2c_reg);
//				if (parm_num == 2)
//					 if (I2C_DATA_READ(i2c_addr,&i2c_data,1) < 0)
//						 goto  pram_err;
//				
//				if(i2c_is_finish())
//				{
//					len = commpare_length+1;
//					memcpy(pcWriteBuffer, pcCommandString, len);
//					len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\nOK\r\n",i2c_data);	
//					return len;
//				}
//				goto pram_err; 
//			}
//			else if(pcom[0] == 'W')
//			{
//				if (parm_num == 4)
//				{
//					//i2c_data
//					pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 3, &pxParameterStringLength);
//					if(pxParameterStringLength > 0)
//					{
//						i2c_data = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
//					}
//					else
//					{
//						goto pram_err;
//					}

//					I2C_BYTE_WRITE(i2c_addr, i2c_reg, i2c_data);

//						
//					if(!i2c_is_finish())
//					{
//						goto pram_err;
//					}
//				}
//				else if (parm_num == 3)
//				{
//					//i2c_data
//					i2c_data = i2c_reg; 	

//					I2C_CMD_WRITE(i2c_addr,&i2c_reg,1);

//						
//					if(!i2c_is_finish())
//					{
//						goto pram_err;
//					}				
//				}
//			}
//			else if(pcom[0] == 'M')
//			{
//				//i2c_addr
//				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//				if(pxParameterStringLength > 0)
//				{
//					i2c_mode = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 
//				}
//				else
//				{
//					goto pram_err;
//				}
//			
//			}				
//		}
//		else
//		{
//			goto pram_err;
//		}	
//	}
//	else
//	{
//		goto pram_err;
//	}*/
	return sprintf((char*)pcWriteBuffer,"OK\r\n");
	
pram_err:	
	return sprintf((char*)pcWriteBuffer,"ERR\r\n");
write_err:
	return sprintf((char*)pcWriteBuffer,"WRITE ERR\r\n");
read_err:
	return sprintf((char*)pcWriteBuffer,"READ ERR\r\n");
} 

int ble_gpio( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	const int8_t *pcom;
	uint32_t pxParameterStringLength;
	uint8_t len;
	
	enum gpio_pin pin;
	enum gpio_level level;
	uint8_t gpio_num;
	
	
	pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
	if(pxParameterStringLength > 0)
	{
		gpio_num = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength);
		switch(gpio_num)
		{
			case 0:
				pin = GPIO_P10;
				break;
			case 1:
				pin = GPIO_P11;
				break;
			case 2:
				pin = GPIO_P12;
				break;
			case 3:
				pin = GPIO_P13;
				break;
			case 4:
				pin = GPIO_P23;
				break;
			case 5:
				pin = GPIO_P24;
				break;
			case 6:
				pin = GPIO_P26;
				break;
			case 7:
				pin = GPIO_P27;
				break;
			default:
				goto pram_err;
//				break;
		}

		uint32_t x = pin;
    int n = 0;
    if (x == 0) n = 32;
    if ((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
    if ((x & 0x000000FF) == 0) { n +=  8; x >>=  8; }
    if ((x & 0x0000000F) == 0) { n +=  4; x >>=  4; }
    if ((x & 0x00000003) == 0) { n +=  2; x >>=  2; }
    if ((x & 0x00000001) == 0) { n +=  1; }
			
    if (n < 16) 
    {
			syscon_SetPMCR0WithMask(QN_SYSCON,0x03 << n, 0 << n);
    }
    else 
    {
			syscon_SetPMCR1WithMask(QN_SYSCON,0x03 << (n-16), 0 << (n-16));
    }
	}
	else
	{
		goto pram_err;
	}
	if(pcCommandString[commpare_length+1] == '?' )
	{
		gpio_set_direction(pin, GPIO_INPUT);
		if(gpio_read_pin(pin) == GPIO_LOW)
		{
			gpio_num = 0;
		}
		else
		{
			gpio_num = 1;
		}
		len = commpare_length+1;
		memcpy(pcWriteBuffer, pcCommandString, len);
		len += sprintf((char *)pcWriteBuffer + len,":%d\r\nOK\r\n",gpio_num);	
		return len;
	}
	else if(pcCommandString[commpare_length+1] == '=')
	{
		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
		if(pxParameterStringLength == 1)
		{
			if(pcom[0] == '0')
			{
				level = GPIO_LOW;
			}
			else if (pcom[0] == '1')
			{
				level = GPIO_HIGH;
			}
			else
			{
				goto pram_err;
			}
			gpio_set_direction(pin, GPIO_OUTPUT);
			gpio_write_pin(pin, level);
		}	
		else
		{
			goto pram_err;		
		}

	}
	else
	{
		goto pram_err;
	}
	return sprintf((char*)pcWriteBuffer,"OK\r\n");	
	
pram_err:	
	return sprintf((char*)pcWriteBuffer,"ERR\r\n");
}

void spi_tx_cb(void)
{
	uint32_t tx_data_len = spi_data.tx_len * 5;
	uint8_t tx_data[tx_data_len];
	com_pdu_send(sizeof("SPI write success!\r\n"),"SPI write success!\r\n");
	if (at_HexToString(tx_data,&spi_data.tx_data[0],spi_data.tx_len))
	{
		com_pdu_send(tx_data_len,tx_data);
		com_pdu_send(sizeof("\r\n"),"\r\n");
	}
}

void spi_rx_cb(void)
{
	uint32_t rx_data_len = spi_data.rx_len * 5;
	uint8_t rx_data[rx_data_len];
	//com_pdu_send(sizeof("SPI write success!\r\n"),"SPI write success!\r\n");
	if (at_HexToString(rx_data,&spi_data.rx_data[0],spi_data.rx_len))
	{
		com_pdu_send(rx_data_len,rx_data);
		com_pdu_send(sizeof("\r\n"),"\r\n");
	}
}

int ble_spi( const uint8_t * const pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length)
{
	const int8_t *pcom;
	uint32_t pxParameterStringLength;
	uint8_t len;
	uint8_t parm_num = at_get_parameters_numbers((const uint8_t*)pcCommandString + commpare_length + 2);
	pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
	switch (pcom[0])
	{
		case	'0'	:
		{
			USR_SPI_CS_DIS();
		}break;
		
		case '1'	:
		{
			uint8_t mode;
			uint32_t clk;
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
			mode = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
			if (mode > 3)
				goto pram_err;
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
			clk = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
			if ((clk < 1000) || (clk > 2000000))
				goto pram_err;
			usr_spi_init((enum SPI_MODE_)len,clk);
			USR_SPI_CS_EN();	
		}break;
		
		case	'W'	:
		{
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
			spi_data.tx_len = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
			for (uint8_t i = 0;i < spi_data.tx_len;i++)
			{
				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2+i, &pxParameterStringLength);
				spi_data.tx_data[i] = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
			}
			if (spi_check_tx_free(QN_SPI1) != SPI_TX_FREE)
				goto busy_err;
			spi_write(QN_SPI1,&spi_data.tx_data[0],spi_data.tx_len,spi_tx_cb);
		}break;
		case	'R'	:
		{
			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
			spi_data.rx_len = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
			for (uint8_t i = 0;i < spi_data.tx_len;i++)
			{
//				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2+i, &pxParameterStringLength);
//				spi_data.tx_data[i] = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
					spi_data.rx_data[i] = 0;
			}
			spi_read(QN_SPI1,&spi_data.rx_data[0],spi_data.rx_len,spi_rx_cb);
		}break;
		
		default	: break;
	
	}
/*//		if (pcom[0] == '1')
//		{
//			uint8_t mode;
//			uint32_t clk;
//			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//			mode = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
//			if (mode > 3)
//				goto pram_err;
//			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 2, &pxParameterStringLength);
//      clk = at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
//			if (1000 < clk <2000000)
//			{                                                                                                               
//				usr_spi_init((enum SPI_MODE_)len,clk);
//				USR_SPI_CS_EN();
//				len += sprintf((char *)pcWriteBuffer + len,":0x%02x\r\nOK\r\n",parm_num);	
//				return len;		
//			}
//			else
//				goto pram_err;
//		}
//		else if (pcom[0] == 'W')
//		{
//			uint8_t spi_len;	
//			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//			spi_len =  at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
//			if (spi_len > SPI_RX_MAX)
//				goto pram_err;
//			if (parm_num == (spi_len + 2))
//			{
//					for (uint8_t i = 0;i<spi_len;i++)
//					{
//						pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2,2+i, &pxParameterStringLength);
//						if(pxParameterStringLength > 0)
//						{
//							spi_data.tx_data[i] = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength); 	
//						}
//						else
//						{
//							goto pram_err;
//						}
//					}
//					if (SPI_TX_FREE != spi_check_tx_free(QN_SPI1))
//						goto busy_err;
//						spi_write(QN_SPI1,&spi_data.tx_data[0],spi_len,spi_tx_cb);
//			}
//			else
//			{
//				goto pram_err;
//			}

//		}
//		else if(pcom[0] == 'R')
//		{
//			uint32_t spi_len;	
//			pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
//			spi_len =  at_HEXstringToNum((const uint8_t *)pcom,pxParameterStringLength);
//			if (spi_len > SPI_RX_MAX)
//				goto pram_err;
//			if (parm_num == 2)
//			{
//					for (uint8_t i = 0;i<spi_len;i++)
//					{
//						spi_data.rx_data[i] = 0;
//					}
//					spi_read(QN_SPI1,&spi_data.rx_data[0],spi_len,spi_rx_cb);
//			}
//			else
//			{
//				goto pram_err;
//			}

//		}			
//	}
//	else if(parm_num == 1)
//	{
//		pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 0, &pxParameterStringLength);
//		if(pxParameterStringLength == 1)
//		{
//			if(pcom[0] == '0')
//			{
//				USR_SPI_CS_DIS();
//			}
////			else if (pcom[0] == '1')
////			{
////				pcom = at_get_parameter((const int8_t*)pcCommandString + commpare_length + 2, 1, &pxParameterStringLength);
////				len = at_HEXstringToNum((const uint8_t *)pcom, pxParameterStringLength);
////				if(len > 3)
////				{
////					goto pram_err;	
////				}			
////				usr_spi_init((enum SPI_MODE_)len);
////				USR_SPI_CS_EN();
////			}
//			else
//			{
//				goto pram_err;
//			}
//		}	
//		else
//		{
//			goto pram_err;		
//		}
//		
//	}
//	else
//	{
//		goto pram_err;	
//	}*/
	return sprintf((char*)pcWriteBuffer,"OK\r\n");	
	
pram_err:	
	return sprintf((char*)pcWriteBuffer,"ERR\r\n");
busy_err:
	return sprintf((char*)pcWriteBuffer,"BUSY!RETRY!\r\n");
}

const At_CommandInput command[] =
{	
	{
		"BAUD",
		 set_baudrate
	},
	{
		"VERSION",
		 get_version
	},
	{
		"NAME",
		 get_name
	},
	{
		"DISCON",
		 ble_discon
	},	
	{
		"ADV",
		 ble_adv
	},
	{
		"PWM",
		 ble_pwm
	},
	{
		"I2C",
		 ble_i2c
	},
	{
		"GPIO",
		 ble_gpio
	},	
	{
		"SPI",
		 ble_spi
	},	
};


uint8_t at_command_length_get(const uint8_t *command)
{

	const uint8_t *sc;

	for (sc = command; (*sc != '\0') && (*sc != ' ') && (*sc != '?') && (*sc != '='); ++sc)
		/* nothing */;
	return sc - command;

}

/*
****************************************************************************************
* @brief             at_get_parameters_numbers
* @param[in]         pcCommandString
* @response          None
* @return            Я�������ĸ���
* @description       ����������Я�������ĸ���
*****************************************************************************************/
uint8_t at_get_parameters_numbers( const uint8_t * pcCommandString )
{
	uint8_t cParameters = 0;
	const uint8_t * b_pcCommandString = pcCommandString;
	while( *pcCommandString != 0x00 )
	{
		if( ( *pcCommandString ) == ',' )
		{
				cParameters++;
		}
		pcCommandString++;
	}
	if(b_pcCommandString != pcCommandString)
		cParameters++;
	return cParameters;
}

/*
****************************************************************************************
* @brief              at_HEXstringToNum
* @param[in]          str				HEX string addr				
* @param[in]          length		HEX srring length						
* @response           None
* @return             result of conversion
* @description
*****************************************************************************************/
uint32_t at_HEXstringToNum(const uint8_t *str, uint32_t length)
{  
 uint8_t  revstr[16]={0}; 
 uint32_t   num[16]={0};  
 uint32_t   count=1;  
 uint32_t   result=0; 
 int 	i;
 memcpy(revstr,str,16);  
 for(i=length-1;i>=0;i--)  
 {  
	
	if ((revstr[i]>='0') && (revstr[i]<='9'))  
	   num[i]=revstr[i]-48;
	else if ((revstr[i]>='a') && (revstr[i]<='f'))  
	   num[i]=revstr[i]-'a'+10;  
	else if ((revstr[i]>='A') && (revstr[i]<='F'))  
	   num[i]=revstr[i]-'A'+10;  
//	else if ((revstr[i]>='k') && (revstr[i]<='K'))
//	{		
//	   num[i+2]=0;
//	   num[i+1]=0;
//	   num[i]=0;
//	}		
	else  
	   num[i]=0;


	if('x' == revstr[1] || 'X' == revstr[1]) {

		result=result+num[i]*count;
		if (((revstr[i-1]>='k') && (revstr[i-1]<='K')))
			count=count*16*16*16;
		count=count*16;

	 }
	 
	 else {
		 
		result=result+num[i]*count;  
		if (((revstr[i-1]>='k') && (revstr[i-1]<='K')))
			count=count*10*10*10;
		count=count*10;
	}
	
 }  
 	
 return result;  
}

uint32_t at_HexToString(uint8_t *str,uint8_t *hex,uint32_t hex_len)
{
	uint8_t temp_h = 0,temp_l = 0;
  for (uint8_t i = 0;i < hex_len;i++)	
	{
		temp_h = ((hex[i] >> 4) & 0x0f);
		temp_l = (hex[i] & 0x0f);
		
		if (temp_h > 0x9)
			temp_h = 'a' + (temp_h - 0xa);
		else
			temp_h = '0' + temp_h;
		
		if (temp_l > 0x9)
			temp_l = 'a' + (temp_l - 0xa);
		else
			temp_l = '0' + temp_l;
		str[i*5+0] = '0';
		str[i*5+1] = 'x';
		str[i*5+2] = temp_h;
		str[i*5+3] = temp_l;
		str[i*5+4] = ' ';
	}
	return sizeof(str);
}

/*
****************************************************************************************
* @brief           at_get_parameter
* @param[in]       pcCommandString    						��������ַ���ָ��
* @param[in]       uxWantedParameter  						Ԥ�ڻ�ȡ������ţ���0��ʼ�����������","�ָ���
* @param[in]       pxParameterStringLength    		��uxWantedParameter�������ĳ���
* @response        None
* @return          pcReturn												��uxWantedParameter��������ָ��
* @description     �ú�������Ϊ��ȡ�����е�uxWantedParameter������
*****************************************************************************************/
const int8_t *at_get_parameter( const int8_t* pcCommandString, int32_t uxWantedParameter, uint32_t *pxParameterStringLength )
{
	//��ʶ��ȡ���Ĳ������
	int uxParametersFound = 0;
	
	//���ڼ����������
	const int8_t *pcReturn = pcCommandString;
	
  //��ʼ����������
	*pxParameterStringLength = 0;
	
	//��ȡ��Ԥ�ڲ������Ϊֹ
	while(uxParametersFound <= uxWantedParameter)
	{
		if( *pcCommandString != 0x00 )
		{
			while( ( ( *pcCommandString ) != 0x00 ) && ( ( *pcCommandString ) != ',' ) )
			{
				pcCommandString++;
			}
			//�����Ԥ�ڻ�ȡ����Ų���������������ȣ��Ͽ�ѭ��
			if(uxParametersFound  ==  uxWantedParameter)
			{
				*pxParameterStringLength = pcCommandString - pcReturn;
				break;
			}
			//���δ��Ԥ�ڲ�����ţ���ô��pcReturn����Ϊ��һ����������ʼλ��
			uxParametersFound++;
			if(( *pcCommandString ) != 0x00)
			{
				pcCommandString++;
				pcReturn = pcCommandString;
			}
		}
		else
		{
			break;
		}
	}
	
	return pcReturn;
}


/*
****************************************************************************************
* @brief
* @param[in]
* @response
* @return
* @description
*****************************************************************************************/
int at_process_command(const uint8_t* const pcCommandInput,uint8_t* pcWriteBuffer)
{
	uint8_t i;
	uint8_t s_input_length;
	uint8_t s_command_length;
	uint8_t s_commpare_length;
	const uint8_t *pcCommandString;
	int return_count = 0;
	
	// calculate the input length of at command,input at command start with "+"
	s_input_length   = at_command_length_get((const uint8_t *)pcCommandInput+1);
	
	// loop all pc_command
	for(i = 0;i < sizeof(command)/sizeof(At_CommandInput);i++)
	{
		//get the pc_command in turn
		pcCommandString  = command[i].pcCommand;
		
		//calculate the pc_command
		s_command_length = at_command_length_get((const uint8_t *)pcCommandString);

		//get the longest command length
		s_commpare_length = s_input_length > s_command_length?s_input_length:s_input_length;
		
		//compare nsize of the two command,if it's the same,jump to the relevant function 
		if (strncmp((const char *)pcCommandInput+1, (const char *)pcCommandString, s_commpare_length) == 0 ) 
		{
			return_count = command[i].pxCommandInterpreter(pcCommandInput,pcWriteBuffer,s_commpare_length);

			break;
		}		
	}
	// input_command not find in all pc_command
	if(i == sizeof(command)/sizeof(At_CommandInput))
	{
		return_count = sprintf((char *)pcWriteBuffer,"not find comamnd\r\n");
	}
	return return_count;
}



