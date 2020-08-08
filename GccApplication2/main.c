#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "lcd_OneWireProtocol.h"

#define DHT11_PIN PIN6
#define MAXVIEW 2

uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;
float perc = 0;
char DHTdata[5];
int view = 0;

void printADC(uint16_t adc) {
	lcd_clear();
	perc = (1023 - adc) / 10;
	char adcStr[16];
	itoa(perc, adcStr, 10);
	lcd_print("Light = ");
	lcd_print(adcStr);
	lcd_print("%");
}

void printDHT() {
	lcd_clear();
	
	if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum) {
		lcd_gotoxy(0,0);
		lcd_print("DHT Error");
	} else {
		itoa(I_RH,DHTdata,10);
		lcd_print("Humidity = ");
		lcd_print(DHTdata);
		lcd_print(".");
		
		itoa(D_RH,DHTdata,10);
		lcd_print(DHTdata);
		lcd_print("%");

		itoa(I_Temp,DHTdata,10);
		lcd_gotoxy(0,1);
		lcd_print("Temp = ");
		lcd_print(DHTdata);
		lcd_print(".");
		
		itoa(D_Temp,DHTdata,10);
		lcd_print(DHTdata);
		lcddata(0xDF);
		lcd_print("C ");
		return;
	}
	return;
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

void printYL69(){
	lcd_clear();
	lcd_print("NO DATA FOR YOU!");
}

int main(void)
{
	DDRD = _BV(4);
	// DDRA = _BV(7); // buzzer
	PORTA = 0x80;
	DDRB = 0x00;
	PORTB = _BV(0) | _BV(1);
	
	
	TCCR1A = _BV(COM1B1) | _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	OCR1B = 24;

	lcdinit();
	lcd_clear();

	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);

	while (1) {
		
		
		/*// buzzer
		if(perc < 40) {
			PORTA ^= 0x80;
			_delay_ms(1000);
			PORTA ^= 0x80;
		}*/
		
		
		
		if (bit_is_clear(PINB, 0)){
			view--;
			if (view < 0){
				view = MAXVIEW;
			}
		} else if (bit_is_clear(PINB, 1)){
			view++;
			if (view > MAXVIEW){
				view = 0;
			}
		}
		
		switch (view) {
			case 0:
				ADCSRA |= _BV(ADSC);
				while (!(ADCSRA & _BV(ADIF)));
				printADC(ADC);
				break;
			case 1:
				Request();
				Response();
				I_RH     = Receive_data();
				D_RH     = Receive_data();
				I_Temp   = Receive_data();
				D_Temp   = Receive_data();
				CheckSum = Receive_data();
				printDHT();
				lcd_print("out");
				break;
			case 2:
				printYL69();
				break;
		}
		
		_delay_ms(1000);
	}
}