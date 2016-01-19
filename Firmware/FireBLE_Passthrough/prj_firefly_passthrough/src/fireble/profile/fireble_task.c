/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_config.h"
#ifdef CFG_PRF_FIREBLE
#include "gap.h"
#include "gatt_task.h"
#include "atts_util.h"
#include "fireble.h"
#include "fireble_task.h"
#include "prf_utils.h"
#include "serialflash.h"
#include "uart.h"
#include "app_printf.h"
#include "ke_mem.h"
#include "usr_design.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FIREBLE_CREATE_DB_REQ message.
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int fireble_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct fireble_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag
   volatile uint64_t cfg_flag = 0;//FIREBLE_MANDATORY_SUM_MASK;
   volatile uint8_t  idx_nb = 0;//FIREBLE_MANDATORY_SUM_NUM;//base size of data base
    //Database Creation Status
   volatile uint8_t  status;

    //Save Application ID
    fireble_env.appid = src_id;
	
    /*---------------------------------------------------*
     * fireble private Service Creation
     *---------------------------------------------------*/
    int i = 0;
    struct atts_desc *fireble_db = NULL;
    struct atts_char_desc *char_in_desc_def = NULL;
    struct atts_char_desc *char_out_desc_def = NULL;
    atts_svc_desc_t *service_desc_def = NULL;
 
		uint16_t sizeofdb = sizeof(struct atts_desc)+//for service
							FIREBLE_MANDATORY_IN_INCREASE_NUM * (FirebleRXNum) * sizeof(struct atts_desc)+
							FIREBLE_MANDATORY_OUT_INCREASE_NUM * (FirebleTXNum) * sizeof(struct atts_desc);

    fireble_db = (struct atts_desc *)ke_malloc(sizeofdb);

		/* begin create service db */
    service_desc_def = (atts_svc_desc_t *)ke_malloc(sizeof(atts_svc_desc_t));
		service_desc_def[0] = FIREBLE_SVC_PRIVATE_UUID;
		fireble_db[idx_nb].uuid = ATT_DECL_PRIMARY_SERVICE;			
		fireble_db[idx_nb].perm = PERM(RD, ENABLE); 
		fireble_db[idx_nb].max_length =	sizeof(service_desc_def[0]);
		fireble_db[idx_nb].length =	sizeof(service_desc_def[0]);
		fireble_db[idx_nb++].value = (uint8_t*)&service_desc_def[0];
		cfg_flag = 1;
		/* end create service db */


    char_in_desc_def = (struct atts_char_desc *)ke_malloc((FirebleRXNum) * sizeof(struct atts_char_desc));
    char_out_desc_def = (struct atts_char_desc *)ke_malloc((FirebleTXNum) * sizeof(struct atts_char_desc));

    for (i = 0; i < FirebleTXNum; i++)
    {
			const struct atts_char_desc value_char = ATTS_CHAR(ATT_CHAR_PROP_NTF , 0,
													 FIREBLE_DATA_OUT_UUID + i);
					char_out_desc_def[i] = value_char;
			
			fireble_db[idx_nb].uuid = ATT_DECL_CHARACTERISTIC;			
			fireble_db[idx_nb].perm =  PERM(RD, ENABLE);	
			fireble_db[idx_nb].max_length =  sizeof(char_out_desc_def[i]);
			fireble_db[idx_nb].length =  sizeof(char_out_desc_def[i]);
					fireble_db[idx_nb++].value = (uint8_t*)&char_out_desc_def[i];

			fireble_db[idx_nb].uuid = FIREBLE_DATA_OUT_UUID + i;
			fireble_db[idx_nb].perm = PERM(NTF, ENABLE);	
			fireble_db[idx_nb].max_length =  FIREBLE_SEND_DATA_MAX_LEN;
			fireble_db[idx_nb].length =  0;
			fireble_db[idx_nb++].value = NULL;

			fireble_db[idx_nb].uuid = ATT_DESC_CLIENT_CHAR_CFG;
			fireble_db[idx_nb].perm = PERM(RD, ENABLE)|PERM(WR, ENABLE);	
			fireble_db[idx_nb].max_length =  sizeof(uint16_t);
			fireble_db[idx_nb].length =  0;
			fireble_db[idx_nb++].value = NULL;

      cfg_flag = (cfg_flag << FIREBLE_MANDATORY_OUT_INCREASE_NUM) | FIREBLE_MANDATORY_OUT_INCREASE_MASK;
    }
    for (i = 0; i < FirebleRXNum; i++)
    {		
//			const struct atts_char_desc value_char = ATTS_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_WR_NO_RESP, 0,
//																							 FIREBLE_DATA_IN_UUID + i);
			const struct atts_char_desc value_char = ATTS_CHAR(ATT_CHAR_PROP_WR, 0,
																							 FIREBLE_DATA_IN_UUID + i);

			char_in_desc_def[i] = value_char;

			fireble_db[idx_nb].uuid = ATT_DECL_CHARACTERISTIC;			
			fireble_db[idx_nb].perm =  PERM(RD, ENABLE);	
			fireble_db[idx_nb].max_length =  sizeof(char_in_desc_def[i]);
			fireble_db[idx_nb].length =  sizeof(char_in_desc_def[i]);
			fireble_db[idx_nb++].value = (uint8_t*)&char_in_desc_def[i];

			fireble_db[idx_nb].uuid = FIREBLE_DATA_IN_UUID + i;
			fireble_db[idx_nb].perm = PERM(WR, ENABLE);	
			fireble_db[idx_nb].max_length =  FIREBLE_REC_DATA_MAX_LEN;
			fireble_db[idx_nb].length =  0;
			fireble_db[idx_nb++].value = NULL;

      cfg_flag = (cfg_flag << FIREBLE_MANDATORY_IN_INCREASE_NUM) | FIREBLE_MANDATORY_IN_INCREASE_MASK;
    }

    //Add Service Into Database
    status = atts_svc_create_db(&fireble_env.shdl, (uint8_t *)&cfg_flag, idx_nb, NULL,
                               dest_id, &fireble_db[0]);
    ke_free(fireble_db);	
    ke_free(char_in_desc_def);
    ke_free(char_out_desc_def);
	  ke_free(service_desc_def);
		
    if(fireble_env.rx.pdata == NULL)
        fireble_env.rx.pdata = (uint8_t*)ke_malloc(FirebleRXNum * FIREBLE_RX_CHAR_PER_VOLUME);

    if(fireble_env.tx.pdata == NULL)
        fireble_env.tx.pdata = (uint8_t*)ke_malloc(FirebleTXNum * FIREBLE_TX_CHAR_PER_VOLUME);
    //Disable FIREBLE
    attsdb_svc_set_permission(fireble_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_FIREBLE, FIREBLE_IDLE);
    }

    //Send response to application
    struct fireble_create_db_cfm * cfm = KE_MSG_ALLOC(FIREBLE_CREATE_DB_CFM, fireble_env.appid,
                                                   TASK_FIREBLE, fireble_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FIREBLE_ENABLE_REQ message.
 * The handler enables the Heart Rate Sensor Profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int fireble_enable_req_handler(ke_msg_id_t const msgid,
                                   struct fireble_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{

	// Save the application task id
	fireble_env.con_info.appid = src_id;
	// Save the connection handle associated to the profile
	fireble_env.con_info.conhdl = param->conhdl;
	uint16_t value = param->out_ntf_en;

	// Check if the provided connection exist
	if (gap_get_rec_idx(param->conhdl) == GAP_INVALID_CONIDX)
	{
		// The connection doesn't exist, request disallowed
		prf_server_error_ind_send((prf_env_struct *)&fireble_env, PRF_ERR_REQ_DISALLOWED,
		          					FIREBLE_ERROR_IND, FIREBLE_ENABLE_REQ);
	}
	else
	{
		for(int i = 0; i < FirebleTXNum; i++)
		{
			//Set sport data. sleep data. Char. NTF Configuration in DB
			attsdb_att_set_value(fireble_env.shdl+1 + 
								  i*FIREBLE_MANDATORY_OUT_INCREASE_NUM + 2, sizeof(uint16_t),
								  (uint8_t *)&value);		
		}
	
		//  	fireble_env.features= param->out_ntf_en;
		// Enable Service + Set Security Level
		attsdb_svc_set_permission(fireble_env.shdl, param->sec_lvl);

		// Go to connected state
		ke_state_set(TASK_FIREBLE, FIREBLE_CONNECTED);
	}

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FIREBLE_SEND_SPORT_DATA_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int fireble_out_ch_send_data_req_handler(ke_msg_id_t const msgid,
                                      struct fireble_data_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	// Status
	uint8_t status = PRF_ERR_OK;

	if(param->conhdl == fireble_env.con_info.conhdl)
	{
		//Update value in DB
		//fireble_env.shdl + FIREBLE_IDX_OUT_DATA_VAL,
		attsdb_att_set_value(param->Channel,param->len,(uint8_t *)param->outData);
		//send notification through GATT
		struct gatt_notify_req * ntf = KE_MSG_ALLOC(GATT_NOTIFY_REQ, TASK_GATT,TASK_FIREBLE, gatt_notify_req);
		ntf->conhdl  = fireble_env.con_info.conhdl;
		ntf->charhdl = param->Channel;			//fireble_env.shdl + FIREBLE_IDX_OUT_DATA_VAL;
		ke_msg_send(ntf);
		
	}
	else
	{
		status = PRF_ERR_INVALID_PARAM;
	}

	if(status != PRF_ERR_OK)
	{
		// Value has not been sent
		fireble_out_ch_data_cfm_send(status,param->channelIndex);
	}

	return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
extern int at_process_command(const uint8_t* const pcCommandInput,uint8_t* pcWriteBuffer);
extern void com_pdu_send(uint8_t len, uint8_t *par);
extern uint8_t at_command_return[];
uint8_t fireble_at_buf_len = 0;
uint8_t fireble_at_buf[40];

void frieble_at_command()
{
	if(fireble_at_buf_len >= 4)
	{
			if(fireble_at_buf[0] == 'A' && fireble_at_buf[1] == 'T')
			{
				if(fireble_at_buf_len == 4)
				{
					ble_channel_send_data(sprintf((char *)at_command_return,"OK\r\n"),at_command_return);
				}
				else
				{
					if(fireble_at_buf[2] == '+')
					{
						fireble_at_buf[fireble_at_buf_len-2] = '\0';
						//com_pdu_send(fireble_at_buf_len,fireble_at_buf);
						ble_channel_send_data(at_process_command(fireble_at_buf + 2,at_command_return),at_command_return);
					}
					else
					{
						ble_channel_send_data(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
					}
				}					
			}
			else
			{
				ble_channel_send_data(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
			}
	}
	else
	{
		ble_channel_send_data(sprintf((char *)at_command_return,"AT ERR\r\n"),at_command_return);
	}
}

static int gatt_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gatt_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	uint8_t status = PRF_ERR_OK;
	if (param->conhdl == fireble_env.con_info.conhdl)
	{
		ASSERT_ERR(param->length <= FIREBLE_RX_CHAR_PER_VOLUME);
		if(param->handle < (fireble_env.shdl+ 1 + FirebleTXNum*FIREBLE_MANDATORY_OUT_INCREASE_NUM + 1))
		{
        // Client Char. Configuration
				uint8_t char_index = param->handle - (fireble_env.shdl + FIREBLE_IDX_OUT_DATA_NTF_CFG);
        if ((param->handle > (fireble_env.shdl + FIREBLE_IDX_OUT_DATA_VAL)) && ((char_index % 3) == 0))
        {
            uint16_t value = 0x0000;

            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if ((value == PRF_CLI_STOP_NTFIND) || (value == PRF_CLI_START_NTF))
            {
							if (value == PRF_CLI_STOP_NTFIND)
							{
									fireble_env.features &= ~(0x01 << (char_index / FIREBLE_MANDATORY_OUT_INCREASE_NUM));
							}
							else //PRF_CLI_START_NTF
							{
									fireble_env.features     |= 0x01 << (char_index / FIREBLE_MANDATORY_OUT_INCREASE_NUM);
									fireble_env.out_features |= 0x01 << (char_index / FIREBLE_MANDATORY_OUT_INCREASE_NUM);
							}
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == PRF_ERR_OK)
            {
								//Update the attribute value
								attsdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&value);
            }
        }		   
		}
		else if(param->handle >= (fireble_env.shdl+ 1 + FirebleTXNum*FIREBLE_MANDATORY_OUT_INCREASE_NUM + 1))
		{
				//com_pdu_send(param->length,(uint8_t*)&(param->value));
				if(fireble_env.features != 0 )
				{
					if(param->length != 00 )
					{
						memcpy(fireble_at_buf + fireble_at_buf_len, (uint8_t*)&(param->value), param->length);
						fireble_at_buf_len +=param->length;
					}
					if(fireble_at_buf[fireble_at_buf_len-2] == '\r' && fireble_at_buf[fireble_at_buf_len-1] == '\n')
					{
						frieble_at_command();
						fireble_at_buf_len = 0;
					}
				}
		}
	}
	
	if (param->response)
	{
			//Send write response
			atts_write_rsp_send(fireble_env.con_info.conhdl, param->handle, status);
	}	
	return (KE_MSG_CONSUMED);
}






/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that Measurement notification
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
 * Convey this information to appli task using @ref FIREBLE_MEAS_SEND_CFM
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_notify_cmp_evt_handler(ke_msg_id_t const msgid,
                                       struct gatt_notify_cmp_evt const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    fireble_out_ch_data_cfm_send(param->status,(param->handle - (fireble_env.shdl + FIREBLE_IDX_OUT_DATA_VAL)) / 3);  //????
	  QPRINTF("gatt notify cmp status:%d\r\n",param->status);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to FIREBLE.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gap_discon_cmp_evt_handler(ke_msg_id_t const msgid,
                                        struct gap_discon_cmp_evt const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

		//Check Connection Handle
		if (param->conhdl == fireble_env.con_info.conhdl)
		{
				fireble_disable();
		}

		QPRINTF("gap_discon_cmp_evt_handler\r\n");
		return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
static const struct ke_msg_handler fireble_disabled[] =
{
    {FIREBLE_CREATE_DB_REQ,       (ke_msg_func_t) fireble_create_db_req_handler}
};

/// Idle State handler definition.
static const struct ke_msg_handler fireble_idle[] =
{
    {FIREBLE_ENABLE_REQ,       (ke_msg_func_t) fireble_enable_req_handler}
};

/// Connected State handler definition.
static const struct ke_msg_handler fireble_connected[] =
{
    {FIREBLE_SEND_DATA_REQ,   	(ke_msg_func_t) fireble_out_ch_send_data_req_handler},
    {GATT_WRITE_CMD_IND,    		(ke_msg_func_t) gatt_write_cmd_ind_handler},
    {GATT_NOTIFY_CMP_EVT,  			(ke_msg_func_t) gatt_notify_cmp_evt_handler},
};

/* Default State handlers definition. */
const struct ke_msg_handler fireble_default_state[] =
{
    {GAP_DISCON_CMP_EVT,         (ke_msg_func_t)gap_discon_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler fireble_state_handler[FIREBLE_STATE_MAX] =
{
    [FIREBLE_DISABLED]       = KE_STATE_HANDLER(fireble_disabled),
    [FIREBLE_IDLE]           = KE_STATE_HANDLER(fireble_idle),
    [FIREBLE_CONNECTED]      = KE_STATE_HANDLER(fireble_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler fireble_default_handler = KE_STATE_HANDLER(fireble_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t fireble_state[FIREBLE_IDX_MAX];

// Register FIREBLE task into kernel
void task_fireble_desc_register(void)
{
    struct ke_task_desc  task_fireble_desc;
    
    task_fireble_desc.state_handler = fireble_state_handler;
    task_fireble_desc.default_handler=&fireble_default_handler;
    task_fireble_desc.state = fireble_state;
    task_fireble_desc.state_max = FIREBLE_STATE_MAX;
    task_fireble_desc.idx_max = FIREBLE_IDX_MAX;

    task_desc_register(TASK_FIREBLE, task_fireble_desc);
}


#endif /* #ifdef CFG_PRF_FIREBLE */


/// @} FIREBLETASK
