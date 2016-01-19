#ifndef APP_FIREBLE_TASK_H_
#define APP_FIREBLE_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup APP_FIREBLE_TASK  Task API
 * @ingroup APP_FIREBLE
 * @brief fireble smart watch Profile Task API
 *
 * @{
 ****************************************************************************************
 */

#ifdef CFG_PRF_FIREBLE

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_fireble.h"

/// @cond

struct app_fireble_env_tag
{
    // Profile role state: enabled/disabled
    uint8_t enabled;
    uint8_t features;
    // Connection handle
    uint16_t conhdl;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern struct app_fireble_env_tag *app_fireble_env;
extern unsigned char ble_send_data_flag;
enum ble_send_data_status_flag{
	BLE_SEND_DATA_FINISH=0,		//ble send data finish
	BLE_SEND_DATA_START,		//ble send data start
};
/// @endcond
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/*
 ****************************************************************************************
 * @brief Handles the create database confirmation from the FIREBLE.   
 *
 ****************************************************************************************
 */
int app_fireble_create_db_cfm_handler(ke_msg_id_t const msgid,
                                   struct fireble_create_db_cfm *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id);

/*
 ****************************************************************************************
 * @brief Handles the disable service indication from the FIREBLE. 
 *
 ****************************************************************************************
 */
int app_fireble_disable_ind_handler(ke_msg_id_t const msgid,
                                 struct fireble_disable_ind *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

/*
 ****************************************************************************************
 * @brief Handles the eror indication nessage from the FIREBLE.  
 *
 ****************************************************************************************
 */
int app_fireble_error_ind_handler(ke_msg_id_t const msgid,
                               struct prf_server_error_ind *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id);


int app_fireble_send_data_cfm_handler(ke_msg_id_t const msgid,
                                    struct fireble_out_ch_send_cfm *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id);

int app_fireble_receive_data_ind_handler(ke_msg_id_t const msgid,
                                          struct fireble_in_ch_rec_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id);

#endif // CFG_PRF_FIREBLE

/// @} APP_FIREBLE_TASK

#endif // APP_FIREBLE_TASK_H_





