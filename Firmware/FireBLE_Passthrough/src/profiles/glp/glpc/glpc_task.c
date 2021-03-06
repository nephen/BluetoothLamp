/**
 ****************************************************************************************
 *
 * @file glpc_task.c
 *
 * @brief Glucose Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup GLPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_config.h"

#if (BLE_GL_COLLECTOR)
#include "gap.h"
#include "attm.h"
#include "glpc.h"
#include "glpc_task.h"
#include "gatt_task.h"
#include "smpc_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve Glucose service characteristics information
const struct prf_char_def glpc_gls_char[GLPC_CHAR_MAX] =
{

    /// Glucose Measurement
    [GLPC_CHAR_MEAS]            = {ATT_CHAR_GLUCOSE_MEAS,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_NTF},
    /// Glucose Measurement Context
    [GLPC_CHAR_MEAS_CTX]        = {ATT_CHAR_GLUCOSE_MEAS_CTX,
                                   ATT_OPTIONAL,
                                   ATT_CHAR_PROP_NTF},
    /// Glucose Feature
    [GLPC_CHAR_FEATURE]         = {ATT_CHAR_GLUCOSE_FEATURE,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_RD},
    /// Record Access control point
    [GLPC_CHAR_RACP]            = {ATT_CHAR_REC_ACCESS_CTRL_PT,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_WR|ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Glucose service characteristic description information
const struct prf_char_desc_def glpc_gls_char_desc[GLPC_DESC_MAX] =
{
     /// Glucose Measurement client config
    [GLPC_DESC_MEAS_CLI_CFG]     = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_MEAS},

        /// Glucose Measurement context client config
    [GLPC_DESC_MEAS_CTX_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_MEAS_CTX},

        /// Record Access control point client config
    [GLPC_DESC_RACP_CLI_CFG]     = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_RACP},
};


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_ENABLE_REQ message.
 * The handler enables the Glucose Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct glpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // Glucose Profile Client Role Task Environment
    struct glpc_env_tag *glpc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conhdl = param->conhdl;
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, glpc);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

        // Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering GLS on peer
            prf_disc_svc_send(&(glpc_env->con_info), ATT_SVC_GLUCOSE);

            glpc_env->last_uuid_req = ATT_SVC_GLUCOSE;

            // Go to DISCOVERING state
            ke_state_set(dest_id, GLPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            glpc_env->gls = param->gls;

            //send APP confirmation that can start normal connection to TH
            glpc_enable_cfm_send(glpc_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        glpc_enable_cfm_send(NULL, &con_info, status);
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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    if (param->status == CO_ERROR_NO_ERROR)
    {
        //even if we get multiple responses we only store 1 range
        if (glpc_env->last_uuid_req == ATT_SVC_GLUCOSE)
        {
            glpc_env->gls.svc.shdl = param->list[0].start_hdl;
            glpc_env->gls.svc.ehdl = param->list[0].end_hdl;
            glpc_env->nb_svc++;
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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    if (param->status == CO_ERROR_NO_ERROR)
    {
        // Retrieve GLS characteristics
        prf_search_chars(glpc_env->gls.svc.ehdl, GLPC_CHAR_MAX,
                         &glpc_env->gls.chars[0], &glpc_gls_char[0],
                         param, &glpc_env->last_char_code);
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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    // Retrieve GLS descriptors
    prf_search_descs(GLPC_DESC_MAX, &glpc_env->gls.descs[0], &glpc_gls_char_desc[0],
                     param, glpc_env->last_char_code);

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
    uint8_t status;
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
        (param->status == ATT_ERR_NO_ERROR))
    {
        // service start/end handles has been received
        if(glpc_env->last_uuid_req == ATT_SVC_GLUCOSE)
        {
            // check if service handles are not ok
            if(glpc_env->gls.svc.shdl == ATT_INVALID_HANDLE)
            {
                // stop discovery procedure.
                glpc_enable_cfm_send(glpc_env, &glpc_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
            }
            //too many services found only one such service should exist
            else if(glpc_env->nb_svc != 1)
            {
                // stop discovery procedure.
                glpc_enable_cfm_send(glpc_env, &glpc_env->con_info, PRF_ERR_MULTIPLE_SVC);
            }
            else
            {
                //discover all GLS characteristics
                prf_disc_char_all_send(&(glpc_env->con_info), &(glpc_env->gls.svc));
                glpc_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
            }
        }
        else if(glpc_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
        {
            status = prf_check_svc_char_validity(GLPC_CHAR_MAX, glpc_env->gls.chars,
                                                 glpc_gls_char);

            // Check for characteristic properties.
            if(status == PRF_ERR_OK)
            {
                glpc_env->last_uuid_req = ATT_INVALID_HANDLE;
                glpc_env->last_char_code = glpc_gls_char_desc[GLPC_DESC_MEAS_CLI_CFG].char_code;

                //Discover Glucose Measurement Char. Descriptor - Mandatory
                prf_disc_char_desc_send(&(glpc_env->con_info),
                                        &(glpc_env->gls.chars[glpc_env->last_char_code]));
            }
            else
            {
                // Stop discovery procedure.
                glpc_enable_cfm_send(glpc_env, &glpc_env->con_info, status);
            }
        }
        else
        {
            //If Glucose Measurement Context Char. found, discover its descriptor
            if ((glpc_env->gls.chars[GLPC_CHAR_MEAS_CTX].char_hdl != ATT_INVALID_HANDLE) &&
                (glpc_env->last_char_code == GLPC_CHAR_MEAS))
            {
                glpc_env->last_char_code = GLPC_CHAR_MEAS_CTX;
                //Discover Intermediate Cuff Pressure Char. Descriptor - Mandatory
                prf_disc_char_desc_send(&(glpc_env->con_info),
                                        &(glpc_env->gls.chars[glpc_env->last_char_code]));
            }
            // discover Record access control point descriptor
            else if (glpc_env->last_char_code != GLPC_CHAR_RACP)
            {
                glpc_env->last_char_code = GLPC_CHAR_RACP;
                //Discover Intermediate Cuff Pressure Char. Descriptor - Mandatory
                prf_disc_char_desc_send(&(glpc_env->con_info),
                        &(glpc_env->gls.chars[glpc_env->last_char_code]));
            }
            else
            {
                status = prf_check_svc_char_desc_validity(GLPC_DESC_MAX,
                                                          glpc_env->gls.descs,
                                                          glpc_gls_char_desc,
                                                          glpc_env->gls.chars);

                glpc_enable_cfm_send(glpc_env, &glpc_env->con_info, status);
            }
        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_READ_FEATURES_REQ message.
 * Send by application task, it's used to retrieve Glucose Sensor features.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glpc_read_features_req_handler(ke_msg_id_t const msgid,
                                          struct glpc_read_features_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    uint8_t status = glpc_validate_request(glpc_env, param->conhdl, GLPC_CHAR_FEATURE);

    // request can be performed
    if(status == PRF_ERR_OK)
    {
        // read glucose sensor feature
        prf_read_char_send(&(glpc_env->con_info),  glpc_env->gls.svc.shdl,
                glpc_env->gls.svc.ehdl,  glpc_env->gls.chars[GLPC_CHAR_FEATURE].val_hdl);
    }
    // send command response with error code
    else
    {
        struct glpc_read_features_rsp * rsp = KE_MSG_ALLOC(GLPC_READ_FEATURES_RSP,
                                                           glpc_env->con_info.appid, dest_id,
                                                           glpc_read_features_rsp);
        // set error status
        rsp->conhdl = param->conhdl;
        rsp->status = status;

        ke_msg_send(rsp);
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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    struct glpc_read_features_rsp * rsp = KE_MSG_ALLOC(GLPC_READ_FEATURES_RSP,
                                                       glpc_env->con_info.appid, dest_id,
                                                       glpc_read_features_rsp);

    // set connection handle
    rsp->conhdl = glpc_env->con_info.conhdl;
    // set error status
    rsp->status = param->status;

    // unpack feature information
    if(param->status == ATT_ERR_NO_ERROR)
    {
        rsp->features = co_read16p(param->data.data);
    }

    ke_msg_send(rsp);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_REGISTER_REQ message.
 * When receiving this request, Glucose collector register to measurement notifications
 * and RACP indications.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glpc_register_req_handler(ke_msg_id_t const msgid,
                                     struct glpc_register_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    // check if collector enabled
    if(param->conhdl == glpc_env->con_info.conhdl)
    {
        // check if client characteristics are present
        if ((glpc_env->gls.descs[GLPC_DESC_MEAS_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)
            || (glpc_env->gls.descs[GLPC_DESC_RACP_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)
            || ((param->meas_ctx_en)
                && (glpc_env->gls.descs[GLPC_DESC_MEAS_CTX_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)))
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if(status == PRF_ERR_OK)
    {
        // register to notification
        prf_gatt_write_ntf_ind(&(glpc_env->con_info),
                                 glpc_env->gls.descs[GLPC_DESC_MEAS_CLI_CFG].desc_hdl,
                                 PRF_CLI_START_NTF);
        glpc_env->last_uuid_req = GLPC_DESC_MEAS_CLI_CFG;

        // save if measurement context notification should be register in an unused variable
        glpc_env->last_char_code = param->meas_ctx_en;
    }
    // inform application that request cannot be performed
    else
    {
        struct glpc_register_cfm * cfm = KE_MSG_ALLOC(GLPC_REGISTER_CFM,
                                                      glpc_env->con_info.appid, dest_id,
                                                      glpc_register_cfm);
        // set error status
        cfm->conhdl = param->conhdl;
        cfm->status = status;

        ke_msg_send(cfm);
    }


    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_RACP_REQ message.
 * When receiving this request, Glucose collector send a RACP command to Glucose sensor.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glpc_racp_req_handler(ke_msg_id_t const msgid,
                                     struct glpc_racp_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    uint8_t status = glpc_validate_request(glpc_env,
                                           param->conhdl,
                                           GLPC_CHAR_RACP);

    if(status == PRF_ERR_OK)
    {
        // send command request
        uint8_t value[GLP_REC_ACCESS_CTRL_MAX_LEN];
        uint16_t length;

        // pack Record Access Control Point request
        length = glpc_pack_racp_req((uint8_t*)&(value[0]), &(param->racp_req));

        // write Record Access Control Point request
        prf_gatt_write(&(glpc_env->con_info),
                         glpc_env->gls.chars[GLPC_CHAR_RACP].val_hdl,
                         &(value[0]),length, GATT_WRITE_CHAR);
        glpc_env->last_uuid_req = ATT_CHAR_REC_ACCESS_CTRL_PT;

        // start the timer; will destroy the link if it expires
        ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
    }
    // inform application that request cannot be performed
    else
    {
        struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                                                  glpc_env->con_info.appid, dest_id,
                                                  glpc_racp_rsp);
        // set error status
        rsp->conhdl = param->conhdl;
        rsp->status = status;

        ke_msg_send(rsp);
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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    // RACP write done.
    if(glpc_env->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
    {
        // an error occurs while writing RACP command
        if(param->status != PRF_ERR_OK)
        {
            struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                                                      glpc_env->con_info.appid, dest_id,
                                                      glpc_racp_rsp);

            // retrieve connection handle
            rsp->conhdl = glpc_env->con_info.conhdl;
            // set error status
            rsp->status = param->status;

            // stop timer.
            ke_timer_clear(GLPC_RACP_REQ_TIMEOUT, dest_id);

            ke_msg_send(rsp);
            glpc_env->last_uuid_req = 0;
        }
    }
    else
    {
        bool finished = false;

        // Restore if measurement context notification should be register in from
        // unused variable
        bool meas_ctx_en = glpc_env->last_char_code;

        // Registration succeed
        if(param->status == PRF_ERR_OK)
        {
            // Glucose measurement  notification registration done
            if(glpc_env->last_uuid_req == GLPC_DESC_MEAS_CLI_CFG)
            {
                // register to RACP indications
                prf_gatt_write_ntf_ind(&(glpc_env->con_info),
                                         glpc_env->gls.descs[GLPC_DESC_RACP_CLI_CFG].desc_hdl,
                                         PRF_CLI_START_IND);
                glpc_env->last_uuid_req = GLPC_DESC_RACP_CLI_CFG;
            }
            // Record access control point indication registration done
            // Register to Glucose Measurement Context notifications if requested.
            else if((glpc_env->last_uuid_req == GLPC_DESC_RACP_CLI_CFG)
                    && meas_ctx_en)
            {
                // register to Glucose Measurement Context notifications
                prf_gatt_write_ntf_ind(&(glpc_env->con_info),
                                         glpc_env->gls.descs[GLPC_DESC_MEAS_CTX_CLI_CFG].desc_hdl,
                                         PRF_CLI_START_NTF);
                glpc_env->last_uuid_req = GLPC_DESC_MEAS_CTX_CLI_CFG;
            }
            // All registration done
            else
            {
                // indication/notification registration finished
                finished = true;
            }
        }

        // send status if registration done or if an error occurs.
        if((param->status != PRF_ERR_OK) || (finished))
        {
            struct glpc_register_cfm * cfm = KE_MSG_ALLOC(GLPC_REGISTER_CFM,
                                                          glpc_env->con_info.appid, dest_id,
                                                          glpc_register_cfm);
            // set error status
            cfm->conhdl = glpc_env->con_info.conhdl;
            cfm->status = param->status;

            ke_msg_send(cfm);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_HANDLE_VALUE_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_handle_value_ind_handler(ke_msg_id_t const msgid,
                                         struct gatt_handle_value_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    // check if collector running
    if(param->conhdl == glpc_env->con_info.conhdl)
    {
        // check if it's a RACP indication
        if (glpc_env->gls.chars[GLPC_CHAR_RACP].val_hdl == param->charhdl)
        {
            struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                                                      glpc_env->con_info.appid, dest_id,
                                                      glpc_racp_rsp);

            // retrieve connection handle
            rsp->conhdl = glpc_env->con_info.conhdl;
            // set error status
            rsp->status = PRF_ERR_OK;

            // stop timer.
            ke_timer_clear(GLPC_RACP_REQ_TIMEOUT, dest_id);

            // unpack RACP response indication.
            glpc_unpack_racp_rsp((uint8_t*)param->value, &(rsp->racp_rsp));
            ke_msg_send(rsp);

            glpc_env->last_uuid_req = 0;
        }

    }

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
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    // check if collector running
    if(param->conhdl == glpc_env->con_info.conhdl)
    {
        // check if it's a Glucose measurement notification
        if (glpc_env->gls.chars[GLPC_CHAR_MEAS].val_hdl == param->charhdl)
        {

            // build a GLPC_MEAS_IND message with glucose measurement value
            struct glpc_meas_ind * ind = KE_MSG_ALLOC(GLPC_MEAS_IND,
                                                      glpc_env->con_info.appid, dest_id,
                                                      glpc_meas_ind);

            // retrieve connection handle
            ind->conhdl = glpc_env->con_info.conhdl;

            // unpack Glucose measurement.
            glpc_unpack_meas_value((uint8_t*) param->value, &(ind->meas_val),
                                   &(ind->seq_num));

            if(glpc_env->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
            {
                // restart the timer; will destroy the link if it expires
                ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
            }

            ke_msg_send(ind);
        }
        // check if it's a Glucose measurement context notification
        else if(glpc_env->gls.chars[GLPC_CHAR_MEAS_CTX].val_hdl == param->charhdl)
        {
            // build a GLPC_MEAS_CTX_IND message with glucose measurement context value
            struct glpc_meas_ctx_ind * ind = KE_MSG_ALLOC(GLPC_MEAS_CTX_IND,
                                                          glpc_env->con_info.appid, dest_id,
                                                          glpc_meas_ctx_ind);

            // retrieve connection handle
            ind->conhdl = glpc_env->con_info.conhdl;

            // unpack Glucose measurement context.
            glpc_unpack_meas_ctx_value((uint8_t*) param->value, &(ind->ctx), &(ind->seq_num));


            if(glpc_env->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
            {
                // restart the timer; will destroy the link if it expires
                ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
            }

            ke_msg_send(ind);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to GLPC.
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
    PRF_CLIENT_DISABLE_IND_SEND(glpc_envs, dest_id, GLPC);

    // Stop timer.
    ke_timer_clear(GLPC_RACP_REQ_TIMEOUT, dest_id);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief RACP request not executed by peer device or is freezed.
 * It can be a connection problem, disconnect the link.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_ATTC).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
static int glpc_racp_req_timeout_handler(ke_msg_id_t const msgid, void const *param,
                                         ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_CLIENT_GET_ENV(dest_id, glpc);

    struct gap_discon_req *discon_req = KE_MSG_ALLOC(GAP_DISCON_REQ,
                                                     TASK_GAP, glpc_env->con_info.appid,
                                                     gap_discon_req);

    discon_req->conhdl = glpc_env->con_info.conhdl;
    discon_req->reason = (uint8_t) CO_ERROR_UNSPECIFIED_ERROR;

    /* send disconnection */
    ke_msg_send(discon_req);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler glpc_connected[] =
{
    {GLPC_READ_FEATURES_REQ,        (ke_msg_func_t)glpc_read_features_req_handler},
    {GATT_READ_CHAR_RESP,           (ke_msg_func_t)gatt_rd_char_rsp_handler},
    {GLPC_REGISTER_REQ,             (ke_msg_func_t)glpc_register_req_handler},
    {GLPC_RACP_REQ,                 (ke_msg_func_t)glpc_racp_req_handler},
    {GATT_WRITE_CHAR_RESP,          (ke_msg_func_t)gatt_write_char_rsp_handler},
    {GATT_HANDLE_VALUE_IND,         (ke_msg_func_t)gatt_handle_value_ind_handler},
    {GATT_HANDLE_VALUE_NOTIF,       (ke_msg_func_t)gatt_handle_value_ntf_handler},
    {GLPC_RACP_REQ_TIMEOUT,         (ke_msg_func_t)glpc_racp_req_timeout_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler glpc_discovering[] =
{
    {GATT_DISC_SVC_BY_UUID_CMP_EVT, (ke_msg_func_t)gatt_disc_svc_by_uuid_evt_handler},
    {GATT_DISC_CHAR_ALL_CMP_EVT,    (ke_msg_func_t)gatt_disc_char_all_evt_handler},
    {GATT_DISC_CHAR_DESC_CMP_EVT,   (ke_msg_func_t)gatt_disc_char_desc_evt_handler},
    {GATT_CMP_EVT,                  (ke_msg_func_t)gatt_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler glpc_default_state[] =
{
    {GLPC_ENABLE_REQ,               (ke_msg_func_t)glpc_enable_req_handler},
    {GAP_DISCON_CMP_EVT,            (ke_msg_func_t)gap_discon_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler glpc_state_handler[GLPC_STATE_MAX] =
{
    [GLPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [GLPC_CONNECTED]   = KE_STATE_HANDLER(glpc_connected),
    [GLPC_DISCOVERING] = KE_STATE_HANDLER(glpc_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler glpc_default_handler = KE_STATE_HANDLER(glpc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t glpc_state[GLPC_IDX_MAX];

// Register GLPC task into kernel
void task_glpc_desc_register(void)
{
    struct ke_task_desc task_glpc_desc;
    
    task_glpc_desc.state_handler = glpc_state_handler;
    task_glpc_desc.default_handler = &glpc_default_handler;
    task_glpc_desc.state = glpc_state;
    task_glpc_desc.state_max = GLPC_STATE_MAX;
    task_glpc_desc.idx_max = GLPC_IDX_MAX;

    task_desc_register(TASK_GLPC, task_glpc_desc);
}

#endif /* (BLE_GL_COLLECTOR) */
/// @} GLPCTASK
