/**
 ****************************************************************************************
 *
 * @file qpps.c
 *
 * @brief Quintic Private Profile Server implementation.
 *
 * Copyright (C) Quintic 2013-2014
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup QPP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_config.h"

#if BLE_QPP_SERVER
#include "gap.h"
#include "gatt_task.h"
#include "atts_util.h"
#include "smpc_task.h"
#include "qpps.h"
#include "qpps_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Full QPP Database Description - Used to add attributes into the database
const struct atts_desc qpps_att_db[QPPS_IDX_NB] =
{
    // Service Declaration
    [QPPS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(qpps_svc),
                                         sizeof(qpps_svc), (uint8_t *)&qpps_svc},

    // Received data Characteristic Declaration
    [QPPS_IDX_RX_DATA_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(qpps_char_rx_data),
                                         sizeof(qpps_char_rx_data), (uint8_t *)&qpps_char_rx_data},
    // Received data Characteristic Value
    [QPPS_IDX_RX_DATA_VAL]         =   {QPPS_RX_CHAR_UUID, PERM(WR, ENABLE), QPP_DATA_MAX_LEN,
                                         0, NULL},
    // User Descriptor
    [QPPS_IDX_RX_DATA_USER_DESP]   =   {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), sizeof("15.08.1"),
                                         sizeof("15.08.1"), (uint8_t *)"15.08.1"},

    // Tx data to client with these Characters

    // Characteristic Declaration
    [QPPS_IDX_VAL_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(qpps_value_char),
                                         sizeof(qpps_value_char), (uint8_t *)&qpps_value_char},
    // Characteristic Value
    [QPPS_IDX_VAL]                  =   {QPPS_FIRST_TX_CHAR_UUID, PERM(NTF, ENABLE), QPP_DATA_MAX_LEN,
                                         0, NULL},
    // Client Characteristic Configuration Descriptor
    [QPPS_IDX_VAL_NTF_CFG]          =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                         0, NULL},
};

/*
 * PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Server Service
const atts_svc_desc_t qpps_svc = QPP_SVC_PRIVATE_UUID;

/// Server Service - Server value Characteristic
const struct atts_char_desc qpps_value_char = ATTS_CHAR(ATT_CHAR_PROP_NTF,
                                                               0,
                                                        QPPS_FIRST_TX_CHAR_UUID);

/// RX data characteristic
//const struct atts_char_desc qpps_char_rx_data = ATTS_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_WR_NO_RESP,
//                                                          0,
//                                                          QPPS_RX_CHAR_UUID);
const struct atts_char_desc qpps_char_rx_data = ATTS_CHAR(ATT_CHAR_PROP_WR,
                                                          0,
                                                          QPPS_RX_CHAR_UUID);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct qpps_env_tag qpps_env;

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
void qpps_init(void)
{
    // Reset environment
    memset(&qpps_env, 0, sizeof(qpps_env));

    // Register QPPS task into kernel
    task_qpps_desc_register();

    // Go to IDLE state
    ke_state_set(TASK_QPPS, QPPS_DISABLED);
}

//void qpps_set_service_uuid(uint8_t param[ATT_UUID_128_LEN])
//{
//    memcpy(qpps_svc, param, ATT_UUID_128_LEN);
//}

void qpps_error_ind_send(uint8_t status)
{
    struct qpps_error_ind *ind = KE_MSG_ALLOC(QPPS_ERROR_IND,
                                              qpps_env.appid,
                                              TASK_QPPS,
                                              qpps_error_ind);
    ind->conhdl    = qpps_env.conhdl;
    //it will be an QPPC status code
    ind->status    = status;
    // send the message
    ke_msg_send(ind);
}

void qpps_disable(void)
{
    //Disable QPPS in database
    attsdb_svc_set_permission(qpps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct qpps_disable_ind * cfg = KE_MSG_ALLOC(QPPS_DISABLE_IND,
                                                 qpps_env.appid, TASK_QPPS,
                                                 qpps_disable_ind);

    memcpy(&cfg->conhdl, &qpps_env.conhdl, sizeof(uint16_t));

    //Notifications Configuration
    cfg->ntf_en = qpps_env.features;

    ke_msg_send(cfg);

    //Go to idle state
    ke_state_set(TASK_QPPS, QPPS_IDLE);
}

#endif /* BLE_QPP_SERVER */

/// @} QPPS
