#ifndef MORSE_CODE_H_
#define MORSE_CODE_H_

#define  F_CPU 7372800UL
#include <avr/io.h>
#include <util/delay.h>

#define MORSE_DDR DDRA
#define MORSE_PORT PORTA
#define MORSE_PIN 7

#define SHORT_MS 100
#define LONG_MS 500

void morse_init();
void morse_signal(uint16_t duration);
void morse_L();			// signalize light
void morse_M();			// signalize moisture
void morse_T();			// signalize temperature

#endif