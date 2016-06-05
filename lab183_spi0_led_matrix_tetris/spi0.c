/*****************************************************************
*
*                          Function spi0.c
*
******************************************************************/

// Include Function
// Include Function
#include <LPC213X.h>
#include "spi0.h"

void init_SPI(void)
{
	PCONP   |= 0x00000100;   // SPI Interface Enable
	// set up port function for SPI interface
	// (1) Clear P0.2-P0.7
	PINSEL0 &= ~(0xFFF0);

	
	// (2) Select P0.4 as SCLK, P0.5 as MISO, 
	//     P0.6 as MOSI, P0.7 as GPIO
	PINSEL0 |= (1 << 8) | (1 << 10) | (1 << 12);
	
  // (3) set P0.7 as Output
	IO0DIR |= STROBE | LATCH;
	
	// (4) set P0.3 and P0.2 as Input
	
	// (5) set up S0SPCR using the following settings
		// BitEnable = 1, 16-bit format
		// CPHA = 0 = Rising Clock Shift Data,
		// CPOL = 0 = Normal Clock
		// MSTR = 1 = Master
		// LSBF = 0 = MSB First
		// SPIE = 0 = Disable SPI Interrupt
	S0SPCR = 0x24;

	S0SPCCR = 15;    // SPI clock rate = PCLK/S0SPCCR ~ 3.67 MHz
	IO0SET = LATCH; // Set LATCH signal
	IO0CLR = STROBE; // Enable display 
}

void write_SPI(unsigned int data)
{
  // Send SPI
  S0SPDR = data; // only 8 bit is read                 
  // Wait SPIF = 1 (SPI Send Complete)
  while((S0SPSR & 0x80) != 0x80);         
}
