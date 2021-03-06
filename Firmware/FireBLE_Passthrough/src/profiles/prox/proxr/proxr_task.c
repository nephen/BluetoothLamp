/**
 ****************************************************************************************
 *
 * @file proxr_task.c
 *
 * @brief Proximity Reporter Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PROXRTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_config.h"

#if (BLE_PROX_REPORTER)

#include "gap.h"
#include "gatt_task.h"
#include "atts_util.h"
#include "proxr.h"
#include "proxr_task.h"
#include "attm_cfg.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXR_CREATE_DB_REQ message.
 * The handler adds LLS and optionally TXPS into the database.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct proxr_create_db_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    //Database Creation Status
    uint8_t status;

    //Save Profile ID
    proxr_env.con_info.prf_id = TASK_PROXR;

    /*---------------------------------------------------*
     * Link Loss Service Creation
     *---------------------------------------------------*/

    //Add Service Into Database
    status = atts_svc_create_db(&proxr_env.lls_shdl, NULL, LLS_IDX_NB, NULL,
                               dest_id, &proxr_lls_att_db[0]);
    //Disable LLS
    attsdb_svc_set_permission(proxr_env.lls_shdl, PERM(SVC, DISABLE));

    /*---------------------------------------------------*
     * Immediate Alert and TX Power Services Creation
     *---------------------------------------------------*/

    if ((param->features & PROXR_IAS_TXPS_SUP) == PROXR_IAS_TXPS_SUP)
    {
        //Add IAS Into Database
        status = atts_svc_create_db(&proxr_env.ias_shdl, NULL, IAS_IDX_NB, NULL,
                                   dest_id, &proxr_ias_att_db[0]);
        //Disable IAS
        attsdb_svc_set_permission(proxr_env.ias_shdl, PERM(SVC, DISABLE));

        //Add TXPS Into Database
        status = atts_svc_create_db(&proxr_env.txps_shdl, NULL, TXPS_IDX_NB, NULL,
                                   dest_id, &proxr_txps_att_db[0]);
        //Disable TXPS
        attsdb_svc_set_permission(proxr_env.txps_shdl, PERM(SVC, DISABLE));
    }

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle state
        ke_state_set(TASK_PROXR, PROXR_IDLE);
    }

    //Send CFM to application
    struct proxr_create_db_cfm * cfm = KE_MSG_ALLOC(PROXR_CREATE_DB_CFM, src_id,
                                                    TASK_PROXR, proxr_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Enable the Proximity Reporter role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int proxr_enable_req_handler(ke_msg_id_t const msgid,
                                    struct proxr_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Keep source of message, to respond to it further on
    proxr_env.con_info.appid = src_id;
    // Store the connection handle for which this profile is enabled
    proxr_env.con_info.conhdl = param->conhdl;

    // Check if the provided connection exist
    if (gap_get_rec_idx(param->conhdl) == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&proxr_env, PRF_ERR_REQ_DISALLOWED,
                                  PROXR_ERROR_IND, PROXR_ENABLE_REQ);
    }
    else
    {
        // Enable LLS + Set Security Level
        attsdb_svc_set_permission(proxr_env.lls_shdl, param->sec_lvl);

        //Set LLS Alert Level to specified value
        attsdb_att_set_value(proxr_env.lls_shdl + LLS_IDX_ALERT_LVL_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->lls_alert_lvl);

        if (proxr_env.ias_shdl != ATT_INVALID_HANDLE)
        {
            // Enable IAS + Set Security Level
            attsdb_svc_set_permission(proxr_env.ias_shdl, param->sec_lvl);
        }

        if (proxr_env.txps_shdl != ATT_INVALID_HANDLE)
        {
            //Save TX Power value in DB
            attsdb_att_set_value(proxr_env.txps_shdl + TXPS_IDX_TX_POWER_LVL_VAL,
                                 sizeof(int8_t), (uint8_t *)&(param->txp_lvl));

            // Enable TXPS + Set Security Level
            attsdb_svc_set_permission(proxr_env.txps_shdl, param->sec_lvl);
        }

        // Go to Connected state
        ke_state_set(TASK_PROXR, PROXR_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
 * The handler will analyse what has been set and decide alert level
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gatt_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t char_code = PROXR_ERR_CHAR;
    uint8_t status = PRF_APP_ERROR;

    if (param->conhdl == proxr_env.con_info.conhdl)
    {
        if (param->handle == proxr_env.lls_shdl + LLS_IDX_ALERT_LVL_VAL)
        {
            char_code = PROXR_LLS_CHAR;
        }
        else if (param->handle == proxr_env.ias_shdl + IAS_IDX_ALERT_LVL_VAL)
        {
            char_code = PROXR_IAS_CHAR;
        }

        if (char_code != PROXR_ERR_CHAR)
        {

            // Start alerting if a valid level has been set in LLS
            if (param->value[0] <= PROXR_ALERT_HIGH)
            {
                //Save value in DB
                attsdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&param->value[0]);
                if(param->last)
                {
                    //Inform APP
                    proxr_alert_start(param->value[0], char_code);
                }

                status = PRF_ERR_OK;
            }

            //If LLS, send write response
            if (char_code == PROXR_LLS_CHAR)
            {
                // Send Write Response
                atts_write_rsp_send(proxr_env.con_info.conhdl, param->handle, status);
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to proximity reporter.
 * Alert according to LLS alert level.
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
    atts_size_t length;
    uint8_t *alert_lvl;

    // Check Connection Handle
    if (param->conhdl == proxr_env.con_info.conhdl)
    {
        // Abnormal reason - APP must alert to the level specified in the Alert Level Char.
        if ((param->reason != CO_ERROR_REMOTE_USER_TERM_CON) &&
            (param->reason != CO_ERROR_CON_TERM_BY_LOCAL_HOST))
        {
            //Get Alert Level value stored in DB
            attsdb_att_get_value(proxr_env.lls_shdl + LLS_IDX_ALERT_LVL_VAL,
                                 &length, &alert_lvl);

            //Send it to APP
            proxr_alert_start(*alert_lvl, PROXR_LLS_CHAR);
        }

        // In any case, inform APP about disconnection
        proxr_disable();
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler proxr_disabled[] =
{
    {PROXR_CREATE_DB_REQ,   (ke_msg_func_t) proxr_create_db_req_handler },
};

/// Idle State handler definition.
const struct ke_msg_handler proxr_idle[] =
{
    {PROXR_ENABLE_REQ,      (ke_msg_func_t) proxr_enable_req_handler },
};

/// Connected State handler definition.
const struct ke_msg_handler proxr_connected[] =
{
    {GATT_WRITE_CMD_IND,    (ke_msg_func_t) gatt_write_cmd_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler proxr_default_state[] =
{
    {GAP_DISCON_CMP_EVT,    (ke_msg_func_t) gap_discon_cmp_evt_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler proxr_state_handler[PROXR_STATE_MAX] =
{
    [PROXR_DISABLED]    = KE_STATE_HANDLER(proxr_disabled),
    [PROXR_IDLE]        = KE_STATE_HANDLER(proxr_idle),
    [PROXR_CONNECTED]   = KE_STATE_HANDLER(proxr_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler proxr_default_handler = KE_STATE_HANDLER(proxr_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t proxr_state[PROXR_IDX_MAX];


// Register PROXR task into kernel  
void task_proxr_desc_register(void)
{
    struct ke_task_desc task_proxr_desc;
    
    task_proxr_desc.state_handler = proxr_state_handler;
    task_proxr_desc.default_handler=&proxr_default_handler;
    task_proxr_desc.state = proxr_state;
    task_proxr_desc.state_max = PROXR_STATE_MAX;
    task_proxr_desc.idx_max = PROXR_IDX_MAX;

    task_desc_register(TASK_PROXR, task_proxr_desc);
}
#endif //BLE_PROX_REPORTER

/// @} PROXRTASK
