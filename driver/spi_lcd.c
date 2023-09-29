#include "common.h"
#include "hardware.h"
#include "gpio.h"
#include "debug.h"
#include "div.h"
#include "board.h"
#include "pmc.h"
#include "timer.h"
#include "flexcom.h"
#include "string.h"
#include "arch/at91_pio.h"

#define FLEXCOM_SPI_SR_TXEMPTY_MASK (unsigned int) (1 << 9)
#define CDX_OUTPUT_PIN AT91C_PIN_PD(4)
#define DISPEN_OUTPUT_PIN AT91C_PIN_PC(24)

static struct at91_flexcom flexcoms[] = {
        {AT91C_ID_FLEXCOM0, FLEXCOM_SPI, AT91C_BASE_FLEXCOM0},
        {AT91C_ID_FLEXCOM1, FLEXCOM_SPI, AT91C_BASE_FLEXCOM1},
        {AT91C_ID_FLEXCOM2, FLEXCOM_SPI, AT91C_BASE_FLEXCOM2},
        {AT91C_ID_FLEXCOM3, FLEXCOM_SPI, AT91C_BASE_FLEXCOM3},
        {AT91C_ID_FLEXCOM4, FLEXCOM_SPI, AT91C_BASE_FLEXCOM4},
        {AT91C_ID_FLEXCOM5, FLEXCOM_SPI, AT91C_BASE_FLEXCOM5},
        {AT91C_ID_FLEXCOM6, FLEXCOM_SPI, AT91C_BASE_FLEXCOM6},
        {AT91C_ID_FLEXCOM7, FLEXCOM_SPI, AT91C_BASE_FLEXCOM7},
        {AT91C_ID_FLEXCOM8, FLEXCOM_SPI, AT91C_BASE_FLEXCOM8},
        {AT91C_ID_FLEXCOM9, FLEXCOM_SPI, AT91C_BASE_FLEXCOM9},
        {AT91C_ID_FLEXCOM10, FLEXCOM_SPI, AT91C_BASE_FLEXCOM10},
        {AT91C_ID_FLEXCOM11, FLEXCOM_SPI, AT91C_BASE_FLEXCOM11},
        {AT91C_ID_FLEXCOM12, FLEXCOM_SPI, AT91C_BASE_FLEXCOM12},
};

int spi_send_data(unsigned int spi_base, u16 val, int command ) {

	unsigned int reg = readl(spi_base + 0x410); // read SPI_STATUS register
	int count = 0;
	int timeout = 1;

	if (command == 0) {
		pio_set_value(CDX_OUTPUT_PIN, 0);
	}
 
        writel((val | (command << 8)), spi_base + 0x40C);

        do {
		reg = readl(spi_base + 0x410); // read SPI_STATUS register
		
		if (reg & FLEXCOM_SPI_SR_TXEMPTY_MASK) {
			timeout = 0;
			break;
		}
		wait_interval_timer(10);
		count ++;

	} while(count < 100);

	if (timeout) {
		dbg_info("[ERROR] FLEXCOM_SPI_SR_TXEMPTY_MASK did not set after writing TDR register, SPI_SR=%x\n", reg);
	}
	pio_set_value(CDX_OUTPUT_PIN, 1);

	return (timeout==1) ? 1 : 0;
}

int spi_lcd_controller_init(int flx_number) {

	/* toggle LCD_EN pin */
        pio_set_value(DISPEN_OUTPUT_PIN, 0);
        wait_interval_timer(200);
        pio_set_value(DISPEN_OUTPUT_PIN, 1);
        wait_interval_timer(200);

	/* initialize SPI controller */
	unsigned int spi_base = flexcoms[flx_number].addr;
	dbg_info("SPI_BASE=%x\n", (unsigned int) spi_base);
	
	unsigned int reg = 1;
        writel(reg, spi_base + 0x400); // set SPI_CR enable bit

	/* unlock SPI write protection mode register */
        unsigned int key = 0x53504900;
	reg = key;
	reg &= ((unsigned int) 0xFFFFFFFE); // clear WPEN bit 0
        writel(reg, spi_base + 0x4E4);

	reg = (1) | (0<< 3) ; // set HOST mode, GCLK as SPI clock source
        writel(reg, spi_base + 0x404); // write SPI_MR register

        reg = readl(spi_base + 0x404); // read SPI_MR register
        dbg_info("SPI_MR=%x\n", reg);


        reg = (0x64 << 8) | (0 << 4) | (1 << 2) | (1 << 1); // configure SPI clock to GCK/100 = 2MHz, set to 8-bit transfers, set CS high after each transfer, sample on rising clock edge
        writel(reg, spi_base + 0x430); // write SPI_MODE register

        reg = readl(spi_base + 0x430); // read SPI_CHIP_SELECT register

        reg = readl(spi_base + 0x410); // read SPI_STATUS register

	spi_send_data(spi_base, (u16) 0x11, 0);

	wait_interval_timer(200);
	
	spi_send_data(spi_base, (u16) 0x29, 0); 

	//--------------------------------ST7789S Frame rate setting----------------------------------//

	spi_send_data(spi_base, (u16) 0xb2, 0);
	spi_send_data(spi_base, (u16) 0x0c, 1);
	spi_send_data(spi_base, (u16) 0x00, 1);
	spi_send_data(spi_base, (u16) 0x33, 1);
	spi_send_data(spi_base, (u16) 0x33, 1);

	spi_send_data(spi_base, (u16) 0xb7, 0);
	spi_send_data(spi_base, (u16) 0x35, 1);

	spi_send_data(spi_base, (u16) 0x3A, 0);
	spi_send_data(spi_base, (u16) 0x55, 1);
	//---------------------------------ST7789S Power setting--------------------------------------//
	spi_send_data(spi_base, (u16) 0xbb, 0);
	spi_send_data(spi_base, (u16) 0x30, 1);//vcom

	spi_send_data(spi_base, (u16) 0xc3, 0);
	spi_send_data(spi_base, (u16) 0x1C, 1);//17µ÷ÉîÇ³

	spi_send_data(spi_base, (u16) 0xc4, 0);
	spi_send_data(spi_base, (u16) 0x18, 1);

	spi_send_data(spi_base, (u16) 0xc6, 0);
	spi_send_data(spi_base, (u16) 0x0f, 1);

	spi_send_data(spi_base, (u16) 0xd0, 0);
	spi_send_data(spi_base, (u16) 0xa4, 1);
	spi_send_data(spi_base, (u16) 0xa2, 1);
	//--------------------------------ST7789S gamma setting---------------------------------------//
	spi_send_data(spi_base, (u16) 0xe0, 0);
	spi_send_data(spi_base, (u16) 0xf0, 1);
	spi_send_data(spi_base, (u16) 0x00, 1);
	spi_send_data(spi_base, (u16) 0x0a, 1);
	spi_send_data(spi_base, (u16) 0x10, 1);
	spi_send_data(spi_base, (u16) 0x12, 1);
	spi_send_data(spi_base, (u16) 0x1b, 1);
	spi_send_data(spi_base, (u16) 0x39, 1);
	spi_send_data(spi_base, (u16) 0x44, 1);
	spi_send_data(spi_base, (u16) 0x47, 1);
	spi_send_data(spi_base, (u16) 0x28, 1);
	spi_send_data(spi_base, (u16) 0x12, 1);
	spi_send_data(spi_base, (u16) 0x10, 1);
	spi_send_data(spi_base, (u16) 0x16, 1);
	spi_send_data(spi_base, (u16) 0x1b, 1);

	spi_send_data(spi_base, (u16) 0xe1, 0);
	spi_send_data(spi_base, (u16) 0xf0, 1);
	spi_send_data(spi_base, (u16) 0x00, 1);
	spi_send_data(spi_base, (u16) 0x0a, 1);
	spi_send_data(spi_base, (u16) 0x10, 1);
	spi_send_data(spi_base, (u16) 0x11, 1);
	spi_send_data(spi_base, (u16) 0x1a, 1);
	spi_send_data(spi_base, (u16) 0x3b, 1);
	spi_send_data(spi_base, (u16) 0x34, 1);
	spi_send_data(spi_base, (u16) 0x4e, 1);
	spi_send_data(spi_base, (u16) 0x3a, 1);
	spi_send_data(spi_base, (u16) 0x17, 1);
	spi_send_data(spi_base, (u16) 0x16, 1);
	spi_send_data(spi_base, (u16) 0x21, 1);
	spi_send_data(spi_base, (u16) 0x22, 1);

	//*********SET RGB Interfae***************

	spi_send_data(spi_base, (u16) 0xB0, 0); 
	spi_send_data(spi_base, (u16) 0x11, 1); //set RGB interface and DE mode.
	spi_send_data(spi_base, (u16) 0x00, 1); 

	spi_send_data(spi_base, (u16) 0xB1, 0); 
	spi_send_data(spi_base, (u16) 0xE0, 1); // set DE mode ; SET Hs,Vs,DE,DOTCLK signal polarity, 
	spi_send_data(spi_base, (u16) 0x08, 1); //VBP 
	spi_send_data(spi_base, (u16) 0x14, 1); //HBP

	spi_send_data(spi_base, (u16) 0x3a, 0); 
	spi_send_data(spi_base, (u16) 0x55, 1); //18 RGB ,55-16BIT RGB

	//************************
	spi_send_data(spi_base, (u16) 0x11, 0); 
	
	wait_interval_timer(100);

	spi_send_data(spi_base, (u16) 0x29, 0); //display on
	spi_send_data(spi_base, (u16) 0x2c, 0); 

	dbg_info("LCD display initialized\n");

	return 0;
}
