#ifndef _FIREBLE_COMMON_H_
#define _FIREBLE_COMMON_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#ifdef CFG_PRF_FIREBLE

#include "prf_types.h"
#include <stdint.h>

#define FIREBLE_SEND_DATA_MAX_LEN				(20)
#define FIREBLE_REC_DATA_MAX_LEN					(20)


/*
 * DEFINES
 ****************************************************************************************
 */

#if FIREBLE_USART_MODULE
#define FIREBLE_SVC_PRIVATE_UUID            0xdd03 //Scale
#elif FIREBLE_SMART_BALL
#define FIREBLE_SVC_PRIVATE_UUID            0xcc03 //Scale
#else
#define FIREBLE_SVC_PRIVATE_UUID            0xCC03//Scale

#endif
enum
{
	FIREBLE_DATA_OUT_UUID 		 = 0xEB00,// data form dev to phone
	FIREBLE_DATA_IN_UUID  		 = 0xEC00,// data form phone to dev 
};


// error code
#define FIREBLE_ERR_RX_DATA_NOT_SUPPORTED		(0x80)
#define FIREBLE_ERR_RX_DATA_EXCEED_MAX_LENGTH	(0x81)
#define FIREBLE_ERR_INVALID_PARAM				(0x82)


#endif /* #ifdef CFG_PRF_FIREBLE */

/// @} fireble_common

#endif /* _FIREBLE_COMMON_H_ */
