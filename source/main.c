/*	Author: skim370
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include "io.h"
#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
unsigned char tmpA = 0x00;
unsigned char count = 0x00;

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}



enum States{wait, addOne, minusOne, reset} state;

unsigned char button0;
unsigned char button1;

unsigned char tmpC;


void button_Tick(){
	button0 = ~PINA & 0x01;
	button1 = ~PINA & 0x02;
	
	switch(state){ // Transitions
		case wait:
			if(button0 && !button1){
				state = addOne;
			}
			else if(!button0 && button1){
				state = minusOne;
			}
			else if(button0 && button1){
				state = reset;
			}
			else{
			state = wait;
			}
			break;
		
		case addOne:
			if(button0 && !button1){
				state = addOne;
			}
			else if(button0 && button1){
				state = reset;
			}
		else
			state = wait;
			break;
		
		case minusOne:
			if(!button0 && button1){
				state = minusOne;
			}
			else if(button0 && button1){
				state = reset;
			}
			else
				state = wait;		
			break;
		
		case reset:
			if(button0 && button1){
				state = reset;
			}
			else
			state = wait;
			break;
		
		}
	switch(state){ // State actions
		case wait:
			break;
		
		case addOne:
			if(count < 9)
				count = count + 1;
			break;
		
		case minusOne:
			if(count > 0)
				count = count - 1;
			break;
	
		case reset:
			count = 0;
			break;
		
	}
}

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF; // A input initialize to 0xFF
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines

	TimerSet(600);
	TimerOn();
	
	// Initializes the LCD display
	LCD_init();
	LCD_ClearScreen();
	
	state = wait;
	count = 0x00;
	while(1){
		LCD_Cursor(1);
		button_Tick();
		LCD_WriteData(count + '0');
		while(!TimerFlag){}
		TimerFlag = 0;
	}
}
