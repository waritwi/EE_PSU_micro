
#include "delay.h"

#define MYDELAY_COUNT 96


void delay_10us(long mydelay)
{
	int index1;
	long	index2;
	for(index2 = 0; index2 < mydelay; index2++){
		for(index1 = 0; index1 < MYDELAY_COUNT; index1++){
		// empty for loop
		}
	}
}

void delay_ms(int mydelay)
{
	int index1;
	for(index1 = 0; index1 < mydelay; index1++){
		delay_10us(100);
	}
}



void delay_sec(int mydelay)
{
	int index1;
	for(index1 = 0; index1 < mydelay; index1++){
		delay_10us(99999);
	}
}
