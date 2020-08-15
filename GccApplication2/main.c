#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "morseCode.h"

#define MAXVIEW 3
int view = 0;

float lightPerc = 0;
#define LIGHT_MIN_PERC 10
#define LIGHT_MAX_PERC 100
int lightFlag = 0;
int nightMode = 0;

float moistPerc1 = 0;
float moistPerc2 = 0;
#define MOIST_MIN_PERC 10
#define MOIST_MAX_PERC 95
int moistFlag1 = 0;
int moistFlag2 = 0;

#define T_MIN 18
#define T_MAX 38
int tempFlag = 0;

#define H_MIN 15
#define H_MAX 100
int humFlag = 0;

#define DHT11_PIN PIN6
uint8_t c = 0, I_RH, D_RH, I_Temp, D_Temp, CheckSum;

void printADC(char *label, int value) {
	char adcStr[16];
	itoa(value, adcStr, 10);
	lcd_print(label);
	lcd_print(adcStr);
	lcd_print("%");
}

void printLight() {
	lcd_clear();
	printADC("Light = ", lightPerc);
	lcd_gotoxy(0, 1);
	if (!nightMode) {
		lcd_print("Day mode");
	} else {
		lcd_print("Night mode");
	}
}

void printMoisture() {
	lcd_clear();
	printADC("Moisture1 = ", moistPerc1);
	lcd_gotoxy(0, 1);
	printADC("Moisture2 = ", moistPerc2);
}

void Request() {
	DDRD |= (1<<DHT11_PIN); 
	PORTD &= ~(1<<DHT11_PIN);		
	_delay_ms(20);					
	PORTD |= (1<<DHT11_PIN);		
}

void Response()	{
	DDRD &= ~(1<<DHT11_PIN);
	while(PIND & (1<<DHT11_PIN)); 
	while((PIND & (1<<DHT11_PIN))==0);
	while(PIND & (1<<DHT11_PIN));
}

uint8_t Receive_data() {	
	for (int q=0; q<8; q++) {
		while((PIND & (1<<DHT11_PIN)) == 0);
		_delay_us(60);
		if(PIND & (1<<DHT11_PIN))				
		c = (c<<1)|(0x01);						
		else									
		c = (c<<1);
		while(PIND & (1<<DHT11_PIN));
	}
	return c;
}

void printDHT() {
	lcd_clear();
	
	if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum) {
		lcd_print("DHT Error");
		return;
	}

	char DHTdata[5];
	
	itoa(I_RH, DHTdata, 10);
	lcd_print("Humidity = ");
	lcd_print(DHTdata);
	lcd_print(".");
	
	itoa(D_RH, DHTdata, 10);
	lcd_print(DHTdata);
	lcd_print("%");

	itoa(I_Temp, DHTdata, 10);
	lcd_gotoxy(0,1);
	lcd_print("Temp = ");
	lcd_print(DHTdata);
	lcd_print(".");
	
	itoa(D_Temp, DHTdata, 10);
	lcd_print(DHTdata);
	lcddata(0xDF);
	lcd_print("C");	
}

void nonBlockingDebounce() {
	GICR &= ~_BV(INT0);
	sei();

	_delay_ms(250);
	GIFR = _BV(INTF0);
	GICR |= _BV(INT0);
	
	cli();
}

/*	Change LCD display view on INT0	*/
ISR(INT0_vect) {
	view = (view + 1) % MAXVIEW;
	nonBlockingDebounce();
}

/*
	Toggle day-mode/night-mode on INT1
	Night-mode ignores light threshold
*/
ISR(INT1_vect) {
	nightMode = nightMode ? 0 : 1;
	nonBlockingDebounce();
}

/*	Signalize unfavorable conditions	*/
void beeper(){
	if (lightFlag)
		morse_L();
	if (tempFlag)
		morse_T();
	if (humFlag)
		morse_H();
	if (moistFlag1)
		morse_1();
	if(moistFlag2)
		morse_2();
}

int main(void)
{
	/*	INIT LCD	*/
	DDRD = _BV(4);
	TCCR1A = _BV(COM1B1) | _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	lcdinit();
	lcd_clear();
	
	/*	INIT SIGNALIZATION	*/
	morse_init();
	
	/*	INIT INTERRUPTS	*/
	MCUCR = _BV(ISC01) | _BV(ISC11);
	GICR = _BV(INT1) | _BV(INT0);
	sei();

	/*	INIT ADC	*/
	ADMUX = _BV(REFS0) | _BV(MUX0);
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

	int DHT_delay = 0;
	
	while (1) {
		
		/*	Read from CH0 (photo-resistor)	*/
		ADMUX ^= _BV(MUX0);
		ADCSRA ^= _BV(ADPS0);
		ADCSRA |= _BV(ADSC);
		while (!(ADCSRA & _BV(ADIF)));
		lightPerc = (1023 - ADC) * 100.00 / 1023.00;
		if (nightMode) {
			lightFlag = (lightPerc > LIGHT_MAX_PERC) ? 1 : 0;
		} else {
			lightFlag = (lightPerc < LIGHT_MIN_PERC || lightPerc > LIGHT_MAX_PERC) ? 1 : 0;
		}
		
		/*	Read from CH1 (soil moisture)	*/
		ADMUX ^= _BV(MUX0);
		ADCSRA ^= _BV(ADPS0);
		ADCSRA |= _BV(ADSC);
		while (!(ADCSRA & _BV(ADIF)));
		ADCSRA |= _BV(ADIF);
		moistPerc1 = 100 - (ADC * 100.00) / 1023.00;
		moistFlag1 = (moistPerc1 < MOIST_MIN_PERC || moistPerc1 > MOIST_MAX_PERC) ? 1 : 0;
		
		ADMUX ^= _BV(MUX0) | _BV(MUX1);
		ADCSRA |= _BV(ADSC);
		while (!(ADCSRA & _BV(ADIF)));
		ADCSRA |= _BV(ADIF);
		moistPerc2 = 100 - (ADC * 100.00) / 1023.00;
		moistFlag2 = (moistPerc2 < MOIST_MIN_PERC || moistPerc2 > MOIST_MAX_PERC) ? 1 : 0;
		ADMUX ^= _BV(MUX0) | _BV(MUX1);
		
		/*
			Read from DHT (humidity and temperature)
			Read once every 5s
		*/
		if (DHT_delay % 20 == 0) {
			Request();
			Response();
			I_RH     = Receive_data();
			D_RH     = Receive_data();
			I_Temp   = Receive_data();
			D_Temp   = Receive_data();
			CheckSum = Receive_data();
			
			tempFlag = (I_Temp < T_MIN || I_Temp > T_MAX) ? 1 : 0;
			humFlag = (I_RH < H_MIN || I_RH > H_MAX) ? 1 : 0;
		}
		DHT_delay++;

		/*	Display on LCD	*/
		switch (view) {
			case 0:
				printLight();
				break;
			case 1:
				printDHT();
				break;
			case 2:
				printMoisture();
				break;
		}
		
		beeper();
		_delay_ms(250);
	}
}