#ifndef _USR_SPI_H_
#define _USR_SPI_H_

#include "system.h"
#include "spi.h"

#define USR_SPI_CS_PIN		GPIO_P12

#define USR_SPI_CS_EN() 	gpio_write_pin(USR_SPI_CS_PIN,GPIO_LOW)
#define USR_SPI_CS_DIS() 	gpio_write_pin(USR_SPI_CS_PIN,GPIO_HIGH)

enum SPI_MODE_
{
    SPI_MODE_0 = 0, 
    SPI_MODE_1,
		SPI_MODE_2,
		SPI_MODE_3
};

extern void usr_spi_init(enum SPI_MODE_ spi_mode);
extern uint8_t usr_spi_tran(uint8_t send_data);

#endif

