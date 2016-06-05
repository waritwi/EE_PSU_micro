#include <string.h>
#include <LPC213X.h>
#include "uart0.h"


// Fosc = 19.6608 Mhz 
// CCLK = Fosc*3
// PCLK = CCLK/2
#define PCLK 29491200
#define MAX_DIGIT 10
#define MAX_DECIMAL 6


#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A

void uart0_init(unsigned int baudrate)
{
  unsigned short u0dl;
  PINSEL0 &= 0xFFFFFFF0; // Reset P0.0,P0.1 Pin Config
  PINSEL0 |= 0x00000001; // P0.0 = TxD0
  PINSEL0 |= 0x00000004; // P0.1 = RxD0
  
  U0LCR &= 0xFC; // Reset Word Select
  U0LCR |= 0x03; // World Lenght = 8 bit
  U0LCR &= 0xFB; // Stop Bit = 1 bit
  U0LCR &= 0xF7; // Disable Parity
  U0LCR &= 0xBF; // Disable Break Control
  U0LCR |= 0x80; // Enable Divisor Latch Access Bit
  
  u0dl = PCLK / (baudrate << 4);// u0dl = PCLK/(16 x Buad)
  U0DLL = u0dl & 0xFF;
  U0DLM = (u0dl >> 8);
  
  U0LCR &= 0x7F; // Disable Divisor Latch Access Bit
}

// Write character to Serial Port
int uart0_putchar(int ch)
{
  if(ch == '\n')
  {
    while(!(U0LSR & 0x20));
    U0THR = 0x0D;
  }
  while(!(U0LSR & 0x20));
  return(U0THR = ch);
}

// Read character from Serial Port
int uart0_getchar(void)
{
  while(!(U0LSR & 0x01));
  return (U0RBR);
}

// write string to UART0
int uart0_puts(char *str_in)
{
		int index;
		int str_in_len = strlen(str_in);
		for(index = 0; index < str_in_len; index++)
		{	
				uart0_putchar(str_in[index]);
		}
		
		return str_in_len;
}

void uart0_print_int(int num)
{
	int num_temp = num;
	int index;
	int digit_count = 0;
	int digit[MAX_DIGIT];

	if(num_temp == 0){
		uart0_putchar('0');
		return;
	}

	if(num < 0){
		uart0_putchar('-');
		num_temp = -num;
	}
	
	while(num_temp > 0){
		digit[digit_count] = num_temp%10;
		num_temp = num_temp/10;
		digit_count++;
		if(digit_count >= MAX_DIGIT)
			break;
	}
	
	for(index = digit_count-1; index >= 0; index--){
		uart0_putchar(digit[index]+'0');
	}
	
}

void uart0_print_double(double num, int precision)
{
	double decimal_temp = num - (int) num;
	int num_temp;
	int index;
	int max_precision;
	int digit[MAX_DECIMAL];
	int carry_over;
	
	if(precision > MAX_DECIMAL){
		max_precision = MAX_DECIMAL;
	}
	else{
		max_precision = precision;
	}
	
	for(index = 0; index <= max_precision; index++){
		
		decimal_temp = decimal_temp*10;
		digit[index] = (int) (decimal_temp);
		decimal_temp = decimal_temp - digit[index];
	}
	
	num_temp = (int) decimal_temp; // get all decimal point to be print + 1
	
	if(digit[max_precision] >= 5)
		carry_over = 1;
	else
		carry_over = 0; // inialize carry over
	
	// perform the rounding
	for(index = max_precision - 1; index >= 0; index--){
	  digit[index] += carry_over; // add carry over
		if(digit[index] == 10){
			carry_over = 1;
			digit[index] = 0; // reset index
		}
		else{
			carry_over = 0;
			break;
		}
	}

	num_temp += (int) num + carry_over; // add carry over if any
	
	uart0_print_int(num_temp); // print part integer first

	if(precision == 0){
		return;
	}
	else{
		uart0_putchar('.');
	}
	
	for(index = 0; index < max_precision ; index++){
		uart0_putchar(digit[index]+'0');
	}
	
}


int getchar(void)
{
  while(!(U0LSR & 0x01));
  return (U0RBR);
}

void uart0_getline (char *line, int n)  {
  int  cnt = 0;
  char c;

  do  {
    if ((c = uart0_getchar()) == CR)  c = LF;     /* read character                 */
    if (c == BACKSPACE  ||  c == DEL)  {    /* process backspace              */
      if (cnt != 0)  {
        cnt--;                              /* decrement count                */
        line--;                             /* and line pointer               */
        uart0_putchar(BACKSPACE);           /* echo backspace                 */
        uart0_putchar(' ');
        uart0_putchar(BACKSPACE);
      }
    }
    else if (c != CNTLQ && c != CNTLS)  {   /* ignore Control S/Q             */
      uart0_putchar (*line = c);            /* echo and store character       */
      line++;                               /* increment line pointer         */
      cnt++;                                /* and count                      */
    }
  }  while(cnt < n - 1  &&  c != LF);      /* check limit and line feed      */
  *(line - 1) = 0;                         /* mark end of string             */
}



