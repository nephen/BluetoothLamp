/**
 ****************************************************************************************
 *
 * @file hogprh_task.c
 *
 * @brief HID Over GATT Profile Report Host Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPRHTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_config.h"

#if (BLE_HID_REPORT_HOST)

#include "gap.h"
#include "attm.h"
#include "hogprh.h"
#include "hogprh_task.h"
#include "gatt_task.h"
#include "hogp_common.h"

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/// State machine used to retrieve HID Service characteristics information
const struct prf_char_def hogprh_hids_char[HOGPRH_CHAR_REPORT + 1] =
{
    /// Report Map
    [HOGPRH_CHAR_REPORT_MAP]             = {ATT_CHAR_REPORT_MAP,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD},
    /// HID Information
    [HOGPRH_CHAR_HID_INFO]               = {ATT_CHAR_HID_INFO,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD},
    /// HID Control Point
    [HOGPRH_CHAR_HID_CTNL_PT]            = {ATT_CHAR_HID_CTNL_PT,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_WR_NO_RESP},
    /// Protocol Mode
    [HOGPRH_CHAR_PROTOCOL_MODE]          = {ATT_CHAR_PROTOCOL_MODE,
                                            ATT_OPTIONAL,
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP},
    /// Report
    [HOGPRH_CHAR_REPORT]                 = {ATT_CHAR_REPORT,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD},
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Report Host Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hogprh_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    // HID Over GATT Profile Client Role Task Environment
    struct hogprh_env_tag *hogprh_env;
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conhdl = param->conhdl;
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, hogprh);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
        hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

        //config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HIDS on peer
            prf_disc_svc_send(&(hogprh_env->con_info), ATT_SVC_HID);

            hogprh_env->last_uuid_req = ATT_SVC_HID;

            // Go to DISCOVERING state
            ke_state_set(dest_id, HOGPRH_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            memcpy(&hogprh_env->hids[0], &param->hids[0], HOGPRH_NB_HIDS_INST_MAX*sizeof(struct hogprh_hids_content));
            hogprh_env->hids_nb = param->hids_nb;

            // Send APP confirmation that can start normal connection to TH
            hogprh_enable_cfm_send(hogprh_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        hogprh_enable_cfm_send(NULL, &con_info, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_DISABLE_REQ message.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_disable_req_handler(ke_msg_id_t const msgid,
                                      struct hogprh_disable_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);
    // Counter
    uint8_t hids_nb;

    for (hids_nb = 0; hids_nb < hogprh_env->hids_nb; hids_nb++)
    {
        // Unregister the profile in the GATT
        prf_unregister_atthdl2gatt(&hogprh_env->con_info, &hogprh_env->hids[hids_nb].svc);
    }

    PRF_CLIENT_DISABLE_IND_SEND(hogprh_envs, dest_id, HOGPRH);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_RD_CHAR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_rd_char_req_handler(ke_msg_id_t const msgid,
                                      struct hogprh_rd_char_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Attribute handle
    uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    // Save the attribute read code
    hogprh_env->last_char_code = param->read_code;

    // Check Connection Handle, HIDS instance and Report Instance
    if ((param->conhdl == hogprh_env->con_info.conhdl) &&
        (param->hids_nb < HOGPRH_NB_HIDS_INST_MAX)    &&
        (param->report_nb < HOGPRH_NB_REPORT_INST_MAX))
    {
        // Descriptor
        if((param->read_code & HOGPRH_DESC_MASK) == HOGPRH_DESC_MASK)
        {
            // If Descriptor is linked to a Report Characteristic (Report Ref or Client Char. Cfg)
            if ((param->read_code == HOGPRH_RD_HIDS_REPORT_REF) || (param->read_code == HOGPRH_RD_WR_HIDS_REPORT_CFG))
            {
                search_hdl = hogprh_env->hids[param->hids_nb].descs[(param->read_code & ~HOGPRH_DESC_MASK) + param->report_nb].desc_hdl;
            }
            // Report Map Char. External Report Reference Descriptor
            else
            {
                search_hdl = hogprh_env->hids[param->hids_nb].descs[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].desc_hdl;
            }
        }
        // Characteristic
        else
        {
            // If Report Char. select the correct instance
            if (param->read_code == HOGPRH_RD_WR_HIDS_REPORT)
            {
                search_hdl = hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_REPORT + param->report_nb].val_hdl;
            }
            else if (param->read_code < HOGPRH_CHAR_REPORT)
            {
                // For characteristics, read_code == char_code
                search_hdl = hogprh_env->hids[param->hids_nb].chars[param->read_code].val_hdl;
            }
        }

        // Check if handle is viable
        if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send Read Request
            prf_read_char_send(&(hogprh_env->con_info), hogprh_env->hids[param->hids_nb].svc.shdl,
                               hogprh_env->hids[param->hids_nb].svc.ehdl, search_hdl);

            // Save the service instance number
            hogprh_env->last_svc_inst_req             = param->hids_nb;
            // Save the report char instance number
            hogprh_env->last_report_char_inst_req     = param->report_nb;
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_RD_CHAR_ERR_RSP, status, hogprh_env->last_char_code);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_CFG_NTF_REQ message.
 * It allows configuration of the peer ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_cfg_ntf_req_handler(ke_msg_id_t const msgid,
                                         struct hogprh_cfg_ntf_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Descriptor handle
    uint16_t cfg_hdl;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    hogprh_env->last_char_code = HOGPRH_RD_WR_HIDS_REPORT_CFG;

    // Check Connection Handle, HIDS instance and Report Instance
    if ((param->conhdl == hogprh_env->con_info.conhdl) &&
        (param->hids_nb < HOGPRH_NB_HIDS_INST_MAX)    &&
        (param->report_nb < HOGPRH_NB_REPORT_INST_MAX))
    {
        // Check value to write
        if((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF))
        {
            // Get descriptor handle
            cfg_hdl = hogprh_env->hids[param->hids_nb].descs[HOGPRH_DESC_REPORT_CFG + param->report_nb].desc_hdl;

            // Check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&hogprh_env->con_info, cfg_hdl, param->ntf_cfg);
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_WR_CHAR_RSP, status, hogprh_env->last_char_code);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_HID_CTNL_PT_WR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_hid_ctnl_pt_wr_req_handler(ke_msg_id_t const msgid,
                                             struct hogprh_hid_ctnl_pt_wr_req const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Characteristic value handle
    uint16_t val_hdl;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    // Check Connection Handle, HIDS instance
    if ((param->conhdl == hogprh_env->con_info.conhdl) &&
        (param->hids_nb < HOGPRH_NB_HIDS_INST_MAX))
    {
        // Check value to write
        if((param->hid_ctnl_pt == HOGP_CTNL_PT_SUSPEND) || (param->hid_ctnl_pt == HOGP_CTNL_PT_EXIT_SUSPEND))
        {
            // Get characteristic value handle
            val_hdl = hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_HID_CTNL_PT].val_hdl;

            // Check if the handle value exists
            if (val_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write(&hogprh_env->con_info, val_hdl,
                               (uint8_t *)&param->hid_ctnl_pt, sizeof(uint8_t), GATT_WRITE_NO_RESPONSE);
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_WR_CHAR_RSP, status, HOGPRH_RD_WR_HID_CTNL_PT);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_SET_REPORT_PROTO_MODE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_set_report_proto_mode_req_handler(ke_msg_id_t const msgid,
                                                    struct hogprh_set_report_proto_mode_req const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id)
{
    // Characteristic value handle
    uint16_t val_hdl;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);
    // Protocol Mode
    uint8_t proto_mode = HOGP_REPORT_PROTOCOL_MODE;

    // Check Connection Handle, HIDS instance
    if ((param->conhdl == hogprh_env->con_info.conhdl) &&
        (param->hids_nb < HOGPRH_NB_HIDS_INST_MAX))
    {
        // Get characteristic value handle
        val_hdl = hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_PROTOCOL_MODE].val_hdl;

        // Check if the handle value exists
        if (val_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Send GATT Write Request
            prf_gatt_write(&hogprh_env->con_info, val_hdl,
                           &proto_mode, sizeof(uint8_t), GATT_WRITE_NO_RESPONSE);
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_WR_CHAR_RSP, status, HOGPRH_RD_WR_HIDS_PROTOCOL_MODE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_PROTO_MODE_WR_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogprh_report_wr_req_handler(ke_msg_id_t const msgid,
                                            struct hogprh_report_wr_req const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    // Characteristic value handle
    uint16_t val_hdl;
    // Status
    uint8_t status = PRF_ERR_OK;
    // Write request type
    uint8_t wr_type;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    hogprh_env->last_char_code = HOGPRH_RD_WR_HIDS_REPORT;

    // Check Connection Handle, HIDS instance and Report instance
    if ((param->conhdl == hogprh_env->con_info.conhdl) &&
        (param->hids_nb < HOGPRH_NB_HIDS_INST_MAX)    &&
        (param->report_nb < HOGPRH_NB_REPORT_INST_MAX))
    {
        // Get report characteristic value handle
        val_hdl = hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_REPORT + param->report_nb].val_hdl;

        // Check if the handle value exists
        if (val_hdl != ATT_INVALID_SEARCH_HANDLE)
        {
            // Check if Char. is writable - Input reports may be unwritable
            if ((hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_REPORT + param->report_nb].prop
                                                & ATT_CHAR_PROP_WR) == ATT_CHAR_PROP_WR)
            {
                // Get MTU to select between GATT_WRITE_LONG_CHAR and GATT_WRITE_CHAR
                if (param->report_length > attm_get_mtu(gap_get_rec_idx(hogprh_env->con_info.conhdl)))
                {
                    wr_type = GATT_WRITE_LONG_CHAR;
                }
                else
                {
                    // Check if it is an Output Report - Output report must support Write and Write Without Response
                    if ((hogprh_env->hids[param->hids_nb].chars[HOGPRH_CHAR_REPORT + param->report_nb].prop
                                                       & ATT_CHAR_PROP_WR_NO_RESP) == ATT_CHAR_PROP_WR_NO_RESP)
                    {
                        // May be GATT_WRITE_CHAR or GATT_WRITE_NO_RESPONSE
                        wr_type  = param->out_report_type;
                    }
                    else
                    {
                        wr_type  = GATT_WRITE_CHAR;
                    }
                }

                // Send GATT Write Request
                prf_gatt_write(&hogprh_env->con_info, val_hdl,
                               (uint8_t *)&param->report[0], param->report_length, wr_type);
            }
            else
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }
        else
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    // If needed, send error indication to APP
    if (status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_WR_CHAR_RSP, status, hogprh_env->last_char_code);
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
    // Counter
    uint8_t i = 0x00;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    if (param->status == PRF_ERR_OK)
    {
        // We can have several instances of HID Service
        for (i = 0; i < (param->nb_resp); i++)
        {
            if (hogprh_env->hids_nb < HOGPRH_NB_HIDS_INST_MAX)
            {
                hogprh_env->hids[hogprh_env->hids_nb].svc.shdl = param->list[i].start_hdl;
                hogprh_env->hids[hogprh_env->hids_nb].svc.ehdl = param->list[i].end_hdl;
            }

            // Update number of found HIDS instances
            hogprh_env->hids_nb++;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_DISC_SVC_INCL_CMP_EVT message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_svc_incl_cmp_evt_handler(ke_msg_id_t const msgid,
                                             struct gatt_disc_svc_incl_cmp_evt const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    // Only save the first included service
    memcpy(&hogprh_env->hids[hogprh_env->last_svc_inst_req].incl_svc, &param->incl_list.list[0], sizeof(struct gatt_incl_list));

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
    //Counters
    uint8_t i, j;
    // Characteristic position
    uint8_t char_pos;
    // Pointer to a characteristic
    struct hogprh_hids_content *hids;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    if (param->status == PRF_ERR_OK)
    {
        //Look over received characteristics
        for (i=0; i<(param->nb_entry); i++)
        {
            //Look over requested characteristics
            for (j=0; j <= HOGPRH_CHAR_REPORT ; j++)
            {
                if(param->list[i].uuid == hogprh_hids_char[j].uuid)
                {
                    // Select position of the characteristic info
                    if (param->list[i].uuid == ATT_CHAR_REPORT)
                    {
                        char_pos = j + hogprh_env->last_report_char_inst_req;
                        // Update number of found report char instances
                        hogprh_env->last_report_char_inst_req++;
                    }
                    else
                    {
                        char_pos = j;
                    }

                    // If too many Report Char are found, they are discarded.
                    if (j < HOGPRH_CHAR_MAX)
                    {
                        hids = &hogprh_env->hids[hogprh_env->last_svc_inst_req];

                        //Save properties and handles
                        hids->chars[char_pos].char_hdl = param->list[i].attr_hdl;
                        hids->chars[char_pos].val_hdl  = param->list[i].pointer_hdl;
                        hids->chars[char_pos].prop     = param->list[i].prop;

                        //Compute number of attribute in Char. using SVC edhl - Limited to 255
                        hids->chars[char_pos].char_ehdl_off = (uint8_t)(hids->svc.ehdl - hids->chars[char_pos].char_hdl + 1);

                        if (char_pos != hogprh_env->last_char_code)
                        {
                            //Update number of attributes for the last found char.
                            hids->chars[hogprh_env->last_char_code].char_ehdl_off
                                = (uint8_t)(hids->chars[char_pos].char_hdl -
                                            hids->chars[hogprh_env->last_char_code].char_hdl);
                        }

                        //Save code of the Char. which has been found.
                        hogprh_env->last_char_code = char_pos;
                    }
                }
            }
        }
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
    // Counters
    uint8_t i;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    //Retrieve characteristic descriptor handle using UUID
    for(i = 0; i< param->nb_entry; i++)
    {
        if (hogprh_env->last_char_code == HOGPRH_CHAR_REPORT_MAP)
        {
            // Report Map Char. External Report Reference Descriptor
            if (param->list[i].desc_hdl == ATT_DESC_EXT_REPORT_REF)
            {
                hogprh_env->hids[hogprh_env->last_svc_inst_req].descs[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].desc_hdl
                                                             = co_read16p(&param->list[i].attr_hdl);
            }
        }
        else
        {
            // Report Char. Report Reference Descriptor
            if (param->list[i].desc_hdl == ATT_DESC_REPORT_REF)
            {
                hogprh_env->hids[hogprh_env->last_svc_inst_req].descs[HOGPRH_DESC_REPORT_REF + hogprh_env->last_report_char_inst_req].desc_hdl
                                                             = co_read16p(&param->list[i].attr_hdl);
            }

            else if (param->list[i].desc_hdl == ATT_DESC_CLIENT_CHAR_CFG)
            {
                if ((param->list[i].attr_hdl > (hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[HOGPRH_CHAR_REPORT + hogprh_env->last_report_char_inst_req].char_hdl + 1)) &&
                    (param->list[i].attr_hdl < (hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[HOGPRH_CHAR_REPORT + hogprh_env->last_report_char_inst_req].char_hdl + 4)))
                {
                hogprh_env->hids[hogprh_env->last_svc_inst_req].descs[HOGPRH_DESC_REPORT_CFG + hogprh_env->last_report_char_inst_req].desc_hdl
                                                             = co_read16p(&param->list[i].attr_hdl);
                }
            }
        }
    }

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
    // Report instance
    uint8_t report_nb;
    // Status
    uint8_t status;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
        (param->status == ATT_ERR_NO_ERROR))
    {
        /*-------------------------------------------------------------------------------------------------------------
         * SERVICES DISCOVERY
         *-------------------------------------------------------------------------------------------------------------*/
        if (hogprh_env->last_uuid_req == ATT_SVC_HID)
        {
            // If no HIDS instance has been found
            if (hogprh_env->hids_nb == 0)
            {
                // Stop discovery procedure.
                hogprh_enable_cfm_send(hogprh_env, &hogprh_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
            }
            else
            {
                hogprh_env->last_svc_inst_req = 0;
                // Discover included service for the first HIDS instance
                prf_disc_incl_svc_send(&hogprh_env->con_info, &hogprh_env->hids[0].svc);
            }
        }

        /*-------------------------------------------------------------------------------------------------------------
         * CHARACTERISTICS DISCOVERY
         *-------------------------------------------------------------------------------------------------------------*/
        else if (hogprh_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
        {
            status = prf_check_svc_char_validity(HOGPRH_CHAR_REPORT + 1, hogprh_env->hids[hogprh_env->last_svc_inst_req].chars,
                                                 hogprh_hids_char);

            if (status == PRF_ERR_OK)
            {
                // Save number of Report Char. that have been found
                hogprh_env->hids[hogprh_env->last_svc_inst_req].report_nb = hogprh_env->last_report_char_inst_req;

                hogprh_env->last_uuid_req  = ATT_INVALID_HANDLE;

                // Discover Report Map Char. Descriptor - Report Map Char. is mandatory
                hogprh_env->last_char_code = HOGPRH_CHAR_REPORT_MAP;

                prf_disc_char_desc_send(&(hogprh_env->con_info),
                                        &(hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[HOGPRH_CHAR_REPORT_MAP]));
            }
            else
            {
                // Stop discovery procedure.
                hogprh_enable_cfm_send(hogprh_env, &hogprh_env->con_info, status);
            }
        }

        /*-------------------------------------------------------------------------------------------------------------
         * DESCRIPTORS DISCOVERY
         *-------------------------------------------------------------------------------------------------------------*/
        else
        {
            if (hogprh_env->last_char_code == HOGPRH_CHAR_REPORT_MAP)
            {
                // External Report Ref descriptor is optional

                // Discover first Report Char. Descriptors - At least one Report Char should be present
                hogprh_env->last_char_code = HOGPRH_CHAR_REPORT;
                hogprh_env->last_report_char_inst_req = 0;

                prf_disc_char_desc_send(&(hogprh_env->con_info),
                                        &(hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[HOGPRH_CHAR_REPORT]));
            }
            else
            {
                // Get report char. instance for which descriptors have been discovered
                report_nb = hogprh_env->last_report_char_inst_req;

                // Check if Report Reference Descriptor is present - Mandatory
                if (hogprh_env->hids[hogprh_env->last_svc_inst_req].descs[HOGPRH_DESC_REPORT_REF + report_nb].desc_hdl != ATT_INVALID_HANDLE)
                {
                    if (report_nb < (HOGPRH_NB_REPORT_INST_MAX-1))
                    {
                        // Select next Report Char instance
                        hogprh_env->last_char_code++;

                        if (hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[hogprh_env->last_char_code].char_hdl != ATT_INVALID_HANDLE)
                        {
                            hogprh_env->last_report_char_inst_req++;

                            prf_disc_char_desc_send(&(hogprh_env->con_info),
                                                    &(hogprh_env->hids[hogprh_env->last_svc_inst_req].chars[hogprh_env->last_char_code]));
                        }
                        else
                        {
                            if (hogprh_env->last_svc_inst_req < (hogprh_env->hids_nb - 1))
                            {
                                // Reset Last Report Char Inst
                                hogprh_env->last_report_char_inst_req = 0;

                                hogprh_env->last_svc_inst_req++;
                                // Discover included service for the next HIDS instance
                                prf_disc_incl_svc_send(&hogprh_env->con_info, &hogprh_env->hids[hogprh_env->last_svc_inst_req].svc);
                            }
                            else
                            {
                                hogprh_enable_cfm_send(hogprh_env, &hogprh_env->con_info, PRF_ERR_OK);
                            }
                        }
                    }
                    else if (report_nb == (HOGPRH_NB_REPORT_INST_MAX - 1))
                    {
                        if (hogprh_env->last_svc_inst_req < (hogprh_env->hids_nb - 1))
                        {
                            // Reset Last Report Char Inst
                            hogprh_env->last_report_char_inst_req = 0;

                            hogprh_env->last_svc_inst_req++;
                            // Discover included service for the next HIDS instance
                            prf_disc_incl_svc_send(&hogprh_env->con_info, &hogprh_env->hids[hogprh_env->last_svc_inst_req].svc);
                        }
                        else
                        {
                            hogprh_enable_cfm_send(hogprh_env, &hogprh_env->con_info, PRF_ERR_OK);
                        }
                    }
                }
                else
                {
                    // Report Reference Descriptor is mandatory
                    hogprh_enable_cfm_send(hogprh_env, &hogprh_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                }
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_DISC_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gatt_disc_cmp_evt_handler(ke_msg_id_t const msgid,
                                     struct gatt_cmp_evt const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    // Discover all characteristic for the current HIDS instance
    hogprh_env->last_uuid_req      = ATT_DECL_CHARACTERISTIC;

    prf_disc_char_all_send(&hogprh_env->con_info, &hogprh_env->hids[hogprh_env->last_svc_inst_req].svc);

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
    uint8_t error_status = param->status;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    if (error_status == PRF_ERR_OK)
    {
        // HID Information Char. value
        if (hogprh_env->last_char_code == HOGPRH_RD_HIDS_HID_INFO)
        {
            struct hogprh_hid_info_rd_rsp *rsp = KE_MSG_ALLOC(HOGPRH_HID_INFO_RD_RSP,
                                                              hogprh_env->con_info.appid, dest_id,
                                                              hogprh_hid_info_rd_rsp);

            rsp->conhdl     = hogprh_env->con_info.conhdl;
            rsp->hids_nb    = hogprh_env->last_svc_inst_req;
            memcpy(&rsp->hid_info, &param->data.data[0], sizeof(struct hids_hid_info));

            ke_msg_send(rsp);
        }

        // Protocol Mode Char. value
        else if (hogprh_env->last_char_code == HOGPRH_RD_WR_HIDS_PROTOCOL_MODE)
        {
            //Check value
            if ((param->data.data[0] == HOGP_BOOT_PROTOCOL_MODE) ||
                (param->data.data[0] == HOGP_REPORT_PROTOCOL_MODE))
            {
                struct hogprh_proto_mode_rd_rsp *rsp = KE_MSG_ALLOC(HOGPRH_PROTO_MODE_RD_RSP,
                                                                    hogprh_env->con_info.appid, dest_id,
                                                                    hogprh_proto_mode_rd_rsp);

                rsp->conhdl         = hogprh_env->con_info.conhdl;
                rsp->hids_nb        = hogprh_env->last_svc_inst_req;
                rsp->proto_mode     = param->data.data[0];

                ke_msg_send(rsp);
            }
            else
            {
                error_status = PRF_APP_ERROR;
            }
        }

        // Report Char. Client Characteristic Configuration value
        else if (hogprh_env->last_char_code == HOGPRH_RD_WR_HIDS_REPORT_CFG)
        {
            struct hogprh_cfg_ntf_rd_rsp *rsp = KE_MSG_ALLOC(HOGPRH_NTF_CFG_RD_RSP,
                                                             hogprh_env->con_info.appid, dest_id,
                                                             hogprh_cfg_ntf_rd_rsp);

            rsp->conhdl     = hogprh_env->con_info.conhdl;
            rsp->hids_nb    = hogprh_env->last_svc_inst_req;
            rsp->report_nb  = hogprh_env->last_report_char_inst_req;
            memcpy(&rsp->cfg_val, &param->data.data[0], sizeof(uint16_t));

            ke_msg_send(rsp);
        }

        // Report Map Char. External Report Reference value
        else if (hogprh_env->last_char_code == HOGPRH_RD_HIDS_REPORT_MAP_EXT_REP_REF)
        {
            struct hogprh_report_map_ext_rep_ref_rd_rsp *rsp = KE_MSG_ALLOC(HOGPRH_REPORT_MAP_EXT_REP_REF_RD_RSP,
                                                                            hogprh_env->con_info.appid, dest_id,
                                                                            hogprh_report_map_ext_rep_ref_rd_rsp);

            rsp->conhdl     = hogprh_env->con_info.conhdl;
            rsp->hids_nb    = hogprh_env->last_svc_inst_req;
            memcpy(&rsp->ext_report_ref, &param->data.data[0], sizeof(uint16_t));

            ke_msg_send(rsp);
        }

        // Report Char. Report Reference value
        else if (hogprh_env->last_char_code == HOGPRH_RD_HIDS_REPORT_REF)
        {
            struct hogprh_report_ref_rd_rsp *rsp = KE_MSG_ALLOC(HOGPRH_REPORT_REP_REF_RD_RSP,
                                                                hogprh_env->con_info.appid, dest_id,
                                                                hogprh_report_ref_rd_rsp);

            rsp->conhdl     = hogprh_env->con_info.conhdl;
            rsp->hids_nb    = hogprh_env->last_svc_inst_req;
            rsp->report_nb  = hogprh_env->last_report_char_inst_req;
            memcpy(&rsp->report_ref, &param->data.data[0], sizeof(struct hids_report_ref));

            ke_msg_send(rsp);
        }

        // Report Map Char. value
        else if (hogprh_env->last_char_code == HOGPRH_RD_HIDS_REPORT_MAP)
        {
            if (param->data.len <= HOGPRH_REPORT_MAP_MAX_LEN)
            {
                struct hogprh_report_map_rd_rsp *rsp = KE_MSG_ALLOC_DYN(HOGPRH_REPORT_MAP_RD_RSP,
                                                                        hogprh_env->con_info.appid, dest_id,
                                                                        hogprh_report_map_rd_rsp,
                                                                        param->data.len);

                rsp->conhdl             = hogprh_env->con_info.conhdl;
                rsp->hids_nb            = hogprh_env->last_svc_inst_req;
                rsp->report_map_length  = param->data.len;
                memcpy(&rsp->report_map[0], &param->data.data[0], param->data.len);

                ke_msg_send(rsp);
                // else ignore this message, the complete received will be received as a Read Long Char. rsp
            }
            else
            {
                error_status = PRF_ERR_UNEXPECTED_LEN;
            }
        }

        // Report Char. value
        else if (hogprh_env->last_char_code == HOGPRH_RD_WR_HIDS_REPORT)
        {
            struct hogprh_report_ind *ind = KE_MSG_ALLOC_DYN(HOGPRH_REPORT_IND,
                                                             hogprh_env->con_info.appid, dest_id,
                                                             hogprh_report_ind,
                                                             param->data.len);

            ind->conhdl             = hogprh_env->con_info.conhdl;
            ind->hids_nb            = hogprh_env->last_svc_inst_req;
            ind->report_nb          = hogprh_env->last_report_char_inst_req;
            ind->ind_type           = HOGPRH_IND_RD_RSP;
            ind->report_length      = param->data.len;
            memcpy(&ind->report[0], &param->data.data[0], param->data.len);

            ke_msg_send(ind);
        }

        //Unsupported Characteristic
        else
        {
            error_status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
    }

    if (error_status != PRF_ERR_OK)
    {
        // Send Write Response to the application
        hogprh_char_req_rsp_send(hogprh_env, HOGPRH_RD_CHAR_ERR_RSP, error_status, hogprh_env->last_char_code);
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
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    // Send Write Response to the application
    hogprh_char_req_rsp_send(hogprh_env, HOGPRH_WR_CHAR_RSP, param->status, hogprh_env->last_char_code);

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
    // HIDS Instance, Report Instance
    uint8_t hids_nb, report_nb;
    // Attribute found
    uint8_t found = PRF_APP_ERROR;
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_CLIENT_GET_ENV(dest_id, hogprh);

    if (param->conhdl == hogprh_env->con_info.conhdl)
    {
        // HIDS instance is unknown.
        for (hids_nb = 0; ((hids_nb < hogprh_env->hids_nb) && (found != PRF_ERR_OK)); hids_nb++)
        {
            for (report_nb = 0; ((report_nb < HOGPRH_NB_REPORT_INST_MAX) && (found != PRF_ERR_OK)); report_nb++)
            {
                // Report Char.
                if (param->charhdl == hogprh_env->hids[hids_nb].chars[HOGPRH_CHAR_REPORT + report_nb].val_hdl)
                {
                    found = PRF_ERR_OK;

                    // Check if size of the data is lower than [ATT_MTU-3]
                    if (param->size < (attm_get_mtu(gap_get_rec_idx(hogprh_env->con_info.conhdl)) - 3))
                    {
                        struct hogprh_report_ind *ind = KE_MSG_ALLOC_DYN(HOGPRH_REPORT_IND,
                                                                         hogprh_env->con_info.appid, dest_id,
                                                                         hogprh_report_ind,
                                                                         param->size);

                        ind->conhdl             = hogprh_env->con_info.conhdl;
                        ind->hids_nb            = hids_nb;
                        ind->report_nb          = report_nb;
                        ind->report_length      = param->size;
                        ind->ind_type           = HOGPRH_IND_NTF;
                        memcpy(&ind->report[0], &param->value[0], param->size);

                        ke_msg_send(ind);
                    }
                    else
                    {
                        // Send a Read Request in order to get the whole report value.
                        prf_read_char_send(&(hogprh_env->con_info), hogprh_env->hids[hids_nb].svc.shdl,
                                           hogprh_env->hids[hids_nb].svc.ehdl, param->charhdl);

                        // Save the service instance number
                        hogprh_env->last_svc_inst_req             = hids_nb;
                        // Save the report char instance number
                        hogprh_env->last_report_char_inst_req     = report_nb;
                        // Save the attribute read code
                        hogprh_env->last_char_code                = HOGPRH_RD_WR_HIDS_REPORT;
                    }
                }
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to HOGPRH.
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
    PRF_CLIENT_DISABLE_IND_SEND(hogprh_envs, dest_id, HOGPRH);

    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Specifies the message handlers for the connected state
const struct ke_msg_handler hogprh_connected[] =
{
    {HOGPRH_DISABLE_REQ,                    (ke_msg_func_t)hogprh_disable_req_handler},
    {HOGPRH_RD_CHAR_REQ,                    (ke_msg_func_t)hogprh_rd_char_req_handler},
    {HOGPRH_CFG_NTF_REQ,                    (ke_msg_func_t)hogprh_cfg_ntf_req_handler},
    {HOGPRH_HID_CTNL_PT_WR_REQ,             (ke_msg_func_t)hogprh_hid_ctnl_pt_wr_req_handler},
    {HOGPRH_SET_REPORT_PROTO_MODE_REQ,      (ke_msg_func_t)hogprh_set_report_proto_mode_req_handler},
    {HOGPRH_REPORT_WR_REQ,                  (ke_msg_func_t)hogprh_report_wr_req_handler},
    {GATT_READ_CHAR_RESP,                   (ke_msg_func_t)gatt_rd_char_rsp_handler},
    {GATT_WRITE_CHAR_RESP,                  (ke_msg_func_t)gatt_write_char_rsp_handler},
    {GATT_HANDLE_VALUE_NOTIF,               (ke_msg_func_t)gatt_handle_value_ntf_handler},
};

/// Specifies the Discovering state message handlers
const struct ke_msg_handler hogprh_discovering[] =
{
    {GATT_DISC_SVC_BY_UUID_CMP_EVT, (ke_msg_func_t)gatt_disc_svc_by_uuid_evt_handler},
    {GATT_DISC_SVC_INCL_CMP_EVT,    (ke_msg_func_t)gatt_disc_svc_incl_cmp_evt_handler},
    {GATT_DISC_CHAR_ALL_CMP_EVT,    (ke_msg_func_t)gatt_disc_char_all_evt_handler},
    {GATT_DISC_CHAR_DESC_CMP_EVT,   (ke_msg_func_t)gatt_disc_char_desc_evt_handler},
    {GATT_CMP_EVT,                  (ke_msg_func_t)gatt_cmp_evt_handler},
    {GATT_DISC_CMP_EVT,             (ke_msg_func_t)gatt_disc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler hogprh_default_state[] =
{
    {HOGPRH_ENABLE_REQ,             (ke_msg_func_t)hogprh_enable_req_handler},
    {GAP_DISCON_CMP_EVT,            (ke_msg_func_t)gap_discon_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler hogprh_state_handler[HOGPRH_STATE_MAX] =
{
    [HOGPRH_IDLE]        = KE_STATE_HANDLER_NONE,
    [HOGPRH_CONNECTED]   = KE_STATE_HANDLER(hogprh_connected),
    [HOGPRH_DISCOVERING] = KE_STATE_HANDLER(hogprh_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler hogprh_default_handler = KE_STATE_HANDLER(hogprh_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t hogprh_state[HOGPRH_IDX_MAX];

// Register HOGPRH task into kernel
void task_hogprh_desc_register(void)
{
    struct ke_task_desc task_hogprh_desc;
    
    task_hogprh_desc.state_handler = hogprh_state_handler;
    task_hogprh_desc.default_handler = &hogprh_default_handler;
    task_hogprh_desc.state = hogprh_state;
    task_hogprh_desc.state_max = HOGPRH_STATE_MAX;
    task_hogprh_desc.idx_max = HOGPRH_IDX_MAX;

    task_desc_register(TASK_HOGPRH, task_hogprh_desc);
}


#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRHTASK
