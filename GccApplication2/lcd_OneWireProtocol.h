/*
 * LCD16x2_4bit.h
 */ 


#ifndef LCD16X2_4BIT_H_
#define LCD16X2_4BIT_H_

#define  F_CPU 7372800UL
#include <avr/io.h>
#include <util/delay.h>

#define LCD_DPRT PORTB
#define LCD_DDDR DDRB
#define LCD_RS 2
#define LCD_EN 3

void lcdcommand(unsigned char cmnd);
void lcddata(unsigned char data);
void lcdinit();
void lcd_gotoxy(unsigned char x, unsigned char y);
void lcd_print(char *str);
void lcd_clear();

#endif /* LCD16X2_4BIT_H_ */