/*****************************************************************
*
*                          Function spi0.h
*
******************************************************************/

// LPC2103      <----->   CC2500
// GPIO(P0.3)  <----->   LATCH
// GPIO(P0.7)  <----->   STROBE
// SCK(P0.4)    <----->   SCLK
// MISO(P0.5)   <----->   SO
// MOSI(P0.6)   <----->   SI



// P0.3 to GDO0
#define LATCH 0x00000008  
// P0.4 to SCLK0
#define SCLK0 0x00000010       
// P0.5 to MISO0
#define MISO0 0x00000020       
 // P0.6 to MOSI0
#define MOSI0 0x00000040      
// P0.7 to STROBE
#define STROBE 0x00000080       

// Function Prototype
void init_SPI(void);
void write_SPI(unsigned int);
