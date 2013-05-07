/*
 * Original firmware
 * =================
 *
 * Flickering LED Candle
 * (C) 2012 Akafugu Corporation
 *
 *
 * Alternative firmware with 8-bit PWM
 * ===================================
 *
 * 2013 - Robert Spitzenpfeil
 *
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

/*
 * Pinout:
 * PB0 (pin 5) - Button (INT0 interrupt)
 * PB1 (pin 6) - LED (PWM)
 * PB2 (pin 7) - LED
 * PB3 (pin 2) - LED
 * PB4 (pin 3) - LED
 * PB5 (pin 1) - Reset
 *
 * 1k resistor per pin. Suitable for running from a 3V CR2032 cell
 *
 *
 * Inspired by similar LED candle projects on Instructables:
 *
 * http://www.instructables.com/id/YAFLC-Yet-Another-Flickering-LED-Candle/
 * http://www.instructables.com/id/Realistic-Fire-Effect-with-Arduino-and-LEDs/
 *
 */

//#define DEBUG

#ifdef DEBUG
	#define PORT_DIR_REG DDRB
	#define PORT_OUT_REG PORTB
	#define PORT_IN_REG PINB
	#define BUTTON_PIN PB2
	// LED sits on PB0
	#define LED_MASK 0b00000001
	// PB0 as output, rest input
	#define PORT_DIR_MASK 0b00000001
	// pin-change mask
	#define PINC_MASK 0b00000100
#else
	#define PORT_DIR_REG DDRB
	#define PORT_OUT_REG PORTB
	#define PORT_IN_REG PINB
	#define BUTTON_PIN PB0
	// LEDs sit on PB4...PB1
	#define LED_MASK 0b00011110
	// PB4...PB1 as output, rest input
	#define PORT_DIR_MASK 0b00011110
	// pin-change mask
	#define PINC_MASK 0b00000001
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>
#include "main.h"

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

// defines for on time (in seconds)
#define ON_1H 3600
#define ON_2H 7200
#define ON_3H 10800
#define ON_4H 14400
#define ON_5h 18000

// how long to stay on
uint16_t off_timer = ON_2H;

// random number seed (will give same flicker sequence each time)
uint32_t lfsr = 0xbeefcace;

volatile uint8_t brightness = 0; // not changed in an ISR, but prevent optimizer to throw it out
volatile uint8_t sleep_requested = 0;

int main(void)
{
	// Attiny13A fuse setting: -U lfuse:w:0x7A:m -U hfuse:w:0xFB:m
	//
	// set system-clock prescaler to 1/8
	// 4.8MHz RC-oscillator --> 600kHz system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS1) | _BV(CLKPS0);

	// configure TIMER0
	TCCR0A = _BV(WGM01);	// set CTC mode
	TCCR0B = ( _BV(CS01) );	// set prescaler to 8
	// enable COMPA isr
	TIMSK0 = _BV(OCIE0A);
	// set top value for TCNT0
	OCR0A = 10;		// just some start value

	// pull-up on for BUTTON_PIN
	PORT_OUT_REG |= _BV(BUTTON_PIN);
	// set port directions
	PORT_DIR_REG = PORT_DIR_MASK;
	
	// disable analog comparator to save power
	ACSR = _BV(ACD);

	// Pin change interrupt enabled
	// The actual pin will be activated later
	GIMSK |= _BV(PCIE);
		
	// globally enable interrupts
	// necessary to wake up from sleep via pin-change interrupt
	sei();

	fade(0,255,1);

	while (1) {
		if(sleep_requested == 1) {
			do_sleep();
		}
		flicker();
	}
}

uint32_t rand(void)
{
	// http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	// Galois LFSR: taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 
	lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
	return lfsr;
}

// Enter sleep mode: Wake on INT0 interrupt
void do_sleep(void)
{
	cli();
	sleep_requested = 0;

	// now we want to make sure the button is not pressed and is stable (not bouncing)
	// switch bouncing is an unwanted wake-up source

	#ifdef DEBUG
		PORT_OUT_REG |= LED_MASK; // all ON - current source
	#else
		PORT_OUT_REG &= ~LED_MASK; // all ON - current sink
	#endif

	while( !(PORT_IN_REG & _BV(BUTTON_PIN)) ) { // while button is pressed (LOW)
		delay(500);
	}

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();

	sei();
	fade(255,0,1);
	GIFR &= ~_BV(PCIF);
	PCMSK = PINC_MASK; // 'remove the safety' on the pin-change interrupt
	sleep_cpu();
	sleep_disable();
	fade(0,255,1);	// wake up here again
}

// Interrupt signal PCINT0: On PB0 (PB2 for DEBUG, different board)
ISR(PCINT0_vect)
{
	PCMSK = 0; // clear PCINT-mask to disable pin-change interrupt
	GIFR |= _BV(PCIF); // clear PCIF
}

// Flicker by randomly setting pins PB1~PB4 on and off
void flicker(void)
{
	uint8_t flicker_brightness = (uint8_t)(rand()); // user lowermost 8 bits to set values for the LED brightness

	fade(brightness,16, (uint8_t)(rand() >> 30) );
	delay( (uint8_t)(rand() >> 24) );
	fade(16,flicker_brightness, (uint8_t)(rand() >> 30) );
	delay( (uint8_t)(rand() >> 24) );
}

ISR(TIM0_COMPA_vect)
{
	static uint16_t bitmask = 0b0000000000000001;
	static uint16_t sec_counter = 0;
	static uint16_t PWM_cycle_counter = 0;
	static uint16_t button_held_counter = 0;
	uint8_t OCR0A_next;

	// increase system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = 0;		// set system-clock prescaler to 1 --> full 9.6MHz

	// Binary-weighted PWM generation - BEGIN

	if ( brightness & bitmask ) { // turn ON
		#ifdef DEBUG
			PORT_OUT_REG |= LED_MASK; // source current
		#else
			PORT_OUT_REG &= ~LED_MASK; // sink current
		#endif
	} else { // turn OFF
		#ifdef DEBUG
			PORT_OUT_REG &= ~LED_MASK;
		#else
			PORT_OUT_REG |= LED_MASK;
		#endif
	}

	OCR0A_next = bitmask;
	bitmask = bitmask << 1;

	if (bitmask == _BV(9)) {
		bitmask = 0x0001;
		OCR0A_next = 2;
		PWM_cycle_counter++;
	}

	OCR0A = OCR0A_next;	// when to run next time
	TCNT0 = 0;		// clear timer to compensate for code runtime above
	TIFR0 = _BV(OCF0A);	// clear interrupt flag to kill any erroneously pending interrupt in the queue

	// PWM generation - END

	if (PWM_cycle_counter == 293) {	// count seconds
		sec_counter++;
		PWM_cycle_counter = 0;
	}

	if (PORT_IN_REG & _BV(BUTTON_PIN)) {	// button is not held
		button_held_counter = 0;
	} else {		// button is held
		button_held_counter++;
	}

	// time to go to sleep?
	if (off_timer != 0 && sec_counter >= off_timer) {
		sec_counter = 0;
		sleep_requested = 1;
	}
	// holding the button for 3 seconds turns the device off
	if (button_held_counter == 7032) {
		sec_counter = 0;
		sleep_requested = 1;
	}

	// decrease system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS1) | _BV(CLKPS0);	// set system-clock prescaler to 1/8 --> 4.8 MHz / 8 = 600kHz
}

void delay(uint16_t ms) {
	while(ms) {
		_delay_ms(1);
		ms--;
	}
}

void fade(uint8_t from, uint8_t to, uint8_t f_delay) {
	uint8_t counter;

	if(from <= to) {
		for(counter = from; counter < to; counter++) {
			brightness = counter;
			delay(f_delay);
		}
	}

	if(from > to) {
		for(counter = from; counter > to; counter--) {
			brightness = counter;
			delay(f_delay);
		}
	}
}
