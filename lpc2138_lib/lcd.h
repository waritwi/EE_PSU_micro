#include <LPC213X.h>

// Define LCD PinIO Mask 
// xxxx xxx0 0000 0000 0000 0000 0000 0000
#define  LCD_RS     0x02000000 
#define  LCD_RW     0x04000000
#define  LCD_EN     0x08000000
#define  LCD_D4     0x10000000
#define  LCD_D5     0x20000000
#define  LCD_D6     0x40000000
#define  LCD_D7     0x80000000
	
#define  LCD_DATA   (LCD_D7|LCD_D6|LCD_D5|LCD_D4)
#define  LCD_IOALL  (LCD_D7|LCD_D6|LCD_D5|LCD_D4|LCD_EN|LCD_RW|LCD_RS)

//#define  lcd_dir_write()  IODIR |= 0xFE000000	// LCD Data Bus = Write
//#define  lcd_dir_read()   IODIR &= 0x07FFFFFF	// LCD Data Bus = Read 

#define  lcd_rs_set() IO1SET = LCD_RS	 	// RS = 1 (Select Instruction)
#define  lcd_rs_clr() IO1CLR = LCD_RS		// RS = 0 (Select Data)
#define  lcd_rw_set() IO1SET = LCD_RW		// RW = 1 (Read)
#define  lcd_rw_clr() IO1CLR = LCD_RW		// RW = 0 (Write)
#define  lcd_en_set() IO1SET = LCD_EN		// EN = 1 (Enable)
#define  lcd_en_clr() IO1CLR = LCD_EN		// EN = 0 (Disable)

#define  lcd_clear()          lcd_write_control(0x01)	// Clear Display
#define  lcd_cursor_home()    lcd_write_control(0x02)	// Set Cursor = 0
#define  lcd_display_on()     lcd_write_control(0x0E)	// LCD Display Enable
#define  lcd_display_off()    lcd_write_control(0x08)	// LCD Display Disable
#define  lcd_cursor_blink()   lcd_write_control(0x0F)	// Set Cursor = Blink
#define  lcd_cursor_on()      lcd_write_control(0x0E)	// Enable LCD Cursor
#define  lcd_cursor_off()     lcd_write_control(0x0C)	// Disable LCD Cursor
#define  lcd_cursor_left()    lcd_write_control(0x10)	// Shift Left Cursor
#define  lcd_cursor_right()   lcd_write_control(0x14)	// Shift Right Cursor
#define  lcd_display_sleft()  lcd_write_control(0x18)	// Shift Left Display
#define  lcd_display_sright() lcd_write_control(0x1C)	// Shift Right Display
#define  lcd_goto_first_row()   lcd_write_control(0x80) // Goto first row
#define  lcd_goto_second_row()  lcd_write_control(0xC0) // Goto second row



/* pototype  section */
extern void lcd_init(void);				// Initial LCD
extern void lcd_out_data4(unsigned char);		// Strobe 4-Bit Data to LCD
extern void lcd_write_byte(unsigned char);		// Write 1 Byte Data to LCD
extern void lcd_write_control(unsigned char); 		// Write Instruction
extern void lcd_write_ascii(unsigned char); 		// Write LCD Display(ASCII)
extern void goto_cursor(unsigned char);		// Set Position Cursor LCD
extern void lcd_print(unsigned char*);			// Print Display to LCD
extern void lcd_print_string(char*); // Print string
extern char busy_lcd(void);				// Read Busy LCD Status
extern void enable_lcd(void);	 			// Enable Pulse
extern void delay(unsigned long int);			// Delay Function


