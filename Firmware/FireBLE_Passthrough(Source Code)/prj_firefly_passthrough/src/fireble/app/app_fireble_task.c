/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "app_env.h"

#ifdef CFG_PRF_FIREBLE
#include "app_fireble.h"
#include "app_fireble_task.h"

/// @cond
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct app_fireble_env_tag *app_fireble_env = &app_env.fireble_ev;

unsigned char ble_send_data_flag=BLE_SEND_DATA_FINISH;
extern unsigned char send_finish_num;
/// @endcond

/*
 ****************************************************************************************
 * @brief Handles the create database confirmation from the FIREBLE.       *//**
 *
 * @param[in] msgid     FIREBLE_CREATE_DB_CFM
 * @param[in] param     struct fireble_create_db_cfm
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_FIREBLE
 *
 * @return If the message was consumed or not.
 * @description
 * This handler will be triggered after a database creation. It contains status of database creation.
 ****************************************************************************************
 */
int app_fireble_create_db_cfm_handler(ke_msg_id_t const msgid,
                                   struct fireble_create_db_cfm *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    if (param->status == ATT_ERR_NO_ERROR)
    {
        app_clear_local_service_flag(BLE_FIREBLE_SERVER_BIT);
    }
    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the disable service indication from the FIREBLE.       *//**
 *
 * @param[in] msgid    FIREBLE_DISABLE_IND
 * @param[in] param     Pointer to the struct fireble_disable_ind
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_FIREBLE
 *
 * @return If the message was consumed or not.
 * @description
 * This handler is used to inform the Application of a correct disable. The configuration
 * that the collector has set in FIREBLE attributes must be conserved and the 4 values that are 
 * important are sent back to the application for safe keeping until the next time this 
 * profile role is enabled.
 ****************************************************************************************
 */
int app_fireble_disable_ind_handler(ke_msg_id_t const msgid,
                                 struct fireble_disable_ind *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    app_fireble_env->conhdl = 0xFFFF;
    app_fireble_env->enabled = false;
    app_task_msg_hdl(msgid, param);
    
    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles the eror indication nessage from the FIREBLE.        *//**
 *
 * @param[in] msgid     FIREBLE_ERROR_IND
 * @param[in] param     Pointer to the struct prf_server_error_ind
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_FIREBLE
 *
 * @return If the message was consumed or not.
 * @description
 * This handler is used to inform the Application of an occurred error.
 ****************************************************************************************
 */
int app_fireble_error_ind_handler(ke_msg_id_t const msgid,
                               struct prf_server_error_ind *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}




/*
 ****************************************************************************************
 * @brief Handles the send output chanel data confirm message from the FIREBLE.     *//**
 *
 * @param[in] msgid     FIREBLE_SEND_SPORT_DATA_CFM
 * @param[in] param     Pointer to the struct fireble_out_ch_send_cfm
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_FIREBLE
 *
 * @return If the message was consumed or not.
 * @description
 * This handler is used to report to the application a confirmation, or error status of a notification
 * request being sent by application.
 ****************************************************************************************
 */
int app_fireble_send_data_cfm_handler(ke_msg_id_t const msgid,
                                    struct fireble_out_ch_send_cfm *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
	app_task_msg_hdl(msgid, param);
	if(param->status == PRF_ERR_OK)
	{
//		QPRINTF("FIREBLE send success %d,%d.\r\n", param->status,param->channel_index);
	}
	else
	{
//		QPRINTF("FIREBLE send error %d,%d.\r\n", param->status,param->channel_index);
	}
	
	fireble_env.out_features |= (0x01 << param->channel_index);
	
    return (KE_MSG_CONSUMED);
}




/*
 ****************************************************************************************
 * @brief Handles the fireble input chanel data receive ind message from the FIREBLE.       *//**
 *
 * @param[in] msgid     FIREBLE_REC_USER_DATA_IND
 * @param[in] param     Pointer to the struct fireble_user_data_rec_ind
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_FIREBLE
 *
 * @return If the message was consumed or not.
 * @description
 * This handler is used to inform application that Energy Expanded value shall be reset.
 ****************************************************************************************
 */
int app_fireble_receive_data_ind_handler(ke_msg_id_t const msgid,
                                          struct fireble_in_ch_rec_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    app_task_msg_hdl(msgid, param);

    return (KE_MSG_CONSUMED);
}

#endif // CFG_PRF_FIREBLE

/// @} APP_fireble_TASK
