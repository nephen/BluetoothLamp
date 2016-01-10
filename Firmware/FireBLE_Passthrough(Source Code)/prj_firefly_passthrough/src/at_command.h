#ifndef AT_COMMAND_H_
#define AT_COMMAND_H_

#include "app_env.h"

#define SOFTWARE_VERSION	"v1.1"
#define RELEASE_DATE "2015-07-7"

typedef int (*pdCOMMAND_CALLBACK)(const uint8_t * pcCommandString,uint8_t* pcWriteBuffer,uint32_t commpare_length);

typedef struct AT_COMMAND_INPUT
{
	const uint8_t  * const pcCommand;	
	const pdCOMMAND_CALLBACK pxCommandInterpreter;
} At_CommandInput;

typedef enum{
	AT_TYPE_SET = 0,
	AT_TYPE_GET
}AT_TYPE;

uint8_t at_command_length_get(const uint8_t *command);
uint8_t at_get_parameters_numbers( const uint8_t * pcCommandString );
uint32_t at_HEXstringToNum(const uint8_t *str, uint32_t length);
const int8_t *at_get_parameter( const int8_t *pcCommandString, int32_t uxWantedParameter, uint32_t *pxParameterStringLength );
int at_process_command(const uint8_t* const pcCommandInput,uint8_t* pcWriteBuffer);
#endif

