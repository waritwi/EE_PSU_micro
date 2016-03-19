#include <LPC213X.h>
#include "lcd.h"


// xxxx xxx0 0000 0000 0000 0000 0000 0000

/* Strobe 4-Bit Data to LCD */
void lcd_out_data4(unsigned char val)
{  
  IO1CLR = (LCD_DATA);	  	// Reset 4-Bit Pin Data
  IO1SET = (val<<28);		// write data to P1.31-P1.28
}

/* Write Data 1 Byte to LCD */
void lcd_write_byte(unsigned char val)
{  
  lcd_out_data4((val>>4)&0x0F);		// Strobe 4-Bit High-Nibble to LCD
  enable_lcd();				// Enable Pulse
  
  lcd_out_data4(val&0x0F);		// Strobe 4-Bit Low-Nibble to LCD
  enable_lcd();				// Enable Pulse  

  while(busy_lcd());      		// Wait LCD Execute Complete  
}

/* Write Instruction to LCD */
void lcd_write_control(unsigned char val)
{ 
  lcd_rs_clr();				// RS = 0 = Instruction Select
  lcd_write_byte(val);			// Strobe Command Byte	    
}

/* Write Data(ASCII) to LCD */
void lcd_write_ascii(unsigned char c)
{  
  lcd_rs_set();				// RS = 1 = Data Select
  lcd_write_byte(c);		   	// Strobe 1 Byte to LCD    
}

/* Initial 4-Bit LCD Interface */
void lcd_init(void)
{
  unsigned int i;			// Delay Count
  PINSEL2 = 0;		      // P1[31..25] as GPIO
  IO1DIR |= 0xFE000000; // P1[31..25] = Output
  for (i=0;i<50000;i++);	// Power-On Delay (15 mS)

  IO1CLR = (LCD_IOALL);	     // Reset (4-Bit Data,EN,RW,RS) Pin
  IO1SET = (LCD_D5|LCD_D4);  // write 0011
  enable_lcd();		         // Enable Pulse
  for (i=0;i<10000;i++);    // Delay 4.1mS

  IO1CLR = (LCD_IOALL);	     // Reset (4-Bit Data,EN,RW,RS) Pin
  IO1SET = (LCD_D5|LCD_D4);  // write 0011
  enable_lcd();		          // Enable Pulse
  for (i=0;i<100;i++);	      // delay 100uS

  IO1CLR = (LCD_IOALL);	     // Reset (4-Bit Data,EN,RW,RS) Pin
  IO1SET = (LCD_D5|LCD_D4);  // write 0011
  enable_lcd();		          // Enable Pulse
  while(busy_lcd());         // Wait LCD Execute Complete
 
  IO1CLR = (LCD_IOALL);	   // Reset (4-Bit Data,EN,RW,RS) Pin
  IO1SET = (LCD_D5);	   // write 0010
  enable_lcd();		       // Enable Pulse
  while(busy_lcd());      // Wait LCD Execute Complete
  
  // Function Set (DL=0 4-Bit,N=1 2 Line,F=0 5X7)
  lcd_write_control(0x28);
  // Display on,Cursor on, Cursor not Blink)
  lcd_write_control(0x0C);
  // Mode (I/D=1 Increment,S=0 Cursor Shift)
  lcd_write_control(0x06);
  // Clear Display,Set DD RAM Address=0
  lcd_write_control(0x01);     
  for (i=0;i<100000;i++);     // Wait Command Ready
}

/* Set LCD Position Cursor */
void goto_cursor(unsigned char i)
{
  i |= 0x80;			// Set DD-RAM Address Command
  lcd_write_control(i);  
}

/* Print Display Data(ASCII) to LCD */
void lcd_print(unsigned char* str)
{
  int i;

  for (i=0;i<16 && str[i]!=0;i++)  	// 16 Character Print
  {
    lcd_write_ascii(str[i]);		// Print Byte to LCD
  }
}

/* Print Display Data(ASCII) to LCD */
void lcd_print_string(char* str)
{
  int i;

  for (i=0;i<16 && str[i]!=0;i++)  	// 16 Character Print
  {
    // Print Byte to LCD
    lcd_write_ascii((unsigned char) str[i]); 
  }
}

/* Wait LCD Ready */
char busy_lcd(void)
{
  delay(1000);
  return 0;
}

/* Enable Pulse to LCD */
void enable_lcd(void)	 		// Enable Pulse
{
  unsigned int i;			// Delay Count
  lcd_en_set();  			// Enable ON
  for (i=0;i<50;i++);
  lcd_en_clr();  			// Enable OFF 
}

/*    1-4294967296     */
void delay(unsigned long int count1)
{
  while(count1 > 0) {count1--;}		// Loop Decrease Counter	
}
