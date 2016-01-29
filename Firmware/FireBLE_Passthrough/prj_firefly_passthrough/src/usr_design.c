/**
 ****************************************************************************************
 *
 * @file usr_design.c
 *
 * @brief Product related design.
 *
 * Copyright (C) Quintic 2012-2013
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

#include <stdint.h>
#include "app_env.h"
#include "led.h"
#include "uart.h"
#include "lib.h"
#include "usr_design.h"
#include "gpio.h"
#if (defined(QN_ADV_WDT))
#include "wdt.h"
#endif
#include "sleep.h"

#if QN_COM
#include "app_com.h"
#endif

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



///IOS Connection Parameter
#define IOS_CONN_INTV_MAX                              0x0012
#define IOS_CONN_INTV_MIN                              0x0008
#define IOS_SLAVE_LATENCY                              0x0000
#define IOS_STO_MULT                                   0x012c

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
 
/*

* FUNCTION DEFINITIONS

****************************************************************************************

*/
  
#if (defined(QN_ADV_WDT))
static void adv_wdt_to_handler(void)
{
    ke_state_set(TASK_APP, APP_IDLE);

    // start adv
    app_gap_adv_start_req(GAP_GEN_DISCOVERABLE|GAP_UND_CONNECTABLE,
                          app_env.adv_data, app_set_adv_data(GAP_GEN_DISCOVERABLE),
                          app_env.scanrsp_data, app_set_scan_rsp_data(app_get_local_service_flag()),
                          GAP_ADV_FAST_INTV1, GAP_ADV_FAST_INTV2);
}
struct usr_env_tag usr_env = {LED_ON_DUR_IDLE, LED_OFF_DUR_IDLE, false, adv_wdt_to_handler};
#else
struct usr_env_tag usr_env = {LED_ON_DUR_IDLE, LED_OFF_DUR_IDLE};
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief   Led1 for BLE status
 ****************************************************************************************
 */
void usr_led1_set(uint16_t timer_on, uint16_t timer_off)
{
    usr_env.led1_on_dur = timer_on;
    usr_env.led1_off_dur = timer_off;

    if (timer_on == 0 || timer_off == 0)
    {
        if (timer_on == 0)
        {
            led_set(1, LED_OFF);
        }
        if (timer_off == 0)
        {
            led_set(1, LED_ON);
        }
        ke_timer_clear(APP_SYS_LED_1_TIMER, TASK_APP);
    }
    else
    {
        led_set(1, LED_OFF);
        ke_timer_set(APP_SYS_LED_1_TIMER, TASK_APP, timer_off);
    }
}

/**
 ****************************************************************************************
 * @brief   Led 1 flash process
 ****************************************************************************************
 */
static void usr_led1_process(void)
{
    if(led_get(1) == LED_ON)
    {
        led_set(1, LED_OFF);
        ke_timer_set(APP_SYS_LED_1_TIMER, TASK_APP, usr_env.led1_off_dur);
    }
    else
    {
        led_set(1, LED_ON);
        ke_timer_set(APP_SYS_LED_1_TIMER, TASK_APP, usr_env.led1_on_dur);
    }
}


/**
 ****************************************************************************************
 * @brief   Application task message handler
 ****************************************************************************************
 */
void app_task_msg_hdl(ke_msg_id_t const msgid, void const *param)
{
    switch(msgid)
    {
        case GAP_SET_MODE_REQ_CMP_EVT:
            if(APP_IDLE == ke_state_get(TASK_APP))
            {
                usr_led1_set(LED_ON_DUR_ADV_FAST, LED_OFF_DUR_ADV_FAST);
                ke_timer_set(APP_ADV_INTV_UPDATE_TIMER, TASK_APP, 30 * 100);
#if (defined(QN_ADV_WDT))
                usr_env.adv_wdt_enable = true;
#endif
            }
            else if(APP_ADV == ke_state_get(TASK_APP))
            {
                usr_led1_set(LED_ON_DUR_ADV_SLOW, LED_OFF_DUR_ADV_SLOW);
#if (defined(QN_ADV_WDT))
                usr_env.adv_wdt_enable = true;
#endif
            }
            break;

        case GAP_DISCON_CMP_EVT:
#if QN_COM
							//clear all event
							ke_evt_clear(1UL << EVENT_UART_RX_FRAME_ID);
							ke_evt_clear(1UL << EVENT_UART_RX_TIMEOUT_ID);
							//clear all timer
							ke_timer_clear(APP_COM_RX_TIMEOUT_TIMER, TASK_APP);
							ke_timer_clear(QPPS_TEST_SEND_TIMER,TASK_APP);
				
							// set com connection status
							com_env.com_conn = COM_DISCONN;
							//when cancel connection and com are still in passthrough traslation mode or passthrough
							//traslation idle mode,auto change to AT traslation mode
							if(com_env.com_mode == COM_MODE_TRAN)
							{
									com_env.com_mode = COM_MODE_IDLE;
							}
#endif
            usr_led1_set(LED_ON_DUR_IDLE, LED_OFF_DUR_IDLE);

            // start adv
            app_gap_adv_start_req(GAP_GEN_DISCOVERABLE|GAP_UND_CONNECTABLE,
                    app_env.adv_data, app_set_adv_data(GAP_GEN_DISCOVERABLE),
                    app_env.scanrsp_data, app_set_scan_rsp_data(app_get_local_service_flag()),
                    GAP_ADV_FAST_INTV1, GAP_ADV_FAST_INTV2);
            break;

        case GAP_LE_CREATE_CONN_REQ_CMP_EVT:
            if(((struct gap_le_create_conn_req_cmp_evt *)param)->conn_info.status == CO_ERROR_NO_ERROR)
            {
#if 	QN_COM			
								// com status init
								com_env.com_conn = COM_CONN;
								//in the connection,if meet the following conditions,FS_QN9021 will enter passthrough mode from AT mode
								if(com_env.com_mode == COM_MODE_AT && (gpio_read_pin(COM_AT_ENABLE) == GPIO_HIGH))
								{
									led_set(2, LED_OFF);
									uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
									// if not all notify no,it will enter idle mode
									if (bit_num >= QPPS_VAL_CHAR_NUM)
									{		
										com_env.com_mode = COM_MODE_TRAN;						
										com_wakeup_handler();
										com_uart_rx_start();
									}
									else
									{
										com_env.com_mode = COM_MODE_IDLE;
									}
								}
#endif								
                if(GAP_PERIPHERAL_SLV == app_get_role())
                {
                    ke_timer_clear(APP_ADV_INTV_UPDATE_TIMER, TASK_APP);
                    usr_led1_set(LED_ON_DUR_CON, LED_OFF_DUR_CON);
#if (defined(QN_ADV_WDT))
                    usr_env.adv_wdt_enable = false;
#endif

                    // Update cnx parameters
                    //if (((struct gap_le_create_conn_req_cmp_evt *)param)->conn_info.con_interval >  IOS_CONN_INTV_MAX)
                    {
                        // Update connection parameters here
                        struct gap_conn_param_update conn_par;
                        /// Connection interval minimum
                        conn_par.intv_min = IOS_CONN_INTV_MIN;
                        /// Connection interval maximum
                        conn_par.intv_max = IOS_CONN_INTV_MAX;
                        /// Latency
                        conn_par.latency = IOS_SLAVE_LATENCY;
                        /// Supervision timeout, Time = N * 10 msec
                        conn_par.time_out = IOS_STO_MULT;
                        app_gap_param_update_req(((struct gap_le_create_conn_req_cmp_evt *)param)->conn_info.conhdl, &conn_par);
                    }
                }
            }
            break;

        case QPPS_DISABLE_IND:
            break;

        case QPPS_CFG_INDNTF_IND:
				{
#if QN_COM
						//com_wakeup_handler();
            uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
						// all notify is on and enter TRAN_MODE ,or Enter TARN_IDLE_MODE
            if (bit_num >= QPPS_VAL_CHAR_NUM)  
            {                
                QPRINTF("\r\nAll notify is on!\r\n");
								if(com_env.com_mode == COM_MODE_IDLE)
								{
									com_env.com_mode = COM_MODE_TRAN;
									com_wakeup_handler();
								}
								if(com_env.com_mode == COM_MODE_TRAN)
								{
									com_uart_rx_start();
								}
								//ke_timer_set(QPPS_TEST_SEND_TIMER,TASK_APP,50);
            }
						else
						{
								if(com_env.com_mode == COM_MODE_TRAN)
								{
									com_env.com_mode = COM_MODE_IDLE;
									com_wakeup_handler();
								}
								//ke_timer_clear(QPPS_TEST_SEND_TIMER,TASK_APP);
						}
#endif            
        }break;
				case QPPS_DAVA_VAL_IND:
							{
									/// passthrough the data from client to com 
									struct qpps_data_val_ind* par = (struct qpps_data_val_ind*)param;
									
								  if (par->length > 0)
									{
#if QN_COM								
										if(	com_env.com_mode == COM_MODE_TRAN)
										{
											//开启自动换行
											if (com_env.auto_line_feed == COM_LF)
											{
												 par->length += 2;
												 par->data[par->length-2] = 0x0D;
												 par->data[par->length-1]	= 0x0A;
												 //发送到串口1
												 com_pdu_send(par->length,&(par->data[0]));
											}
											else
											{											
												 //发送到串口1
												 com_pdu_send(par->length,&(par->data[0]));
											}
										}
#endif								
									}
							}
						break;
							
				case QPPS_DATA_SEND_CFM:
						{
							  /// 发送完成后需要重新置位char_status,表明该特征已经可以开始下一轮的数据发送了
								app_qpps_env->char_status |= (QPPS_VALUE_NTF_CFG << ((struct qpps_data_send_cfm *)param)->char_index);
#if QN_COM							
								uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
								/// if com_mode is  COM_MODE_TRAN and data has send to client success,continiu receive data from com
								if (bit_num >= QPPS_VAL_CHAR_NUM)
								{										
										if (!co_list_is_empty(&com_env.queue_rx))
											app_tx_done();    //继续取包发送数据
										if(com_env.com_mode == COM_MODE_TRAN)
										{
											com_uart_rx_start();   
										}
								}
#endif								
						}
						break;
        case OTAS_TRANSIMIT_STATUS_IND:
            //only need response once when ota status is in ota status start request
            if(((struct otas_transimit_status_ind*)param)->status == OTA_STATUS_START_REQ)  
            {
                app_ota_ctrl_resp(START_OTA);
            }
            break;
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles LED status timer.
 *
 * @param[in] msgid      APP_SYS_UART_DATA_IND
 * @param[in] param      Pointer to struct app_uart_data_ind
 * @param[in] dest_id    TASK_APP
 * @param[in] src_id     TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_led_timer_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if(msgid == APP_SYS_LED_1_TIMER)
    {
        usr_led1_process();
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles advertising mode timer event.
 *
 * @param[in] msgid     APP_ADV_INTV_UPDATE_TIMER
 * @param[in] param     None
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_APP
 *
 * @return If the message was consumed or not.
 * @description
 *
 * This handler is used to inform the application that first phase of adversting mode is timeout.
 ****************************************************************************************
 */
int app_gap_adv_intv_update_timer_handler(ke_msg_id_t const msgid, void const *param,
                                          ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if(APP_ADV == ke_state_get(TASK_APP))
    {
        usr_led1_set(LED_ON_DUR_IDLE, LED_OFF_DUR_IDLE);

        // Update Advertising Parameters
        app_gap_adv_start_req(GAP_GEN_DISCOVERABLE|GAP_UND_CONNECTABLE, 
                                app_env.adv_data, app_set_adv_data(GAP_GEN_DISCOVERABLE), 
                                app_env.scanrsp_data, app_set_scan_rsp_data(app_get_local_service_flag()),
                                GAP_ADV_SLOW_INTV, GAP_ADV_SLOW_INTV);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief   Restore peripheral setting after wakeup
 ****************************************************************************************
 */
void usr_sleep_restore(void)
{
	
#if defined(QN_COM_UART)
    uart_init(QN_COM_UART, USARTx_CLK(0), UART_9600);
    uart_tx_enable(QN_COM_UART, MASK_ENABLE);
    uart_rx_enable(QN_COM_UART, MASK_ENABLE);
#endif	
	
#if QN_DBG_PRINT
    uart_init(QN_DEBUG_UART, USARTx_CLK(0), UART_9600);
    uart_tx_enable(QN_DEBUG_UART, MASK_ENABLE);
    uart_rx_enable(QN_DEBUG_UART, MASK_ENABLE);
#endif
	
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

#if (defined(QN_ADV_WDT))
    if(usr_env.adv_wdt_enable)
    {
        wdt_init(1007616, WDT_INT_MOD); // 30.75s
    }
#endif
		
		
}


/**
 ****************************************************************************************
 * @brief   Button 1 click callback
 * @description
 *  Button 1 is used to enter adv mode.
 ****************************************************************************************
 */
void usr_at_wakeup_cb(void)
{
    // If BLE is in the sleep mode, wakeup it.
    if(ble_ext_wakeup_allow())
    {
#if ((QN_DEEP_SLEEP_EN) && (!QN_32K_RCO))
        if (sleep_env.deep_sleep)
        {
            wakeup_32k_xtal_switch_clk();
        }
#endif

        sw_wakeup_ble_hw();

    }		
		ke_evt_set(1UL << EVENT_AT_ENABLE_PRESS_ID);
}


/**
 ****************************************************************************************
 * @brief   All GPIO interrupt callback
 ****************************************************************************************
 */
void gpio_interrupt_callback(enum gpio_pin pin)
{
    switch(pin)
    {
        case COM_AT_ENABLE:
							usr_at_wakeup_cb();
            break;
				
#if (defined(QN_TEST_CTRL_PIN))
        case QN_TEST_CTRL_PIN:
            //When test controll pin is changed to low level, this function will reboot system.
            gpio_disable_interrupt(QN_TEST_CTRL_PIN);
            syscon_SetCRSS(QN_SYSCON, SYSCON_MASK_REBOOT_SYS);
            break;
#endif

        default:
            break;
    }
}


/**
 ****************************************************************************************
 * @brief   User initialize
 ****************************************************************************************
 */
void usr_init(void)
{
	
}

/*
****************************************************************************************
* @brief       msgid:QPPS_TEST_SEND_TIMER
* @param[in]
* @response
* @return
* @description @@@Send test data to client
*****************************************************************************************/

int app_qpps_test_send_timer_handler(ke_msg_id_t const msgid, void const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	//QPRINTF("\r\n@@@QPPS_TEST_SEND_TIMER!\r\n");
	ke_timer_clear(QPPS_TEST_SEND_TIMER,TASK_APP);
  static uint8_t i = 0;
	app_qpps_data_send(app_qpps_env->conhdl,0,1,&i);
	i++;
	ke_timer_set(QPPS_TEST_SEND_TIMER,TASK_APP,50);
	return(KE_MSG_CONSUMED);
}

/// @} USR

