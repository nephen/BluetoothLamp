/**
 ****************************************************************************************
 *
 * @file app_com.c
 *
 * @brief Pass through project process
 *
 * Copyright (C) FireFly 2015
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
#include "app_com.h"
#include "uart.h"
#include "lib.h"
#include "sleep.h"
#include "at_command.h"
#include "led.h"


/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */
#define AT_COMMAN_LEN_MAX	255
 

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct com_env_tag  com_env;
uint8_t at_command_return[AT_COMMAN_LEN_MAX];

//default baudrate set to UART_9600
uint8_t baudrate = UART_9600;
struct uart_divider_cfg
{
    uint8_t integer_h;
    uint8_t integer_l;
    uint8_t fractional;
};
extern const struct uart_divider_cfg uart_divider[UART_BAUD_MAX];

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static void app_push(struct ke_msg *msg);

extern uint32_t get_bit_num(uint32_t val);



void show_com_mode(uint8_t com_mode)
{
	QPRINTF("\r\n/*******************************");
	QPRINTF("\r\n *                             *");
	switch(com_mode)
	{
		case	COM_MODE_IDLE	:
		{
			QPRINTF("\r\n *       COM_MODE_IDLE         *");
			break;
		}
		case	COM_MODE_TRAN	:
		{
			QPRINTF("\r\n *       COM_MODE_TRAN         *");
			break;
		}
		case	COM_MODE_AT	:
		{
			QPRINTF("\r\n *        COM_MODE_AT          *");
			break;
		}
		default	:
			QPRINTF("\r\n *      COM_MODE_UNKNOWN        *");
			break;
	}
	QPRINTF("\r\n *                             *");
	QPRINTF("\r\n********************************/\r\n");
}


/*******************************************************************

				COM Init

******************************************************************/

/*
****************************************************************************************
* @brief         com_gpio_init
* @param[in]     None
* @response      None
* @return        None
* @description   Init GPIO and Init the event and it's callback function
*****************************************************************************************/
void com_gpio_init(void)
{
    //set wakeup config,when GPIO low and trigger interrupt
    gpio_wakeup_config(COM_AT_ENABLE,GPIO_WKUP_BY_LOW);
    gpio_enable_interrupt(COM_AT_ENABLE);


    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_AT_ENABLE_PRESS_ID,
                                          app_event_at_enable_press_handler))
    {
        ASSERT_ERR(0);
    }

    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_AT_COMMAND_PROC_ID,
                                          app_com_at_command_handler))
    {
        ASSERT_ERR(0);
    }

}

/*
****************************************************************************************
* @brief          com_init
* @param[in]      None
* @response       None
* @return         None
* @description    Init com state
*****************************************************************************************/
void com_init(void)
{
    //for com uart tx
    com_env.tx_state = COM_UART_TX_IDLE;	//initialize tx state
    com_env.com_conn = COM_DISCONN;
    com_env.com_mode = COM_MODE_IDLE;
    com_env.auto_line_feed = COM_NO_LF;
    co_list_init(&com_env.queue_tx);			//init TX queue
    co_list_init(&com_env.queue_rx);			//init RX queue
	
    com_gpio_init();
	
		show_com_mode(com_env.com_mode);

    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_TX_ID, com_tx_done))
        ASSERT_ERR(0);
    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_RX_FRAME_ID, com_event_uart_rx_frame_handler))
        ASSERT_ERR(0);
    if(KE_EVENT_OK != ke_evt_callback_set(EVENT_UART_RX_TIMEOUT_ID, com_event_uart_rx_timeout_handler))
        ASSERT_ERR(0);

}

/*
****************************************************************************************
* @brief         app_event_com_tx_handler
* @param[in]     None
* @response      None
* @return        None
* @description   set a event when msg are sended and free the space
*****************************************************************************************/

/*******************************************************************
*
* 				COM :  AT Mode and TRAN Mode switch
*
******************************************************************/


/*
****************************************************************************************
* @brief          com_wakeup_handler
* @param[in]      None
* @response       APP_COM_AT_RX_ENABLE_TIMER(app_com_at_rx_enable_handler)
* @return         None
* @description    com wake msg handler
*****************************************************************************************/
void com_wakeup_handler(void)
 {
    switch(com_env.com_mode)
    {
    case COM_MODE_IDLE:
    {
        // Enter COM_MODE_AT when COM_AT_ENABLE¡¡is GPIO_LOW
        if (gpio_read_pin(COM_AT_ENABLE) == GPIO_LOW)
        {
            com_env.com_mode = COM_MODE_AT;
            led_set(2, LED_ON);
            com_uart_at_rx_start();
        }
        // Enter COM_MODE_TRAN when  COM_AT_ENABLE isnot GPIO_LOW and the connection created.
        else if(com_env.com_conn == COM_CONN)
        {
            com_env.com_mode = COM_MODE_IDLE;
            led_set(2, LED_OFF);
            uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
            if (bit_num >= QPPS_VAL_CHAR_NUM)
            {
								com_env.com_mode = COM_MODE_TRAN;
                com_uart_rx_start();
            }
        }
        // Enter COM_MODE_AT when connection is disconnect
        else if(com_env.com_conn == COM_DISCONN)
        {
            com_env.com_mode = COM_MODE_IDLE;
            led_set(2, LED_OFF);
        }
        break;
    }

    case COM_MODE_TRAN:
        // Enter COM_MODE_AT
    {
        com_env.com_mode = COM_MODE_AT;
        led_set(2, LED_ON);
        com_uart_at_rx_start();
        break;
    }
    case COM_MODE_AT:
        // Enter  COM_MODE_TRAN
    {
        com_env.com_mode = COM_MODE_IDLE;
        led_set(2, LED_OFF);
        uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
        if (bit_num >= QPPS_VAL_CHAR_NUM)
        {
						com_env.com_mode = COM_MODE_TRAN;
            com_uart_rx_start();
        }
        break;
    }
    default:
        break;
    }
//	}
    ke_timer_set(APP_COM_AT_RX_ENABLE_TIMER, TASK_APP, 2);
}


/*
****************************************************************************************
* @brief           app_com_at_rx_enable_handler
* @param[in]       msgid
* @param[in]       param
* @param[in]       dest_id
* @param[in]       src_id
* @response        None
* @return          KE_MSG_CONSUMED or ERROR_CODE
* @description
*****************************************************************************************/
int app_com_at_rx_enable_handler(ke_msg_id_t const msgid, void const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch(com_env.com_mode)
    {
    case COM_MODE_IDLE:
        break;
    case COM_MODE_TRAN:
        break;
    case COM_MODE_AT:
        if(gpio_read_pin(COM_AT_ENABLE) == GPIO_HIGH)
        {
            com_env.com_mode = COM_MODE_IDLE;
            if(com_env.com_conn == COM_CONN)
            {
                led_set(2, LED_OFF);
                uint8_t bit_num = get_bit_num(app_qpps_env->char_status);
                if (bit_num >= QPPS_VAL_CHAR_NUM)
                {
                    com_env.com_mode = COM_MODE_TRAN;
                    com_uart_rx_start();
                }
                else
                {
                    com_env.com_mode = COM_MODE_IDLE;
                }
            }
            else
            {
                led_set(2, LED_OFF);
                com_env.com_mode = COM_MODE_IDLE;
            }
        }
        break;
    default:
        break;
    }
		show_com_mode(com_env.com_mode);
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles at_enable press.
 ****************************************************************************************
 */
void app_event_at_enable_press_handler(void)
{
    ke_evt_clear(1UL << EVENT_AT_ENABLE_PRESS_ID);
#if ((QN_DEEP_SLEEP_EN) && (!QN_32K_RCO))
    if (sleep_env.deep_sleep)
    {
        sleep_env.deep_sleep = false;
        // start 32k xtal wakeup timer
        wakeup_32k_xtal_start_timer();
    }
#endif
    com_wakeup_handler();
}



/*******************************************************************
*
* 				COM :  RX fuction in TRAN Mode
*
******************************************************************/

/*
****************************************************************************************
* @brief               com_uart_rx
* @param[in]           None
* @response            None
* @return              None
* @description         Read data form com RX
*****************************************************************************************/
void com_uart_rx(void)
{
    //continue receive the data for RX
    com_env.com_rx_len++;
    //set pt gpio state
    if(com_env.com_rx_len == QPPS_VAL_CHAR_NUM_MAX*QPP_DATA_MAX_LEN)  //receive data buf is full, should sent them to ble
    {
        ke_evt_set(1UL << EVENT_UART_RX_FRAME_ID);
    }
    else
    {
        uart_read(QN_COM_UART, &com_env.com_rx_buf[com_env.com_rx_len], 1, com_uart_rx);
        ke_evt_set(1UL << EVENT_UART_RX_TIMEOUT_ID);
    }
}

void	com_uart_rx_start(void)
{
    uart_uart_ClrIntFlag(CFG_COM_UART,0x1ff);
    uart_uart_GetRXD(CFG_COM_UART);
		com_env.com_rx_len = 0;
		uart_read(QN_COM_UART, &com_env.com_rx_buf[com_env.com_rx_len],1, com_uart_rx);
}

/*******************************************************************

					AT	Command function

******************************************************************/


void com_uart_at(void)
{
    // command receive over,send them to ble
    if(com_env.com_at_buf[com_env.com_at_len] == '\n' )
    {
        ke_evt_set(1UL << EVENT_AT_COMMAND_PROC_ID);
    }
    else
    {
        com_env.com_at_len++;
        //if receive data buf is not full,continue receive the command data.
        //while receive data buf is full, should sent them to ble
        if(com_env.com_at_len < AT_COMMAN_LEN_MAX)
        {
            uart_read(QN_COM_UART, &com_env.com_at_buf[com_env.com_at_len], 1, com_uart_at);
        }
        else
        {
            com_env.com_at_buf[AT_COMMAN_LEN_MAX -1] = '\0';
            ke_evt_set(1UL << EVENT_AT_COMMAND_PROC_ID);
        }
    }
}

void app_com_at_command_handler(void)
{
#ifdef CATCH_LOG
		QPRINTF("at_command:\r\n");

		for(uint8_t i = 0;i < com_env.com_at_len+1;i++)
			QPRINTF("%c",com_env.com_at_buf[i]);
#endif
	
    //AT Comand length is more than 3
    if(com_env.com_at_len >= 3)
    {
        // if it's start with "AT"
        if(com_env.com_at_buf[0] == 'A' && com_env.com_at_buf[1] == 'T')
        {
            //Test if Enter the AT mode
            if(com_env.com_at_len == 3)
            {
                com_pdu_send(sprintf((char *)at_command_return,"OK\r\n"),at_command_return);
            }
            else
            {
                //if it's a AT Command ,send it to at_process_command process.
                if(com_env.com_at_buf[2] == '+')
                {
                    com_env.com_at_buf[com_env.com_at_len-1] = '\0';
                    com_pdu_send(at_process_command(com_env.com_at_buf + 2,at_command_return),at_command_return);
                }
                else
                {
                    com_pdu_send(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
                }
            }
        }
        else
        {
            com_pdu_send(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
        }
    }
    else
    {
        com_pdu_send(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
    }
    //receive continue
    com_uart_at_rx_start();
    ke_evt_clear(1UL << EVENT_AT_COMMAND_PROC_ID);
}

void com_uart_at_rx_start(void)
{
    uart_uart_ClrIntFlag(CFG_COM_UART,0x1ff);
    uart_uart_GetRXD(CFG_COM_UART);
    com_env.com_at_len = 0;
    uart_read(QN_COM_UART, &com_env.com_at_buf[com_env.com_at_len], 1, com_uart_at);
}

int app_com_at_baudrate_change_handler(ke_msg_id_t const msgid, void const *param,
                                       ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint32_t reg;

//	QPRINTF("app_com_at_baudrate_change_handler:%d\r\n",baudrate);
    reg = (uart_divider[baudrate].integer_h << (UART_POS_DIVIDER_INT + 8))
          | (uart_divider[baudrate].integer_l << UART_POS_DIVIDER_INT)
          | uart_divider[baudrate].fractional;
    uart_uart_SetBaudDivider(QN_COM_UART, reg);

    return (KE_MSG_CONSUMED);
}

void com_event_uart_rx_timeout_handler(void)
{
    ke_timer_set(APP_COM_RX_TIMEOUT_TIMER, TASK_APP, COM_FRAME_TIMEOUT);
    ke_evt_clear(1UL << EVENT_UART_RX_TIMEOUT_ID);
}

int app_com_rx_timeout_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uart_rx_int_enable(QN_COM_UART, MASK_DISABLE);  //disable uart rx interrupt
    struct app_uart_data_ind *com_data = ke_msg_alloc(APP_COM_UART_RX_DONE_IND,
                                         TASK_APP,
                                         TASK_APP,
                                         com_env.com_rx_len+1);
    com_data->len=com_env.com_rx_len;
    memcpy(com_data->data,com_env.com_rx_buf,com_env.com_rx_len);
    ke_msg_send(com_data);

    return (KE_MSG_CONSUMED);
}

void com_event_uart_rx_frame_handler(void)
{
    uart_rx_int_enable(QN_COM_UART, MASK_DISABLE);  //disable uart rx interrupt
    struct app_uart_data_ind *com_data = ke_msg_alloc(APP_COM_UART_RX_DONE_IND,
                                         TASK_APP,
                                         TASK_APP,
                                         com_env.com_rx_len+1);
    com_data->len=com_env.com_rx_len;
    memcpy(com_data->data,com_env.com_rx_buf,com_env.com_rx_len);
    ke_msg_send(com_data);

    ke_timer_clear(APP_COM_RX_TIMEOUT_TIMER, TASK_APP);
    ke_evt_clear(1UL << EVENT_UART_RX_FRAME_ID);
}


void dev_send_to_app(struct app_uart_data_ind *param)
{
    uint8_t *buf_20;
    int16_t len = param->len;
    int16_t send_len = 0;
	
		uint8_t packet_len = get_bit_num(app_qpps_env->char_status)*20;
#ifdef	CATCH_LOG
	QPRINTF("\r\ntx len %d  data : ",len);

    for(uint8_t j = 0; j<len; j++)
        QPRINTF("%c",param->data[j]);
    QPRINTF("\r\n");
#endif
    if(app_qpps_env->char_status)
    {
        for(uint8_t i =0; send_len < len; i++)
        {
            if (len > packet_len) //Split data into package when len longger than 20
            {
                if (len - send_len > packet_len)	
                {
                    buf_20 = (uint8_t*)ke_msg_alloc(0, 0, 0, packet_len);
                    if(buf_20 != NULL)
                    {
                        memcpy(buf_20,param->data+send_len,packet_len);
                        send_len+=packet_len;
                    }
                }
                else
                {
                    buf_20 = (uint8_t *)ke_msg_alloc(0,0,0,len-send_len);
                    if (buf_20 != NULL)
                    {
                        memcpy(buf_20,param->data+send_len,len-send_len);
                        send_len = len;
                    }
                }
            }
            else	//not longger ther 20 send data directely
            {
								buf_20 = (uint8_t *)ke_msg_alloc(0,0,0,len);
								if (buf_20 != NULL)
								{
										memcpy(buf_20,param->data,len);
										send_len = len;
								}
                //app_qpps_data_send(app_qpps_env->conhdl,0,len,param->data);
            }
						//push the package to kernel queue.
						app_push(ke_param2msg(buf_20));
				}
    }
}

void app_push(struct ke_msg *msg)
{
    // Push the message into the list of messages pending for transmission
    co_list_push_back(&com_env.queue_rx, &msg->hdr);
    //QPRINTF("\r\n@@@app_push:");
//    for (uint8_t i = 0; i<msg->param_len; i++)
//        QPRINTF("%c",((uint8_t *)&msg->param)[i]);
//    QPRINTF("\r\n");
	
		//only send in the first push.
		uint8_t *p_data = (uint8_t *)msg->param;
		uint8_t pack_nb = msg->param_len/QPP_DATA_MAX_LEN + 1;
		uint8_t pack_divide_len =  msg->param_len%QPP_DATA_MAX_LEN;
		for (uint8_t char_idx = 0,i = 0;((app_qpps_env->char_status & (~(QPPS_VALUE_NTF_CFG << (char_idx - 1) ))) && (char_idx < QPPS_VAL_CHAR_NUM ));char_idx++)
		{
			if (i < (pack_nb - 1))
			{
				app_qpps_env->char_status &= ~(QPPS_VALUE_NTF_CFG << char_idx);
				app_qpps_data_send(app_qpps_env->conhdl,char_idx,QPP_DATA_MAX_LEN,(uint8_t *)p_data);
				p_data += QPP_DATA_MAX_LEN;
			}
			else
			{
				if ((pack_divide_len != 0) && (i == (pack_nb - 1)))
				{
					app_qpps_env->char_status &= ~(QPPS_VALUE_NTF_CFG << char_idx);
					app_qpps_data_send(app_qpps_env->conhdl,char_idx,pack_divide_len,(uint8_t *)p_data);
					p_data += pack_divide_len;
				}
			}
			i++;
		}
}

void app_tx_done(void)
{
    struct ke_msg * msg;
    //release current message (which was just sent)
    msg = (struct ke_msg *)co_list_pop_front(&com_env.queue_rx);
    // Free the kernel message space
    ke_msg_free(msg);
    // Check if there is a new message pending for transmission
    if ((msg = (struct ke_msg *)co_list_pick(&com_env.queue_rx)) != NULL)
    {
        // Forward the message to the HCI UART for immediate transmission
//        QPRINTF("\r\napp_tx_done:");
//        for (uint8_t i = 0; i<msg->param_len; i++)
//            QPRINTF("%c",((uint8_t *)&msg->param)[i]);
//        QPRINTF("\r\n");
			
			
				uint8_t *p_data = (uint8_t *)msg->param;
				uint8_t pack_nb = msg->param_len/QPP_DATA_MAX_LEN + 1;
				uint8_t pack_divide_len =  msg->param_len%QPP_DATA_MAX_LEN;
				for (uint8_t char_idx = 0,i = 0;((app_qpps_env->char_status & (~(QPPS_VALUE_NTF_CFG << (char_idx - 1)))) && (char_idx < QPPS_VAL_CHAR_NUM));char_idx++)
				{
					if (i < (pack_nb - 1))
					{
						app_qpps_env->char_status &= ~(QPPS_VALUE_NTF_CFG << char_idx);
						app_qpps_data_send(app_qpps_env->conhdl,char_idx,QPP_DATA_MAX_LEN,(uint8_t *)p_data);
						p_data += QPP_DATA_MAX_LEN;
					}
					else
					{
						if ((pack_divide_len != 0) && (i == (pack_nb - 1)))
						{
							app_qpps_env->char_status &= ~(QPPS_VALUE_NTF_CFG << char_idx);
							app_qpps_data_send(app_qpps_env->conhdl,char_idx,pack_divide_len,(uint8_t *)p_data);
							p_data += pack_divide_len;
						}
					}
					i++;
				}
				
    }

//    QPRINTF("app_tx_done\r\n");
}

int app_com_uart_rx_done_ind_handler(ke_msg_id_t const msgid, void const *param,
                                     ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch(msgid)
    {
    case APP_COM_UART_RX_DONE_IND:
    {
        struct app_uart_data_ind* frame = (struct app_uart_data_ind*)param;


        if(frame->len) //have data
        {
            dev_send_to_app(frame);
        }
    }
    break;
    default :
        break;
    }

    return (KE_MSG_CONSUMED);
}





void app_event_com_tx_handler(void)
{
    ke_evt_set(1UL<<EVENT_UART_TX_ID);
}

/*******************************************************************
*																																	*
*					COM TX																									*
*																																	*
******************************************************************/

/*
****************************************************************************************
* @brief          com_uart_write
* @param[in]      struct ke_msg *msg						the msg will be transmit to com
* @response       app_event_com_tx_handler		  finish tx and free space
* @return         None
* @description
*****************************************************************************************/
void com_uart_write(struct ke_msg *msg)
{
    //go to start tx state
    com_env.tx_state = COM_UART_TX_ONGOING;
    uart_write(QN_COM_UART, ((uint8_t *)&msg->param), msg->param_len, app_event_com_tx_handler);
}


/**
****************************************************************************************
* @brief After-process when one PDU has been sent.
*
****************************************************************************************
*/
void com_tx_done(void)
{
    struct ke_msg * msg;
    // Clear the event
    ke_evt_clear(1<<EVENT_UART_TX_ID);
    // Go back to IDLE state
    com_env.tx_state = COM_UART_TX_IDLE;
    //release current message (which was just sent)
    msg = (struct ke_msg *)co_list_pop_front(&com_env.queue_tx);
    // Free the kernel message space
    ke_msg_free(msg);
    // Check if there is a new message pending for transmission
    if ((msg = (struct ke_msg *)co_list_pick(&com_env.queue_tx)) != NULL)
    {
        // Forward the message to the HCI UART for immediate transmission
        com_uart_write(msg);
    }
}

// Push msg into eaci tx queue
static void com_push(struct ke_msg *msg)
{
    // Push the message into the list of messages pending for transmission
    co_list_push_back(&com_env.queue_tx, &msg->hdr);

    // Check if there is no transmission ongoing
    if (com_env.tx_state == COM_UART_TX_IDLE)
        // Forward the message to the HCI UART for immediate transmission
        com_uart_write(msg);
}

/**
 ****************************************************************************************
 * @brief EACI send PDU
 *
 ****************************************************************************************
 */
void com_pdu_send(uint8_t len, uint8_t *par)
{
    // Allocate one msg for EACI tx
    uint8_t *msg_param = (uint8_t*)ke_msg_alloc(0, 0, 0, len);

    // Save the PDU in the MSG
    memcpy(msg_param, par, len);

    //extract the ke_msg pointer from the param passed and push it in HCI queue
    com_push(ke_param2msg(msg_param));
}

//end
