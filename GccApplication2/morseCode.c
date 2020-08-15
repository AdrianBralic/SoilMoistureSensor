#include "morseCode.h"

void morse_init() {
	MORSE_DDR |= _BV(MORSE_PIN);
	MORSE_PORT |= _BV(MORSE_PIN);
}

void morse_signal(uint16_t duration) {
	MORSE_PORT ^= _BV(MORSE_PIN);
	
	for (uint16_t i = 0; i < duration; i++)
		 _delay_ms(1);
	
	MORSE_PORT ^= _BV(MORSE_PIN);
	_delay_ms(SHORT_MS);
}

void morse_L() {
	morse_signal(SHORT_MS);
	morse_signal(LONG_MS);
	morse_signal(SHORT_MS);
	morse_signal(SHORT_MS);
	_delay_ms(LONG_MS);
}

void morse_T() {
	morse_signal(LONG_MS);
	_delay_ms(LONG_MS);
}

void morse_H() {
	morse_signal(SHORT_MS);
	morse_signal(SHORT_MS);
	morse_signal(SHORT_MS);
	morse_signal(SHORT_MS);
	_delay_ms(LONG_MS);
}

void morse_1() {
	morse_signal(SHORT_MS);
	morse_signal(LONG_MS);
	morse_signal(LONG_MS);
	morse_signal(LONG_MS);
	morse_signal(LONG_MS);
	_delay_ms(LONG_MS);
}

void morse_2() {
	morse_signal(SHORT_MS);
	morse_signal(SHORT_MS);
	morse_signal(LONG_MS);
	morse_signal(LONG_MS);
	morse_signal(LONG_MS);
	_delay_ms(LONG_MS);
}