#ifndef __UART0_H
#define __UART0_H

/* initialize uart0 with appropriate baudrate */
extern void uart0_init(unsigned int baudrate);

/* put character to terminal 
 * printf() and fputc() will call this function
 */
extern int uart0_putchar(int);

/* get character from standard input */
extern int uart0_getchar(void);

/* print string to standard output */
extern int uart0_puts(char *);

/* print integer */
extern void uart0_print_int(int);

/* print floating point number */
extern void uart0_print_double(double ,int);

/* get multiple characters from standard input */
extern void uart0_getline(char *line, int n);


#endif // __UART0_H
