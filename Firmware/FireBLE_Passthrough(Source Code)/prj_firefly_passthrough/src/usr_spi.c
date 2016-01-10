#include "usr_spi.h"

void usr_spi_io_conf(void)
{
		syscon_SetPMCR0WithMask(QN_SYSCON,
															P10_MASK_PIN_CTRL 
															| P11_MASK_PIN_CTRL  
															| P12_MASK_PIN_CTRL 
															| P13_MASK_PIN_CTRL,
	
	                             P13_SPI1_CLK_PIN_CTRL
                             | P12_GPIO_10_PIN_CTRL 
                             | P11_SPI1_DAT_PIN_CTRL
                             | P10_SPI1_DIN_PIN_CTRL);
	
		gpio_set_direction(USR_SPI_CS_PIN,GPIO_OUTPUT);
		gpio_write_pin(USR_SPI_CS_PIN,GPIO_HIGH);		
	
}

void usr_spi_init(enum SPI_MODE_ spi_mode)
{
    uint32_t reg = 0;
	
		usr_spi_io_conf();
    spi_clock_on(QN_SPI1);
	
		reg = SPI_MASK_SPI_IE
		| SPI_SSx_CFG
		| SPI_BIG_ENDIAN
		| SPI_8BIT
		| SPI_BITORDER_CFG
		| SPI_MASTER_MOD
		| spi_mode
		|SPI_BITRATE(1000000);
	
    spi_spi_SetCR0(QN_SPI1, reg);
	
}

uint8_t usr_spi_tran(uint8_t send_data)
{
	
	while ( !(spi_spi_GetSR(QN_SPI1) & SPI_MASK_TX_FIFO_NFUL_IF) );
	spi_spi_SetTXD(QN_SPI1, send_data);
	QN_SPI1->RXD;
	while ( !(spi_spi_GetSR(QN_SPI1) & SPI_MASK_RX_FIFO_NEMT_IF) );
	return QN_SPI1->RXD;
}

//uint8_t id1,id2;
//void read_id(void)
//{
//	gpio_write_pin(USR_SPI_CS_PIN,GPIO_LOW);
//	usr_spi_tran(0x90);	
//	usr_spi_tran(0x00);	
//	usr_spi_tran(0x00);	
//	usr_spi_tran(0x00);	
//	id1 = usr_spi_tran(0x00);	
//	id2 = usr_spi_tran(0x00);	
//	gpio_write_pin(USR_SPI_CS_PIN,GPIO_HIGH);
//}
//void test_spi(void)
//{

//	usr_spi_init(SPI_MODE_0);
//		    // pin mux

//	
//	while(1)
//	{
//		read_id();
//		delay(100000);

//	}
//	

//}
