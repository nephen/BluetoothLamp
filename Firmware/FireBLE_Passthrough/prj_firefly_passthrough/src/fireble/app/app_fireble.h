#ifndef APP_FIREBLE_H_
#define APP_FIREBLE_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#ifdef CFG_PRF_FIREBLE
#include "fireble.h"
#include "fireble_task.h"
#include "app_fireble_task.h"

#define FIREBLE_TX_NUM_4		4
#define FIREBLE_TX_NUM_13	13
#define BLE_FIREBLE_TX_NUM	FIREBLE_TX_NUM_4

#define BLE_TRANSFER_TEST_EN	0
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Create the fireble service database - at initiation     
 *
 ****************************************************************************************
 */
void app_fireble_create_db(void);

/*
 ****************************************************************************************
 * @brief Start the fireble profile - at connection    
 * 
 ****************************************************************************************
 */
void app_fireble_enable_req(uint16_t conhdl, uint8_t sec_lvl, uint8_t con_type,
                         uint16_t fireble_out_ch_ntf_en, uint8_t out_ch_loc);


/*
 ****************************************************************************************
 * @brief Send fireble out chanel send sport data - at connection 
 *
 ****************************************************************************************
 */
	
#include "app_env.h"
	
typedef enum{
	BLE_SEND_SUCCESS = 0,
	BLE_SEND_WRONG,
	BLE_SEND_DISCONNECT,
	BLE_SEND_TIMEOUT,
	BLE_SEND_ALL
}BLE_SEND_FINISH_TYPE;

typedef enum{
	BLE_SEND_CMD_TYPE = 0,
	BLE_SEND_ACK_TYPE,
}BLE_SEND_TYPE;

typedef struct{
	unsigned char flag; //data is running send ! 
	unsigned char total_package; // all send package numbers !
	unsigned char current_package; // current send package index !
	unsigned char finish; //all package send finish , but not send ok !
	unsigned char timeout;//every send data timerout !
	unsigned short channel_send_flag; //when every channel send one data , so the channel 
									 //bit set 1,when this channel send ok ,
									 //then clear the channel bit !
	unsigned char *buf;//send data buf !
}ble_send_info_struct;

extern void ble_channel_send_data(unsigned char channel,unsigned char *data);

#endif // CFG_PRF_FIREBLE

/// @} APP_fireble_API

#endif // APP_fireble_H_
