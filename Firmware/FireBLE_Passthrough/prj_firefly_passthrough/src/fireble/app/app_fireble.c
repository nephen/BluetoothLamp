/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "app_env.h"

#ifdef CFG_PRF_FIREBLE
#include "app_fireble.h"
#include "app_gap_task.h"
#include "app_fireble_task.h"
#include "usr_design.h"
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Create the heart rate service database - at initiation        *//**
 *
 *
 * @response FIREBLE_CREATE_DB_CFM
 * @description
 * This function shall be send after system power-on (or after GAP Reset) in order to 
 * create heart rate profile database. This database will be visible from a peer device but
 * not usable until app_fireble_enable_req() is called within a BLE connection.
 * @note The Heart Rate profile requires the presence of one DIS characteristic
 ****************************************************************************************
 */
void app_fireble_create_db(void)//,uint8_t* RXUuid,uint8_t* TXUuid)
{
    struct fireble_create_db_req * msg = KE_MSG_ALLOC(FIREBLE_CREATE_DB_REQ, TASK_FIREBLE, TASK_APP, fireble_create_db_req);
    ke_msg_send(msg);
}

/*
 ****************************************************************************************
 * @brief Start the fireble profile - at connection      *//**
 * 
 * @param[in] conhdl Connection handle for which the profile fireble role is enabled.
 * @param[in] sec_lvl Security level required for protection of FIREBLE attributes:
 * Service Hide and Disable are not permitted. Possible values are:
 * - PERM_RIGHT_ENABLE
 * - PERM_RIGHT_UNAUTH
 * - PERM_RIGHT_AUTH
 * @param[in] con_type Connection type: configuration(0) or discovery(1)
 * @param[in] fireble_meas_ntf_en Heart Rate Notification configuration
 * @param[in] body_sensor_loc Body sensor leocation, Possible values are:
 *
 * @response None
 * @description
 * This function is used for enabling the Heart Rate Sensor role of the Heart Rate profile.
 * Before calling this function, a BLE connection shall exist with peer device. 
 * Application shall provide connection handle in order to activate the profile.
 ****************************************************************************************
 */
void app_fireble_enable_req(uint16_t conhdl, uint8_t sec_lvl, uint8_t con_type,
                         uint16_t fireble_out_ch_ntf_en, uint8_t out_ch_loc)
{
    struct fireble_enable_req * msg = KE_MSG_ALLOC(FIREBLE_ENABLE_REQ, TASK_FIREBLE, TASK_APP,
                                                fireble_enable_req);

    msg->conhdl = conhdl;
    msg->sec_lvl = sec_lvl;
    msg->con_type = con_type;
    msg->out_ntf_en = fireble_out_ch_ntf_en;
    msg->out_ch_loc = out_ch_loc;
    ke_msg_send(msg);
}

void ble_channel_send_data(unsigned char len,unsigned char *data)
{
	if(len > 0)
	{
		struct fireble_data_send_req * msgi = KE_MSG_ALLOC(FIREBLE_SEND_DATA_REQ, TASK_FIREBLE, TASK_APP,
																									 fireble_data_send_req);
		unsigned char channel = 0;
		if(len > FIREBLE_SEND_DATA_MAX_LEN)
			len = FIREBLE_SEND_DATA_MAX_LEN;
		if(msgi != NULL )
		{		
				msgi->conhdl = app_fireble_env->conhdl;
				msgi->Channel = fireble_env.shdl+1 + (channel%FirebleTXNum)*3 + 1;
				msgi->channelIndex = channel;
				msgi->len = len;
				memcpy(msgi->outData,data,len);
				ke_msg_send(msgi);
		}
	}
}


#endif // CFG_PRF_FIREBLE

/// @} APP_FIREBLE_API
