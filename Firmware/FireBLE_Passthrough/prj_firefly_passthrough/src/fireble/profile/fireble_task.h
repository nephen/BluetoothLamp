#ifndef _FIREBLE_TASK_H_
#define _FIREBLE_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#ifdef CFG_PRF_FIREBLE
#include <stdint.h>
#include "ke_task.h"
#include "fireble.h"
#include "fireble_common.h"
/*
 * DEFINES
 ****************************************************************************************
 */


#define FIREBLE_IDX_MAX     0x01

#if BLE_OTA_ENABLE
#define OTA_PIC_UPDATA		1
#define OTA_FIRST_IS_PIC	1		// 1:first updata is pic ; 0:first updata is code!
#else
#define OTA_PIC_UPDATA		0
#define OTA_FIRST_IS_PIC	0		// 1:first updata is pic ; 0:first updata is code!
#endif
#define OTA_RESET_FLASH		0xfe
#define OTA_RESET_PIC		0xfd
#define OTA_UPDATA_TIMEOUT	1500


#define FirebleRXNum		1
#define FirebleTXNum		1    

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ota_pic_env
{
	unsigned int addr;
};
	
struct fireble_data_proc_env
{
	bool IsOTA;
	uint8_t  DataProcStep;
	uint8_t  TotalFrame;
	uint16_t TotalDataLen;
	uint16_t DataCnt;
	uint16_t CheckSumRecive;
	uint16_t CheckSumCalcu;
	uint16_t  DataBuffP;
	uint8_t  *DataBuff;
};

enum
{
	DATA_RESULT_OK,
	DATA_RESULT_ERROR		
};

enum
{
	OTA_STATUS_END,
	OTA_STATUS_PIC,
	OTA_STATUS_FLASH
};

enum
{
	DATA_TYPE_FRAM_RESULT
};

enum
{
	FIREBLE_DATAPROC_INIT,
	FIREBLE_DATAPROC_GET_TOTALPACAGE,	
	FIREBLE_DATAPROC_DEAL_HEAD,
	FIREBLE_DATAPROC_DEAL_DATA,	
	FIREBLE_DATAPROC_DEAL_LOST,	
	
	FIREBLE_DATAPROC_END
};


/// Possible states of the FIREBLE task
enum
{
    /// Disabled state
    FIREBLE_DISABLED,
    /// Idle state
    FIREBLE_IDLE,
    /// Connected state
    FIREBLE_CONNECTED,

    /// Number of defined states.
    FIREBLE_STATE_MAX,
};

/// Messages for FIREBLE Profile
enum
{
    ///Start the FIREBLE Profile - at connection
    FIREBLE_ENABLE_REQ = KE_FIRST_MSG(TASK_FIREBLE),

    /// Disable confirmation with configuration to save after profile disabled
    FIREBLE_DISABLE_IND,

    /// Error indication to Host
    FIREBLE_ERROR_IND,

    ///Send FIREBLE sport data to Phone APP
    FIREBLE_SEND_DATA_REQ,
    
    ///Send FIREBLE Profile value confirm to Phone APP 
    ///if correctly sent.
    FIREBLE_SEND_DATA_CFM,

    ///Receive FIREBLE Profile value form Phone APP
    FIREBLE_RECEIVE_DATA_IND,

    ///Add FIREBLE into the database
    FIREBLE_CREATE_DB_REQ,
	
    ///Inform APP about DB creation status
    FIREBLE_CREATE_DB_CFM,

	FIREBLE_DEAL_OTA_IND
};

///Parameters of the @ref FIREBLE_CREATE_DB_REQ message
struct fireble_create_db_req
{
    ///Database configuration
    uint8_t features;
	
    uint8_t tx_char_num;
    uint8_t rx_char_num;
	uint8_t *tx_uuid;
	uint8_t *rx_uuid;
};

/// Parameters of the @ref FIREBLE_ENABLE_REQ message
struct fireble_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized;
    /// b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or DISCOVERY
    uint8_t con_type;

	uint8_t out_ntf_en;
	
    uint8_t out_ch_loc;
};

/////Parameters of the @ref FIREBLE_MEAS_SEND_REQ message
struct fireble_data_send_req
{
		///Connection handle
		uint16_t conhdl;	
		uint16_t len;
		uint8_t  Channel;
		uint8_t  channelIndex;  // add by wufan for continuous send by all the tx channel function, save corresponding channel in bits of char
		uint8_t	 outData[FIREBLE_SEND_DATA_MAX_LEN];
};


/// @endcond

/**
 ****************************************************************************************
 * @addtogroup APP_fireble_TASK
 * @{
 ****************************************************************************************
 */

///Parameters of the @ref FIREBLE_CREATE_DB_CFM message
struct fireble_create_db_cfm
{
    ///Status
    uint8_t status;
};

///Parameters of the @ref FIREBLE_DISABLE_IND message
struct fireble_disable_ind
{
    uint16_t conhdl;
    /// Heart Rate Notification configuration
    uint16_t fireble_ntf_en;
};

///Parameters of the @ref FIREBLE_MEAS_SEND_CFM message
struct fireble_out_ch_send_cfm
{
    ///Connection handle
    uint16_t conhdl;
	uint8_t channel_index;
    ///Status
    uint8_t status;
};

struct fireble_in_ch_rec_ind
{
    ///Connection handle
    uint16_t conhdl;
	uint8_t channel;
    uint8_t length;
    uint8_t data[FIREBLE_REC_DATA_MAX_LEN];
//	uint8_t *ReciveDataP;
};


/// @} APP_fireble_TASK

/// @cond

extern 	struct fireble_data_proc_env DataProcEnv;


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler fireble_state_handler[FIREBLE_STATE_MAX];
extern const struct ke_state_handler fireble_default_handler;
extern ke_state_t fireble_state[FIREBLE_IDX_MAX];

extern void task_fireble_desc_register(void);
extern void DataProc(uint8_t CharIndex,uint8_t *DataP,uint8_t DataLen);
#if BLE_OTA_ENABLE
extern int ota_receive_data_timeout_timer_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id);
#endif

#endif /* #ifdef CFG_PRF_FIREBLE */

/// @} FIREBLETASK
/// @endcond
#endif /* _FIREBLE_TASK_H_ */
