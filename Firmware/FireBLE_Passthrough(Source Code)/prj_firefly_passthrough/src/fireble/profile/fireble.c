/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_config.h"

#ifdef CFG_PRF_FIREBLE
#include "gap.h"
#include "gatt_task.h"
#include "atts_util.h"
#include "smpc_task.h"
#include "fireble.h"
#include "fireble_task.h"

/*
 * HTPT PROFILE ATTRIBUTES
 ****************************************************************************************
 */

struct fireble_env_tag fireble_env;

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void fireble_init(void)
{
    // Reset environment
    memset(&fireble_env, 0, sizeof(fireble_env));

	// Register fireble task into kernel
    task_fireble_desc_register();

    // Go to IDLE state
    ke_state_set(TASK_FIREBLE, FIREBLE_DISABLED);
}

void fireble_out_ch_data_cfm_send(uint8_t status, uint8_t channelIndex)
{
    // Send CFM to APP that value has been sent or not
    struct fireble_out_ch_send_cfm * cfm = KE_MSG_ALLOC(FIREBLE_SEND_DATA_CFM, fireble_env.con_info.appid,
                                                   TASK_FIREBLE, fireble_out_ch_send_cfm);

    cfm->conhdl = fireble_env.con_info.conhdl;
    cfm->status = status;
	  cfm->channel_index = channelIndex;

    ke_msg_send(cfm);
}


void fireble_disable(void)
{
    //Disable HRS in database
    attsdb_svc_set_permission(fireble_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct fireble_disable_ind *ind = KE_MSG_ALLOC(FIREBLE_DISABLE_IND,
                                                fireble_env.con_info.appid, TASK_FIREBLE,
                                                fireble_disable_ind);

    memcpy(&ind->conhdl, &fireble_env.con_info.conhdl, sizeof(uint16_t));


    ke_msg_send(ind);

    //Go to idle state
    ke_state_set(TASK_FIREBLE, FIREBLE_IDLE);
}

#endif /* CFG_PRF_FIREBLE */

/// @} FIREBLE
