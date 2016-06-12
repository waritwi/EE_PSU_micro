/* Tetris program for use with ET-Display 16x32
 * The display uses column driver. 
 * The hardware has been modified to be used with SPI interface.
 *
 * Inteface: SPI 16 bits, need 3 frames of 16 bits
 * Frame data format
 * |   Frame 0   |    Frame 1   |   Frame 2   | 
 * | Row 31: 16  |   Row 15: 0  |   Col 15:0  |
 *
 * Timer0: used for scanning display 
 * default scan rate is 2 kHz per column
 * Timer1: used for user input update
 * scan rate: 1 kHz
 * PWM: used as a timer in the game time
 * default rate: 1 Hz
 * RTC: used to generate random number generator
 * together with T1PC (fast running clock)
 */

#include <LPC213x.h>
#include "lpc213x_vic.h"
#include "spi0.h"
#include "retarget.h"
#include "uart0.h"
#include "tetris.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 1
#define DEBUG_ROTATE 0

#define BASE_ROW 2

// timer constant
#define T0PR_VALUE 29
#define T0MR0_VALUE 500
#define T1PR_VALUE 2949
#define T1MR0_VALUE 49
#define PWMPR_VALUE 2949
#define PWMMR0_VALUE 10000
#define PWMMR0_MIN 1000
#define PWMMR0_DELTA 1000
#define LEVEL_LIMIT 5

#define REPEAT_COUNT 20
#define INITIAL_OFFSET 8
#define DATA_MASK 0xFFFF
#define LATCH_DELAY 50
#define TIMER_LED 8
#define PROC1_LED 10
#define BLOCK_LIST_COUNT 4

#define load_pulse() {IO0CLR = LATCH; IO0SET = LATCH;}
#define proc1_on()  {IO0CLR = (1 << PROC1_LED);}
#define proc1_off() {IO0SET = (1 << PROC1_LED);}

/******************************************* 
 * function prototype
 *******************************************/ 
__irq void timer1IRQ(void);
__irq void timer0IRQ(void);
__irq void pwmIRQ(void);
void setupLed(void);
void initDisp(void);
void timer0Init(void);
void timer0IntSetup(void);
void timer1Init(void);
void timer1IntSetup(void);
void pwmInit(void);
void pwmIntSetup(void);
void pwmDecreaseTime(void);
void rtcInit(void);
void disableTimer(void);
void newShape(void);
void mergeData(void);
int collisionTest(void);
void moveLeft(void);
void moveRight(void);
int clearRow(void);
void resetParam(void);
void mergeDown(int);
int rotateCW(void);
int rotateCW2(void);
int rotateCCW(void);
int dropDown(void); 

int collisionTest2(void);
void moveLeft2(void);

// global variables
unsigned int bgImage[MAX_COL];
unsigned int myBlock[MAX_COL];
unsigned int dispBuffer[2][MAX_COL];
unsigned int myBlock2[MAX_COL];

int displayColumn;
int currentRow;
char currentBuffer;
char updateFlag;
char moveFlag;
char currentShape;
char currentShapeVar;
signed char objColOffset;
char currentLevel;

char endGameFlag;
int clearRowFlag;
int holdCount;
char cmdFlag;
char newShapeFlag;
int collisionRow;
int collisionRow2;
int rotCollision;
int dropCount;
char ledFlag;
int lineErase;
char blockList[BLOCK_LIST_COUNT];
unsigned char blockListIndex;



int main(void){
  int index;
	char cmd;
	
  setupLed(); // set up status LEDs
	
	uart0_init(38400);
	uart0_puts("\nTetris\n");
	// setup VIC
	timer0IntSetup(); 
	timer1IntSetup();
	pwmIntSetup();
	
	// start program automatically when the core is reset
	init_SPI(); // initailize SPI0 and enable display output
	resetParam(); // reset all variables
	initDisp(); // reset the display
	timer0Init(); // start display refresh timer
	timer1Init(); // start user input timer
	rtcInit(); // initialize RTC
  pwmInit(); // start game timer

	while(1){
		
		if(U0LSR & 0x1){
			cmd = U0RBR;
			switch(cmd){
				case 'a': // move right
					cmdFlag = 1;
					break;
				case 'f': // move left
					 cmdFlag = 2;
					break;
				case 'r': // rotate
					 cmdFlag = 3;
					break;
				case ' ':
					 cmdFlag = 4;
					break;
				case 'd': // disable timer
					disableTimer();
					break;
				case 'n': // start a new came
					printf("New game\n");
					disableTimer(); // disable timer
				  resetParam(); // clear all paramerters
					initDisp();  // initialize display
					timer0Init(); // initialize Timer0
					timer1Init(); // initialize Timer1
					rtcInit(); // initialize RTC
				  pwmInit();
					break;
				default: // unknown command
					printf("0x%02x\n",cmd);
					cmdFlag = 0;
					break;
			}
		}
		
		if(moveFlag && !endGameFlag){
			
			#if DEBUG1
			printf("\nRow %2d, ", currentRow);
			#endif
			#if DEBUG2
			proc1_on();
			#endif
			
			ledFlag ^= 0x1;
			if(ledFlag & 0x1){
				IO0CLR = (1 << TIMER_LED);
			}
			else{
				IO0SET = (1 << TIMER_LED);
			}
			
				if(newShapeFlag){
					newShape();  // generate new block
					for(index = 0; index < MAX_COL; index++){
						dispBuffer[currentBuffer^(0x1)][index] = bgImage[index] | myBlock[index];
					}
				}
				else{
					currentRow++; // move the next column	
					// detect collision between myBlock and bgImage
					if(currentRow >= MAX_ROW){
						#if DEBUG1
						uart0_puts(" Bottom");
						#endif
						newShapeFlag = 1;
						currentRow = BASE_ROW;
					}				

				// test collision if move down
				collisionRow = collisionTest();
				#if DEBUG1
				printf("Test2: %2d ", collisionRow);
				#endif
					
				// merge background with the block if collision or reach bottom
				if(collisionRow > BASE_ROW){
					#if DEBUG1
					printf("Collis: %d",collisionRow);
					#endif
					for(index = 0; index < MAX_COL; index++){
						// should this be  
						bgImage[index] = bgImage[index] | (myBlock[index]);
					}
					clearRowFlag = clearRow(); // clear data
					if(clearRowFlag){
						#if DEBUG1
						printf("\nFull Row: 0x%08x\n",clearRowFlag);
						#endif						
						mergeDown(clearRowFlag);
						clearRowFlag = 0;
						#if DEBUG3
						printf("Line erase: %3d\n", lineErase);
						#endif
						// increase time
						if(lineErase >= ((currentLevel + 1)*LEVEL_LIMIT)){ 
							pwmDecreaseTime();
							currentLevel++;
							printf("Level: %2d\n", currentLevel);
						}
					}
					// update the next buffer
					for(index = 0; index < MAX_COL; index++){
						dispBuffer[currentBuffer^(0x1)][index] = bgImage[index];
					}
					// check if the background grew over base row
					if(collisionRow <= BASE_ROW + 2){
						for(index = 0; index < MAX_COL; index++){
							if(bgImage[index] & ((1 << (BASE_ROW + 1)) - 1)){
								endGameFlag = 1;
								printf("Game over 2\n");
								disableTimer();
								newShapeFlag = 0;
							}
						}
					}
				}
				else if(collisionRow == BASE_ROW){
					// update the next buffer
					for(index = 0; index < MAX_COL; index++){
						dispBuffer[currentBuffer^(0x1)][index] = bgImage[index] | myBlock[index];
					}					
					newShapeFlag = 0;
					endGameFlag = 1;
					printf("Game over\n"); // notify user
					//disableTimer();
				}
				// no collision detected just move down the block
				else{
					for(index = 0; index < MAX_COL; index++){
						myBlock[index] = myBlock[index] << 1;					
					}
					// update the next buffer with background and block
					for(index = 0; index < MAX_COL; index++){
						dispBuffer[currentBuffer^(0x1)][index] = bgImage[index] | myBlock[index];
					}
				}
									
			}		
			currentBuffer ^= 0x1; // change buffer			
			moveFlag = 0; // reset flag
			
			// move process debugging
			#if DEBUG2
			proc1_off();
			#endif
		}
		
		
		// user input
		if(updateFlag && cmdFlag){

			if(cmdFlag == 1){
				moveLeft();
			}
			else if(cmdFlag == 2){
				moveRight();
			}
			else if(cmdFlag == 3){
				rotCollision = rotateCW();
				#if DEBUG1
				printf("Rot: %d ", rotCollision);
				#endif
			}
			else if(cmdFlag == 4){
				dropCount = dropDown();
				#if DEBUG1
				printf("Drop: %d ", dropCount);
				#endif
			}

			cmdFlag = 0;
			for(index = 0; index < MAX_ROW; index++){
				dispBuffer[currentBuffer^(0x1)][index] = bgImage[index] | myBlock[index];
			}	
			
			currentBuffer ^= 0x1; // change buffer	
			updateFlag = 0;
		}

	}
}

/*
 * reset display to zero
 */

void initDisp(void){
	char index;
	
	for(index = 0; index < MAX_ROW; index++){
		dispBuffer[0][index] = 0;
		dispBuffer[1][index] = 0;
		bgImage[index] = 0;
	}
}

/*********************************************
 * Timer0 initialization
 *********************************************/
void timer0Init(void){
  T0TCR = 0x2; // reset timer and hold  
	T0CTCR = 0x0; // set bit 1:0 to 0 for timer operation
  T0PR = T0PR_VALUE; // set timer0 pre-scaler
  T0MR0 = T0MR0_VALUE-1; // set timer0 MR0
  T0MCR &= 0xF000; // reset T0MCR bit 11:0
  T0MCR |= 0x3; // when counter reach target value
	              // generate timer0 interrupt, reset, and stop
  T0TCR = 0x1; // start timer	
}

/*********************************************
 * Timer1 initialization
 *********************************************/
void timer1Init(void){
  T1TCR = 0x2; // reset timer and hold  
	T1CTCR = 0x0; // set bit 1:0 to 0 for timer operation
  T1PR = T1PR_VALUE; // set timer1 pre-scaler
  T1MR0 = T1MR0_VALUE-1; // set timer1 MR0
  T1MCR &= 0xF000; // reset T1MCR bit 11:0
  T1MCR |= 0x3; // when counter reach target value
	              // generate timer1 interrupt, reset
  T1TCR = 0x1; // start timer	
}

/*********************************************
 * PWM initialization
 *********************************************/
void pwmInit(void){
  PWMTCR = 0x2; // reset timer and hold  
  PWMPR = PWMPR_VALUE; // set timer0 pre-scaler
  PWMMR0 = PWMMR0_VALUE-1; // set timer0 MR0
  PWMMCR = 0x3; // generate interrupt, reset, and stop	              
  PWMTCR = 0x1; // start timer	
}

/*********************************************
 * PWM change time
 *********************************************/
void pwmDecreaseTime(void){
	if(PWMMR0 > PWMMR0_MIN){
  PWMTCR = 0x2; // reset timer and hold  
  //PWMPR = PWMPR_VALUE; // set timer0 pre-scaler
  PWMMR0 = PWMMR0 - PWMMR0_DELTA; // set timer0 MR0
  PWMMCR = 0x3; // generate interrupt, reset, and stop	              
  PWMTCR = 0x1; // start timer	
	}
}



void timer0IntSetup(void){
  VICIntSelect &= ~(1 << VIC_TIMER0); // use TIMER0 in Vectored IRQ mode
  VICVectAddr0 = (unsigned int) timer0IRQ; // assigned interrupt function
  VICVectCntl0 = 0x20 | VIC_TIMER0;  // vector slot 0 assigned for TIMER0
  VICIntEnable |= (1 << VIC_TIMER0); // enable TIMER0 	
}

void timer1IntSetup(void){
  VICIntSelect &= ~(1 << VIC_TIMER1); // use TIMER1 in Vectored IRQ mode
  VICVectAddr1 = (unsigned int) timer1IRQ; // assigned interrupt function
  VICVectCntl1 = 0x20 | VIC_TIMER1;  // vector slot 1 assigned for TIMER1
  VICIntEnable |= (1 << VIC_TIMER1); // enable TIMER1 	
}

void pwmIntSetup(void){
  VICIntSelect &= ~(1 << VIC_PWM0); // use PWM in Vectored IRQ mode
  VICVectAddr2 = (unsigned int) pwmIRQ; // assigned interrupt function
  VICVectCntl2 = 0x20 | VIC_PWM0;  // vector slot 2 assigned for PWM
  VICIntEnable |= (1 << VIC_PWM0); // enable PWM 	
}

__irq void timer0IRQ(void){  
	write_SPI((dispBuffer[currentBuffer][displayColumn] >> MAX_COL) & DATA_MASK);
	write_SPI(dispBuffer[currentBuffer][displayColumn] & DATA_MASK);
	write_SPI(1 << displayColumn); // column data
	load_pulse();
	displayColumn = (displayColumn+1) & 0xF; // update row
  T0IR = 0x1; // clear TIMER0 MR0 interrupt
  VICVectAddr = 0; // return interrupt  
}

__irq void timer1IRQ(void){
	updateFlag = 1;
  T1IR = 0x1; // clear TIMER1 MR0 interrupt
  VICVectAddr = 0; // return interrupt  
}

__irq void pwmIRQ(void){
	moveFlag = 1;
  PWMIR = 0x1; // clear TIMER1 MR0 interrupt
  VICVectAddr = 0; // return interrupt  
}

void rtcInit(void){
	CCR = 0x11;
}

// disable pwm timer and user input timer 
void disableTimer(void){
	PWMTCR = 0x2;
  PWMIR = 0x1;	
	T1TCR = 0x2;
	T1IR = 0x1;
}


void setupLed(void){
	PINSEL0 &= ~((3 << (TIMER_LED << 1)) | (3 << (PROC1_LED << 1)));
	IO0DIR |= (1 << TIMER_LED) | (1 << PROC1_LED); //
	IO0CLR = (1 << TIMER_LED); // turn on LED
	IO0SET = (1 << PROC1_LED);
}

void resetParam(void){
	int index;
	displayColumn = 0;
	currentRow = BASE_ROW;
  currentBuffer = 0;
	updateFlag = 0;
	moveFlag = 0;
	newShapeFlag = 1;
	endGameFlag = 0;
	cmdFlag = 0;
	clearRowFlag = 0;
  ledFlag = 0;
  lineErase = 0;
	currentLevel = 1;
  blockListIndex = 0;
	for(index = 0; index < BLOCK_LIST_COUNT; index++){
		blockList[index] = BLOCK_SHAPE*BLOCK_VAR;
	}
}

void newShape(void){
	int index;
	char temp;
	
	temp = (((CTC >> 1) ^ T1PC) & 0x7);
	if(temp == (currentShape >> 2)){ 
		currentShape = ((temp + T0PC) & 0x7) << 2;
	}
	else{
		currentShape = temp << 2;
	}
//	do{
//		
//		if((blockList[(blockListIndex - 1) & 0x7] != temp) &&
//			 (blockList[(blockListIndex - 2) & 0x7] != temp)){
//			currentShape = temp;
//			break;
//		}
//	}
//	while(newShape);
	
	
	//currentShape = ((((CTC >> 1) ^ T1PC) + T0PC + 3) & 0x7) << 2;
	currentShapeVar = 0;
	currentRow = BASE_ROW;
	newShapeFlag = 0;
	objColOffset = 0;
	for(index = 0; index < MAX_COL; index++){
		myBlock[index] = blockData[currentShape][index];
		myBlock2[index] = myBlock[index]; // debug
	}
}

void mergeData(void){
	int index;
	for(index = 0; index < MAX_ROW; index++){
		bgImage[index] = bgImage[index] | myBlock[index];					
	}
}


int collisionTest(void){
	int rowIndex, colIndex;
	int mergeCheck;
	int result = 0;
	
	for(colIndex = 0; colIndex < MAX_COL; colIndex++){
		mergeCheck = bgImage[colIndex] & (myBlock[colIndex] << 1); 
		if(mergeCheck){			// if this is true
			for(rowIndex = MAX_ROW-1; rowIndex >= 0; rowIndex--){
				if(mergeCheck & (1 << rowIndex)){
					result = rowIndex - 1; // return result
				}
			}
			newShapeFlag = 1;
			break;
		}
		else if((myBlock[colIndex]) & 0x80000000){		
//		else if((myBlock[colIndex] << 1) & 0x80000000){
			result = 0x100; // end of column
			newShapeFlag = 1;
			break; 
		}
	}
	
	return (result);
}


int collisionTest2(void){
	int rowIndex, colIndex;
	int mergeCheck;
	int result = 0;
	
	for(colIndex = 0; colIndex < MAX_COL; colIndex++){
		mergeCheck = bgImage[colIndex] & (myBlock2[colIndex] << (currentRow - BASE_ROW)); 
		if(mergeCheck){			// if this is true
			for(rowIndex = 0; rowIndex < BLOCK_SIZE; rowIndex++){
				if(mergeCheck & (1 << (rowIndex + currentRow))){
					result = rowIndex + currentRow - 1; // return result
				}
			}
			//newShapeFlag = 1;
			break;
		}
		else if((myBlock2[colIndex] << (currentRow - BASE_ROW)) & 0x80000000){
			result = 0x100; // bottom row
			//newShapeFlag = 1;
			break; 
		}
	}	
	return (result);
}

// check if the object can be move left
void moveLeft(void){
	int colIndex;
	char sideCollision = 0;
	
	if(!(myBlock[0])){
		for(colIndex = 0; colIndex < MAX_COL-1; colIndex++){
			if(bgImage[colIndex] & myBlock[colIndex+1]){
				sideCollision = 1; // collided if move
			}
		}
		// valid move
		if(sideCollision == 0){
			for(colIndex = 0; colIndex < MAX_COL - 1; colIndex++){
				myBlock[colIndex] = myBlock[colIndex+1];
			}
			myBlock[MAX_COL - 1] = 0;
			objColOffset--;
		}
	}
	
}

void moveLeft2(void){

	
}

void moveRight(void){
	int colIndex;
	char sideCollision = 0;
	
	if(!(myBlock[MAX_COL-1])){
		for(colIndex = MAX_COL-1; colIndex > 0; colIndex--){
			if(bgImage[colIndex] & myBlock[colIndex-1]){
				sideCollision = 1; // collided if move
			}
		}
		
		if(sideCollision == 0){
			for(colIndex = MAX_COL-1; colIndex > 0; colIndex--){
				myBlock[colIndex] = myBlock[colIndex-1];
			}
			myBlock[0] = 0;
			objColOffset++;
		}
	}	
}

// check for the full columns
int clearRow(void){
	int result = 0;
	int index, index2;
	int collapseData = bgImage[0];
	
	// combine all row together
	for(index = 1; index < MAX_COL; index++){
		collapseData &= bgImage[index]; 
	}
	
	if(collapseData){
		for(index = 0; index < MAX_ROW; index++){
			if(collapseData & (1 << index)){
				lineErase++; // increase line count
				for(index2 = 0; index2 < MAX_ROW; index2++){
					bgImage[index2] &= ~(1 << index); // clear bit
				}
			}
		}
		result = collapseData;
	}
	return(result);
}

void mergeDown(int data){
	int index, index2;
	int dataMask, tempData;
	
	for(index = 1; index < MAX_ROW; index++){
		if(data & (1 << index)){
			#if DEBUG1
				printf(" M: %d ", index); 
			#endif
			dataMask = ((1 << index)-1);
			for(index2 = 0; index2 < MAX_COL; index2++){
				tempData = (bgImage[index2] & dataMask) << 1; // keep unchange data
				bgImage[index2] = (bgImage[index2] & ~dataMask) | tempData ;
			}
		}
	}
}


int rotateCW(void){
	int result = 0;
	int colIndex, rowIndex;
	signed int tempIndex;
	char tempShape;
	unsigned int tempObj[MAX_COL];
	unsigned int mergeCheck;
	
	#if DEBUG_ROTATE
	printf("Debug-r: %2d ",currentShape);
	#endif
	if(((currentShape >> 2) & 0x7) >= 2){
		tempShape = (((currentShape & 0x3) + 1) & 0x3) + (currentShape & 0x1C);
		#if DEBUG_ROTATE
		printf("T1: %2d %2d\n", tempShape,objColOffset);
		#endif
		// check all collision first
		if(objColOffset >= 7){
			#if DEBUG_ROTATE
			printf("Hit right wall\n");
			#endif
			result = 0x200;
		}
		else if(objColOffset <= -8){
			#if DEBUG_ROTATE
			printf("Hit left wall\n");
			#endif
			result = 0x400;
		}
		else if((currentShape == 9 || currentShape == 11) && objColOffset <= -7){
			#if DEBUG_ROTATE
			printf("Hit left wall for I shape\n");
			#endif
			result = 0x400;			
		}
		else{
			for(colIndex = 0; colIndex < MAX_COL; colIndex++){
				tempIndex = colIndex-objColOffset;;
				if(tempIndex >= 0 && tempIndex < MAX_COL){
					tempObj[colIndex] = blockData[tempShape][tempIndex] << (currentRow - BASE_ROW);
				}
				else{
					tempObj[colIndex] = 0;
				}
				// check collision
				mergeCheck = tempObj[colIndex] & bgImage[colIndex];
				if(mergeCheck){			// if this is true
					#if DEBUG_ROTATE
					printf("can't rotate\n");
					#endif
					for(rowIndex = MAX_ROW-1; rowIndex >= 0; rowIndex--){
						if(mergeCheck & (1 << rowIndex)){
							result = rowIndex - 1; // return result
						}
					}
				}
				else if((tempObj[colIndex] << 1) & 0x80000000){
					result = 0x100; // end of column
					#if DEBUG_ROTATE
					printf("rot: reach bottom\n");
					#endif
				}
			}
		}
		// if it can be moved
		if(result == 0){
			currentShape = tempShape;
			#if DEBUG_ROTATE
			uart0_putchar('\n');
			#endif
			for(colIndex = 0; colIndex < MAX_COL; colIndex++){
				myBlock[colIndex] = tempObj[colIndex];
				myBlock2[colIndex] = blockData[tempShape][colIndex];
				#if DEBUG_ROTATE
				printf("0x%02x,", tempObj[colIndex]);
				#endif
			}
			#if DEBUG_ROTATE
			uart0_putchar('\n');			
			#endif
		}
	}
	return(result);
}


int rotateCCW(void){
	int result = 0;
	
	return(result);
}


int dropDown(void){
  int index,index2;
	int dropCount[MAX_COL];
  int dropMax;	
	
	for(index = 0; index < MAX_COL; index++){
		dropCount[index] = MAX_ROW;
		if(myBlock[index]){
			if(bgImage[index] == 0){
				for(index2 = currentRow; index2 < MAX_ROW; index2++){
					if(myBlock[index] & (1 << index2)){
						dropCount[index] = MAX_ROW - index2 - 1;
					}
					else if((myBlock[index] & (1 << index2)) == 0){
						break; // exit
					}
				}
			}
			else{
				for(index2 = 0; index2 < MAX_ROW; index2++){
					if(bgImage[index] & (myBlock[index] << index2)){
						dropCount[index] = index2 - 1;
						break;
					}
				}
			}
		}
	}
	
	dropMax = dropCount[0];
	for(index = 1; index < MAX_COL; index++){
		if(dropMax > dropCount[index]){
			dropMax = dropCount[index];
		}
	}
	
	for(index = 0; index < MAX_COL; index++){
		myBlock[index] = (myBlock[index] << dropMax);
	}	
	
	currentRow = currentRow + dropMax;
	
	return dropMax;
}
