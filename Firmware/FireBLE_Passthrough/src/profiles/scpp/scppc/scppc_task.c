/**
 ****************************************************************************************
 *
 * @file scppc_task.c
 *
 * @brief Scan Parameters Profile Client Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup SCPPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_config.h"

#if (BLE_SP_CLIENT)

#include "gap.h"
#include "attm.h"
#include "scppc.h"
#include "scppc_task.h"
#include "gatt_task.h"
#include "scpp_common.h"
#include "prf_types.h"

/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// State machine used to retrieve Scan Parameters Service characteristics information
const struct prf_char_def scppc_scps_char[SCPPC_CHAR_MAX] =
{
    /// Scan Interval Window
    [SCPPC_CHAR_SCAN_INTV_WD]      = {ATT_CHAR_SCAN_INTV_WD,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_WR_NO_RESP},
    /// Scan Refresh
    [SCPPC_CHAR_SCAN_REFRESH]      = {ATT_CHAR_SCAN_REFRESH,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_NTF},
};

/// State machine used to retrieve Scan Parameters Service characteristic description information
const struct prf_char_desc_def scppc_scps_char_desc[SCPPC_DESC_MAX] =
{
    /// Boot Keyboard Input Report Client Config
    [SCPPC_DESC_SCAN_REFRESH_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, SCPPC_CHAR_SCAN_REFRESH},
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SCPPC_ENABLE_REQ message.
 * The handler enables the Scan Parameters Profile Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int scppc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct scppc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Scan Parameters Profile Client Role Task Environment
    struct scppc_env_tag *scppc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conhdl = param->conhdl;
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status = PRF_CLIENT_ENABLE(con_info, param, scppc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

        // Save Scan Interval Window value
        memcpy(&scppc_env->scan_intv_wd, &param->scan_intv_wd, sizeof(struct scan_intv_wd));

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering SCPS on peer
            prf_disc_svc_send(&(scppc_env->con_info), ATT_SVC_SCAN_PARAMETERS);

            scppc_env->last_uuid_req = ATT_SVC_SCAN_PARAMETERS;

            // Go to DISCOVERING state
            ke_state_set(dest_id, SCPPC_DISCOVERING);
        }
        // Normal connection, get saved att details
        else
        {
            scppc_env->scps = param->scps;

            if (scppc_env->scps.chars[SCPPC_CHAR_SCAN_REFRESH].char_hdl != ATT_INVALID_HANDLE)
            {
                /* If connected to a bonded Scan Server, the Scan Client shall enable notifications of the
                 * Scan Refresh characteristic using the Client Characteristic Configuration descriptor.
                 */
                struct scppc_scan_refresh_ntf_cfg_req *req = KE_MSG_ALLOC(SCPPC_SCAN_REFRESH_NTF_CFG_REQ,
                                                                          dest_id, dest_id,
                                                                          scppc_scan_refresh_ntf_cfg_req);

                req->conhdl    = scppc_env->con_info.conhdl;
                req->ntf_cfg    = PRF_CLI_START_NTF;

                // send the message
                ke_msg_send(req);
            }
            else
            {
                // Write Scan Interval Windows value
                struct scppc_scan_intv_wd_wr_req * req = KE_MSG_ALLOC(SCPPC_SCAN_INTV_WD_WR_REQ,
                                                                      dest_id, dest_id,
                                                                      scppc_scan_intv_wd_wr_req);

                req->conhdl = scppc_env->con_info.conhdl;

                co_write16p(&req->scan_intv_wd.le_scan_intv, scppc_env->scan_intv_wd.le_scan_intv);
                co_write16p(&req->scan_intv_wd.le_scan_window, scppc_env->scan_intv_wd.le_scan_window);

                ke_msg_send(req);
            }


            scppc_enable_cfm_send(scppc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        // An error has been raised during the process, scps is NULL and won't be handled
        scppc_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SCPPC_SCAN_INTV_WD_WR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int scppc_scan_intv_wd_wr_req_handler(ke_msg_id_t const msgid,
                                             struct scppc_scan_intv_wd_wr_req const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (param->conhdl == scppc_env->con_info.conhdl)
    {
       if (scppc_env->scps.chars[SCPPC_CHAR_SCAN_INTV_WD].char_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
           if (src_id == scppc_env->con_info.appid)
           {
               // Save the last written value
               co_write16p(&scppc_env->scan_intv_wd.le_scan_intv, param->scan_intv_wd.le_scan_intv);
               co_write16p(&scppc_env->scan_intv_wd.le_scan_window, param->scan_intv_wd.le_scan_window);
           }

           // Send GATT Write Request
           prf_gatt_write(&scppc_env->con_info, scppc_env->scps.chars[SCPPC_CHAR_SCAN_INTV_WD].val_hdl,
                          (uint8_t *)&param->scan_intv_wd, sizeof(struct scan_intv_wd), GATT_WRITE_NO_RESPONSE);
        }
        //send app error indication for inexistent handle for this characteristic
        else
        {
            scppc_error_ind_send(scppc_env, PRF_ERR_INEXISTENT_HDL);
        }
    }
    else
    {
        scppc_error_ind_send(scppc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SCPPC_SCAN_REFRESH_NTF_CFG_RD_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int scppc_scan_refresh_ntf_cfg_rd_req_handler(ke_msg_id_t const msgid,
                                                     struct scppc_scan_refresh_ntf_cfg_rd_req const *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (param->conhdl == scppc_env->con_info.conhdl)
    {
        scppc_env->last_char_code = SCPPC_DESC_SCAN_REFRESH_CFG;

        prf_read_char_send(&(scppc_env->con_info),
                           scppc_env->scps.svc.shdl,
                           scppc_env->scps.svc.ehdl,
                           scppc_env->scps.descs[SCPPC_DESC_SCAN_REFRESH_CFG].desc_hdl);
    }
    else
    {
    	scppc_error_ind_send(scppc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref SCPPC_SCAN_REFRESH_NTF_CFG_REQ message.
 * It allows configuration of the peer ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int scppc_scan_refresh_ntf_cfg_req_handler(ke_msg_id_t const msgid,
                                                  struct scppc_scan_refresh_ntf_cfg_req const *param,
                                                  ke_task_id_t const dest_id,
                                                  ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = 0x0000;
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if(param->conhdl == scppc_env->con_info.conhdl)
    {
        cfg_hdl = scppc_env->scps.descs[SCPPC_DESC_SCAN_REFRESH_CFG].desc_hdl;

        //check value to write
        if(!((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF)))
        {
            scppc_error_ind_send(scppc_env, PRF_ERR_INVALID_PARAM);
        }
        else
        {
            //check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&scppc_env->con_info, cfg_hdl, param->ntf_cfg);
            }
            else
            {
                scppc_error_ind_send(scppc_env, PRF_ERR_INEXISTENT_HDL);
            }
        }
    }
    else
    {
        scppc_error_ind_send(scppc_env, PRF_ERR_INVALID_PARAM);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_DISC_SVC_BY_UUID_CMP_EVT message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_svc_by_uuid_evt_handler(ke_msg_id_t const msgid,
                                             struct gatt_disc_svc_by_uuid_cmp_evt const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (param->status == PRF_ERR_OK)
    {
        // Even if we get multiple responses we only store 1 range
        if (scppc_env->last_uuid_req == ATT_SVC_SCAN_PARAMETERS)
        {
            scppc_env->scps.svc.shdl = param->list[0].start_hdl;
            scppc_env->scps.svc.ehdl = param->list[0].end_hdl;
            scppc_env->nb_svc++;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_DISC_CHAR_ALL_CMP_EVT message.
 * Characteristics for the currently desired service handle range are obtained and kept.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_char_all_evt_handler(ke_msg_id_t const msgid,
                                          struct gatt_disc_char_all_cmp_evt const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (param->status == PRF_ERR_OK)
    {
        // Retrieve SPCS characteristics
        prf_search_chars(scppc_env->scps.svc.ehdl, SCPPC_CHAR_MAX,
                         &scppc_env->scps.chars[0], &scppc_scps_char[0],
                         param, &scppc_env->last_char_code);
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_DISC_CHAR_DESC_CMP_EVT message.
 * This event can be received several times
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_char_desc_evt_handler(ke_msg_id_t const msgid,
                                           struct gatt_disc_char_desc_cmp_evt const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    // Retrieve SCPS descriptors
    prf_search_descs(SCPPC_DESC_MAX, &scppc_env->scps.descs[0], &scppc_scps_char_desc[0],
                     param, scppc_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gatt_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    uint8_t status = PRF_ERR_OK;

    if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
        (param->status == ATT_ERR_NO_ERROR))
    {
        // Service start/end handles has been received
        if(scppc_env->last_uuid_req == ATT_SVC_SCAN_PARAMETERS)
        {
            // check if service handles are not ok
            if(scppc_env->scps.svc.shdl== ATT_INVALID_HANDLE)
            {
                // stop discovery procedure.
                scppc_enable_cfm_send(scppc_env, &scppc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
            }
            // Too many services found only one such service should exist
            else if(scppc_env->nb_svc > 1)
            {
                scppc_enable_cfm_send(scppc_env, &scppc_env->con_info, PRF_ERR_MULTIPLE_SVC);
            }
            else
            {
                // Discover all SCPS characteristics
                prf_disc_char_all_send(&(scppc_env->con_info), &(scppc_env->scps.svc));
                scppc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
            }
        }
        else if(scppc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
        {
            status = prf_check_svc_char_validity(SCPPC_CHAR_MAX, scppc_env->scps.chars,
                                                 scppc_scps_char);

            if(status == PRF_ERR_OK)
            {
                //If Scan Refresh Char. is present, discover its descriptor
                if (scppc_env->scps.chars[SCPPC_CHAR_SCAN_REFRESH].char_hdl != ATT_INVALID_HANDLE)
                {
                    scppc_env->last_uuid_req = ATT_INVALID_HANDLE;
                    scppc_env->last_char_code = scppc_scps_char_desc[SCPPC_DESC_SCAN_REFRESH_CFG].char_code;

                    // Discover Scan Refresh Char. Descriptor - Mandatory
                    prf_disc_char_desc_send(&(scppc_env->con_info),
                                            &(scppc_env->scps.chars[scppc_env->last_char_code]));
                }
                else
                {
                    scppc_enable_cfm_send(scppc_env, &scppc_env->con_info, status);
                }
            }
            else
            {
                // Stop discovery procedure.
                scppc_enable_cfm_send(scppc_env, &scppc_env->con_info, status);
            }
        }
        else
        {
            status = prf_check_svc_char_desc_validity(SCPPC_DESC_MAX,
                                                      scppc_env->scps.descs,
                                                      scppc_scps_char_desc,
                                                      scppc_env->scps.chars);

            scppc_enable_cfm_send(scppc_env, &scppc_env->con_info, status);
        }

        if (status == PRF_ERR_OK)
        {
            // Write Scan Interval Windows value
            struct scppc_scan_intv_wd_wr_req * req = KE_MSG_ALLOC(SCPPC_SCAN_INTV_WD_WR_REQ,
                                                                  dest_id, dest_id,
                                                                  scppc_scan_intv_wd_wr_req);

            req->conhdl = scppc_env->con_info.conhdl;

            co_write16p(&req->scan_intv_wd.le_scan_intv, scppc_env->scan_intv_wd.le_scan_intv);
            co_write16p(&req->scan_intv_wd.le_scan_window, scppc_env->scan_intv_wd.le_scan_window);

            ke_msg_send(req);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_READ_CHAR_RESP message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_rd_char_rsp_handler(ke_msg_id_t const msgid,
                                    struct gatt_read_char_resp const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (scppc_env->last_char_code == SCPPC_DESC_SCAN_REFRESH_CFG)
    {
        struct scppc_scan_refresh_ntf_cfg_rd_rsp *rsp = KE_MSG_ALLOC(SCPPC_SCAN_REFRESH_NTF_CFG_RD_RSP,
                                                                     scppc_env->con_info.appid, dest_id,
                                                                     scppc_scan_refresh_ntf_cfg_rd_rsp);

        rsp->conhdl = scppc_env->con_info.conhdl;
        memcpy(&rsp->ntf_cfg, &param->data.data[0], sizeof(uint16_t));

        ke_msg_send(rsp);
    }
    // Unsupported Characteristic
    else
    {
        scppc_error_ind_send(scppc_env, PRF_ERR_FEATURE_NOT_SUPPORTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CHAR_RESP message.
 * Generic event received after every simple write command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_write_char_rsp_handler(ke_msg_id_t const msgid,
                                       struct gatt_write_char_resp const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    struct scppc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(SCPPC_WR_CHAR_RSP,
                                                    scppc_env->con_info.appid, dest_id,
                                                    scppc_wr_char_rsp);

    wr_cfm->conhdl    = scppc_env->con_info.conhdl;
    //it will be a GATT status code
    wr_cfm->status    = param->status;

    // send the message
    ke_msg_send(wr_cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_HANDLE_VALUE_NTF message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_handle_value_ntf_handler(ke_msg_id_t const msgid,
                                        struct gatt_handle_value_notif const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct scppc_env_tag *scppc_env = PRF_CLIENT_GET_ENV(dest_id, scppc);

    if (param->conhdl == scppc_env->con_info.conhdl)
    {
        //Scan Refresh
        if (param->charhdl == scppc_env->scps.chars[SCPPC_CHAR_SCAN_REFRESH].val_hdl)
        {
            if (param->value[0] == SCPP_SERVER_REQUIRES_REFRESH)
            {
                // Rewrite the most recent settings written on the server
                struct scppc_scan_intv_wd_wr_req * req = KE_MSG_ALLOC(SCPPC_SCAN_INTV_WD_WR_REQ,
                                                                      dest_id, dest_id,
                                                                      scppc_scan_intv_wd_wr_req);

                req->conhdl = param->conhdl;

                co_write16p(&req->scan_intv_wd.le_scan_intv, scppc_env->scan_intv_wd.le_scan_intv);
                co_write16p(&req->scan_intv_wd.le_scan_window, scppc_env->scan_intv_wd.le_scan_window);

                ke_msg_send(req);
            }
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to SCPPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(scppc_envs, dest_id, SCPPC);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler scppc_connected[] =
{
    {SCPPC_SCAN_INTV_WD_WR_REQ,             (ke_msg_func_t)scppc_scan_intv_wd_wr_req_handler},
    {SCPPC_SCAN_REFRESH_NTF_CFG_RD_REQ,     (ke_msg_func_t)scppc_scan_refresh_ntf_cfg_rd_req_handler},
    {SCPPC_SCAN_REFRESH_NTF_CFG_REQ,        (ke_msg_func_t)scppc_scan_refresh_ntf_cfg_req_handler},
    {GATT_HANDLE_VALUE_NOTIF,               (ke_msg_func_t)gatt_handle_value_ntf_handler},
    {GATT_READ_CHAR_RESP,                   (ke_msg_func_t)gatt_rd_char_rsp_handler},
    {GATT_WRITE_CHAR_RESP,                  (ke_msg_func_t)gatt_write_char_rsp_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler scppc_discovering[] =
{
    {GATT_DISC_SVC_BY_UUID_CMP_EVT,         (ke_msg_func_t)gatt_disc_svc_by_uuid_evt_handler},
    {GATT_DISC_CHAR_ALL_CMP_EVT,            (ke_msg_func_t)gatt_disc_char_all_evt_handler},
    {GATT_DISC_CHAR_DESC_CMP_EVT,           (ke_msg_func_t)gatt_disc_char_desc_evt_handler},
    {GATT_CMP_EVT,                          (ke_msg_func_t)gatt_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler scppc_default_state[] =
{
    {SCPPC_ENABLE_REQ,                      (ke_msg_func_t)scppc_enable_req_handler},
    {GAP_DISCON_CMP_EVT,                    (ke_msg_func_t)gap_discon_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler scppc_state_handler[SCPPC_STATE_MAX] =
{
    [SCPPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [SCPPC_CONNECTED]   = KE_STATE_HANDLER(scppc_connected),
    [SCPPC_DISCOVERING] = KE_STATE_HANDLER(scppc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler scppc_default_handler = KE_STATE_HANDLER(scppc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t scppc_state[SCPPC_IDX_MAX];

// Registet SCPPC task into kernel
void task_scppc_desc_register(void)
{
    struct ke_task_desc task_scppc_desc;
    
    task_scppc_desc.state_handler = scppc_state_handler;
    task_scppc_desc.default_handler = &scppc_default_handler;
    task_scppc_desc.state = scppc_state;
    task_scppc_desc.state_max = SCPPC_STATE_MAX;
    task_scppc_desc.idx_max = SCPPC_IDX_MAX;
    
    task_desc_register(TASK_SCPPC, task_scppc_desc);
}

#endif /* (BLE_SP_CLIENT) */
/// @} SCPPCTASK
