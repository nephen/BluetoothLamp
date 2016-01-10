/**
 ****************************************************************************************
 *
 * @file app_com.h
 *
 * @brief UART transport module functions for Easy Application Controller Interface.
 *
 * Copyright (C) Quintic 2012-2013
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

#ifndef APP_PT_H_
#define APP_PT_H_

#include "app_env.h"

#define COM_FRAME_TIMEOUT 3 //COM_FRAME_TIMEOUT*10ms

#define QPPS_VAL_CHAR_NUM	(1)
#define	QPPS_VAL_CHAR_NUM_MAX	(7)
#define COM_AT_COMM_BUF		(0xff)

#define	COM_RX_ENABLE					(GPIO_P02)  //待定，未确定是否需要
#define	COM_TX_ENABLE					(GPIO_P02)  //待定，未确定是否需要
#define	COM_AT_ENABLE					(GPIO_P12)

#define EVENT_AT_ENABLE_PRESS_ID 		0
#define EVENT_AT_COMMAND_PROC_ID 		1
//#define EVENT_COM_RX_WAKEUP_ID			2
#define EVENT_UART_TX_ID 						3
#define EVENT_UART_RX_FRAME_ID 			4
#define EVENT_UART_RX_TIMEOUT_ID 		5

enum com_st
{
    COM_UART_TX_IDLE = 0,
    COM_UART_TX_ONGOING
};

enum com_conn
{
    COM_DISCONN = 0,
    COM_CONN
};

enum com_mode
{
		COM_MODE_IDLE =0,
  	COM_MODE_TRAN,
		COM_MODE_AT
};

enum LF_STATUS
{
		COM_LF = true,
		COM_NO_LF = false,
};

struct com_env_tag
{
		uint8_t com_conn;
    uint8_t com_mode;
    ///Message id
    uint8_t msg_id;
    
    ///UART TX parameter 
    uint8_t tx_state;       //either transmitting or done.
    struct co_list queue_tx;///Queue of kernel messages corresponding to packets sent through HCI
    struct co_list queue_rx;///Queue of kernel messages corresponding to packets sent through HCI
    
    ///UART RX parameter 
    uint8_t com_rx_len;
		uint8_t com_at_len;
    uint8_t com_rx_buf[QPPS_VAL_CHAR_NUM_MAX*QPP_DATA_MAX_LEN];
    uint8_t com_at_buf[COM_AT_COMM_BUF];
		bool		auto_line_feed;

};

extern void	app_com_rx_wakeup_process(void);
extern void	com_uart_rx_start(void);
extern void com_uart_at_rx_start(void);

extern  struct com_env_tag  com_env;

extern uint32_t get_bit_num(uint32_t val);
extern void com_tx_done(void);
extern void app_com_at_command_handler(void);
extern void com_event_uart_rx_frame_handler(void);
extern int app_com_uart_rx_done_ind_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern int app_com_rx_timeout_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern int app_com_at_rx_enable_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern int app_com_at_baudrate_change_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
extern void com_event_uart_rx_timeout_handler(void);
extern void com_pdu_send(uint8_t len, uint8_t *par);
extern void com_init(void);
extern void app_event_com_rx_wakeup_handler(void);
extern void app_event_at_enable_press_handler(void);
extern void com_wakeup_handler(void);
extern void app_tx_done(void);
extern void show_com_mode(uint8_t com_mode);
#endif
