#include <LPC213X.h>
#include "lpc213x_vic.h"
#include "delay.h"
#include "retarget.h"
#include "uart0.h"
#include "i2c.h"
#include "mpu6050_reg.h"

#define MPU_ADDR 0x68
#define TX_BUFFER_SIZE 32
#define RX_BUFFER_SIZE 32

#define TX_DIR 0
#define RX_DIR 1

void i2cInit(void);
void i2cSetISR(void);
__irq void i2cISR(void);
void readReg(char devAddr, unsigned char reg, unsigned char numByte, unsigned char *rxBuff);
void writeReg(char devAddr, unsigned char reg, unsigned char numByte, unsigned char *txBuff);

unsigned char txBuffer[TX_BUFFER_SIZE];
unsigned char txBufferIndex;
unsigned char txNumByte;
unsigned char rxBuffer[RX_BUFFER_SIZE];
unsigned char rxBufferIndex;
unsigned char rxReg;
unsigned char myData;
char txFlag;
char rxDataFlag;
char dirFlag;
unsigned char slaveAddr;

int main(void){
  
	char cmd;


	// setup I2C 
	i2cInit();
	i2cSetISR();
	IO0DIR |= 0xFF00; // set bit 8 to GPIO
	IO0SET = 0xFF00; 
	
	uart0_init(38400); // setup UART0
	uart0_puts("Test I2C\n");
	
	txBufferIndex = 0;
	txFlag = 0;
	rxDataFlag = 0;
	
	while(1){ 
	
		if(U0LSR & 0x1){
			cmd = U0RBR;
			
			switch (cmd){
				case 't':
					uart0_puts("Test\n");
					
					break;
				case 'd':
					myData = MPU_ADDR << 1; // set address + W
				  rxReg = MPU6050_RA_WHO_AM_I;
				  IO0SET = 0xFF00; 
					I2C0CONSET = 0x20; // set start condition
					txFlag = 1;
				  //txBufferIndex = TX_BUFFER_SIZE-1;
					break;
				
				case 'w':
					
					break;
				
				default:
					
					break;
			}
			

			
		}
		
		if(rxDataFlag == 1){
			printf("Rx: 0x%02x\n", rxBuffer[0]);
			rxDataFlag = 0; // reset flag
		}
		
	}
	
}

void i2cInit(void){
	PINSEL0 = 0;
	PINSEL0 |= 0x50; // set P0.2 to SCL and P0.3 to SDA
	I2C0SCLL = 72;
	I2C0SCLH = 72; // set SPI duty to 50% at 100 kHz
	I2C0CONSET = 0x40; // enable I2C0 (set I2CEN)
}

void i2cSetISR(void){
   VICIntSelect &= ~(1 << VIC_I2C); // use I2C in Vectored IRQ mode
   VICVectAddr0 = (unsigned int) i2cISR; // assigned interrupt function
   VICVectCntl0 = 0x20 | VIC_I2C;  // vector slot 0 assigned for I2C
   VICIntEnable |= (1 << VIC_I2C); // enable I2C  	
}

__irq void i2cISR(void){
	
	char i2cStatus;
	
	i2cStatus = I2C0STAT; // read status
	
	switch(i2cStatus){
		
		case 0x8:
		  if(dirFlag == 1){// read
				I2C0DAT = slaveAddr << 1 | 0x1; // SLA + R
				I2C0CONSET = 0x04; // enable ACK
			}
			else{ // write
				txBufferIndex = 0;
				I2C0DAT = slaveAddr << 1; // SLA + W
			}	  
		  I2C0CONCLR = 0x28; // clear STA and SI
			break;
		
		case 0x18: // SLA + W and ACK
			I2C0DAT = txBuffer[txBufferIndex];
		  txBufferIndex++; // increase Tx buffer index
		  I2C0CONCLR = 0x08; // clear SI interrupt
			break;
		
		case 0x20: // SLAVE ADDR + W send and NO ACK
		  I2C0CONSET = 0x10; // enable ACK
		  I2C0CONCLR = 0x08; // clear SI interrupt
			break;
		
		case 0x28:
			if(dirFlag){ // read
				I2C0CONSET = 0x30; // set STO and STA
				I2C0CONCLR = 0x08; // clear SI interrupt				
			}
			else{
				if(txBufferIndex >= txNumByte){
					I2C0CONSET = 0x10; // set STO
					I2C0CONCLR = 0x08; // clear SI interrupt
				}
				else{// send the next byte 
					I2C0CONCLR = 0x08; // clear SI interrupt
				}
			}

			break;
		
		case 0x30: // no ACK is received
		  I2C0CONSET = 0x14; // enable ACK
		  I2C0CONCLR = 0x08; // clear SI interrupt			
			break;

		case 0x38: // no ACK is received
		  I2C0CONSET = 0x24; // enable ACK
		  I2C0CONCLR = 0x08; // clear SI interrupt			
			break;		

		case 0x40: // no ACK is received
		  //I2C0CONSET = 0x24; // enable ACK
		  I2C0CONCLR = 0x0C; // clear SI interrupt			
			break;		
		
		case 0x48: // SLAVE ADDR + W send and NO ACK
			// send a stop condition w
		  I2C0CONSET = 0x14; // enable ACK
		  I2C0CONCLR = 0x08; // clear SI interrupt
			break;

		case 0x50:
			// stop transmission 
		  if(rxBufferIndex >= RX_BUFFER_SIZE-1){
				rxBuffer[rxBufferIndex] = I2C0DAT; // read data
				rxBufferIndex++;
				I2C0CONCLR = 0x0C; // clear SI interrupt
			}
			else{ // send the next byte
				rxBuffer[rxBufferIndex] = I2C0DAT; // read data
				rxBufferIndex++;
				I2C0CONSET = 0x04; // enable ACK
				I2C0CONCLR = 0x08; // clear SI interrupt
			}
			break;

		case 0x58: // SLAVE ADDR + W send and NO ACK
			rxBuffer[0] = I2C0DAT; // read data
			rxDataFlag = 1;
		  I2C0CONSET = 0x10; // enable ACK
		  I2C0CONCLR = 0x08; // clear SI interrupt
			break;

		default:	
			break;
		
	}
	
	VICVectAddr = 0; // return interrupt
}


void readReg(char devAddr, unsigned char reg, unsigned char numByte, unsigned char *rxBuff){
	//unsigned char index;
	
	I2C0CONSET = 0x20; // set start condition
	
}

void writeReg(char devAddr, unsigned char reg, unsigned char numByte, unsigned char *txBuff){
	//unsigned char index;
	
	slaveAddr = devAddr;
  dirFlag = TX_DIR;
	txNumByte = numByte;
}

