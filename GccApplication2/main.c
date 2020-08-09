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

float moistPerc = 0;
#define MOIST_MIN_PERC 10
#define MOIST_MAX_PERC 100
int moistFlag = 0;

#define DHT11_PIN PIN6
uint8_t c=0, I_RH, D_RH, I_Temp, D_Temp, CheckSum;

int stot  = 0;	// TODO possibly replace with TIMER0 SIGNALIZATION BUSY variable

void printADC(char *label, int value) {
	lcd_clear();
	char adcStr[16];
	itoa(value, adcStr, 10);
	lcd_print(label);
	lcd_print(adcStr);
	lcd_print("%");
}

void Request()						/* Microcontroller send start pulse or request */
{
	DDRD |= (1<<DHT11_PIN); // Request is sent from MCU PIN
	PORTD &= ~(1<<DHT11_PIN);		/* set to low pin, pull down */
	_delay_ms(20);					/* wait for 20ms */
	PORTD |= (1<<DHT11_PIN);		/* set to high pin, pull up */
}

void Response()						/* receive response from DHT11 */
{
	DDRD &= ~(1<<DHT11_PIN); // explicitly pull up PIN
	while(PIND & (1<<DHT11_PIN)); // check to see if state changed from high to low
	while((PIND & (1<<DHT11_PIN))==0); // check if pulled down voltage is equal to zero
	while(PIND & (1<<DHT11_PIN)); // check to see if state change from low to high
}

uint8_t Receive_data()							/* receive data */
{	
	/*
	The data frame is of total 40 bits long, it contains 5 segments and each segment
	is 8-bit long. We check each bit if it is high or low
	*/
	for (int q=0; q<8; q++)
	{
		while((PIND & (1<<DHT11_PIN)) == 0);/* check received bit 0 or 1, if pulled up */
		_delay_us(60);
		if(PIND & (1<<DHT11_PIN))				/* if high pulse is greater than 30ms */
		c = (c<<1)|(0x01);						/* then it is logic HIGH */
		else									/* otherwise it is logic LOW */
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
	nonBlockingDebounce();
}

/*	Signalize unfavorable conditions	*/
ISR(TIMER0_COMP_vect){
	stot--;
	if(stot %100==0) {
		PORTA ^= 0x80;
		_delay_ms(100);
		PORTA ^= 0x80;
	}
}

int main(void)
{
	/*	INIT LCD	*/
	DDRD = _BV(4);
	TCCR1A = _BV(COM1B1) | _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	OCR1B = 24;
	lcdinit();
	lcd_clear();
	
	/*
		INIT SIGNALIZATION
		Enable Timer0 interrupts on TIMSK only during unfavorable conditions
	*/
	TCCR0 = _BV(WGM01) | _BV(CS02) | _BV(CS00);
	OCR0 = 71;
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
		lightFlag = (lightPerc < LIGHT_MIN_PERC || lightPerc > LIGHT_MAX_PERC) ? 1 : 0;
		
		/*	Read from CH1 (soil moisture)	*/
		ADMUX ^= _BV(MUX0);
		ADCSRA ^= _BV(ADPS0);
		ADCSRA |= _BV(ADSC);
		while (!(ADCSRA & _BV(ADIF)));
		ADCSRA |= _BV(ADIF);
		moistPerc = 100 - (ADC * 100.00) / 1023.00;
		moistFlag = (moistPerc < MOIST_MIN_PERC || moistPerc > MOIST_MAX_PERC) ? 1 : 0;
		
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
		}
		DHT_delay++;

		/*	Display on LCD	*/
		switch (view) {
			case 0:
				printADC("Light = ", lightPerc);
				break;
			case 1:
				printDHT();
				break;
			case 2:
				printADC("SMS = ", moistPerc);
				break;
		}
		
		if (lightFlag || moistFlag) {
			if(stot == 0)
				stot = 100;
			TIMSK |= _BV(OCIE0);
		} else {
			TIMSK &= ~_BV(OCIE0);
		}
		
		_delay_ms(250);
	}
}