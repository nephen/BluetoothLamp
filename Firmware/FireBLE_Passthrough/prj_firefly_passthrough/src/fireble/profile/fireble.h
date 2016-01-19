#ifndef _FIREBLE_H_
#define _FIREBLE_H_
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#ifdef CFG_PRF_FIREBLE
#include "fireble_common.h"
#include "prf_types.h"
#include "attm.h"
#include "atts.h"
#include "atts_db.h"

/*
 * DEFINES
 ****************************************************************************************
 */
 
#define FIREBLE_MANDATORY_OUT_INCREASE_NUM     3
#define FIREBLE_MANDATORY_OUT_INCREASE_MASK    0x07
#define FIREBLE_MANDATORY_IN_INCREASE_NUM     2
#define FIREBLE_MANDATORY_IN_INCREASE_MASK    0x03

#define FIREBLE_MANDATORY_SUM_NUM          6
#define FIREBLE_MANDATORY_SUM_MASK         0x3F//6 one 

#define FIREBLE_RX_CHAR_NUM                1//6
#define FIREBLE_RX_CHAR_PER_VOLUME         20

#define FIREBLE_TX_CHAR_NUM                5//13
#define FIREBLE_TX_CHAR_PER_VOLUME         20


//#define FIREBLE_MANDATORY_MASK			(0x0F)
//#define FIREBLE_OUT_DATA_MASK       		(0x30)
//#define FIREBLE_IN_DATA_MASK       		(0xC0)

/*
 * MACROS
 ****************************************************************************************
 */

#define FIREBLE_IS_SUPPORTED(mask) \
    (((fireble_env.features & mask) == mask))

///Attributes State Machine
enum
{
    FIREBLE_IDX_SVC,

	// out chanel , write data to phone
    FIREBLE_IDX_OUT_DATA_CHAR,
    FIREBLE_IDX_OUT_DATA_VAL,
    FIREBLE_IDX_OUT_DATA_NTF_CFG, // notifaction
  
	// in chanel, read data from phone
    FIREBLE_IDX_IN_DATA_CHAR,
    FIREBLE_IDX_IN_DATA_VAL,

    FIREBLE_IDX_NB,
};


enum
{
    FIREBLE_CH_OUT_CHAR,
    FIREBLE_CH_IN_CHAR,
    FIREBLE_CH_CHAR_MAX,
};


enum
{
		FIREBLE_OUT_CH_LOC_SUP 	= 0x01,
		FIREBLE_IN_CH_LOC_SUP 	= 0x02,
    FIREBLE_OUT_CH_NTF_CFG 	= 0x04,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct fireble_env_tag
{
    ///Application Task Id
    ke_task_id_t appid;
    /// Connection Info
    struct prf_con_info con_info;

    ///Service Start Handle
    uint16_t shdl;
    ///Database configuration
    uint8_t features;
	  uint8_t out_features;
    struct
    {
        uint16_t index;
        uint16_t length;
        uint8_t *pdata;
    }rx;
	
    struct
    {
        uint16_t index;
        uint16_t length;
        uint8_t *pdata;
    }tx;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

//extern const struct atts_desc fireble_att_db[FIREBLE_IDX_NB];
//extern const atts_svc_desc_t fireble_svc;
extern struct fireble_env_tag fireble_env;


extern const struct atts_char_desc fireble_data_out;
extern const struct atts_char_desc fireble_data_in;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the FIREBLE module.
 * This function performs all the initializations of the FIREBLE module.
 ****************************************************************************************
 */
void fireble_init(void);

/**
 ****************************************************************************************
 * @brief Send a FIREBLE_SEND_SPORT_DATA_CFM message to the application.
 *
 * @param[in] status Confirmation Status
 ****************************************************************************************
 */
void fireble_out_ch_data_cfm_send(uint8_t status, uint8_t channelIndex);


/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void fireble_disable(void);

#endif /* #ifdef CFG_PRF_FIREBLE */

/// @} FIREBLE

#endif /* _FIREBLE_H_ */
