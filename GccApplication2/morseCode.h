#ifndef MORSE_CODE_H_
#define MORSE_CODE_H_

#define  F_CPU 7372800UL
#include <avr/io.h>
#include <util/delay.h>

#define MORSE_DDR DDRA
#define MORSE_PORT PORTA
#define MORSE_PIN 7

#define SHORT_MS 100
#define LONG_MS 300

void morse_init();
void morse_signal(uint16_t duration);
void morse_L();			// signalize light
void morse_T();			// signalize temperature
void morse_H();			// signalize humidity
void morse_1();			// signalize moisture on sensor 1
void morse_2();			// signalize moisture on sensor 2

#endif