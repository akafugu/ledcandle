/*
 * Flickering LED Candle
 * (C) 2012 Akafugu Corporation
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

#define BUTTON_PORT PORTB
#define BUTTON_DDR  DDRB
#define BUTTON_PIN  PINB
#define BUTTON_BIT PB0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

// button control
volatile bool button_held = false;

// defines for on time (in seconds)
#define ON_1H 3600
#define ON_2H 7200
#define ON_3H 10800
#define ON_4H 14400
#define ON_5h 18000

// for counting seconds
volatile uint16_t int_counter;
volatile uint16_t sec_counter;

// how long to stay on
volatile uint16_t off_timer = ON_2H;

// for hold to turn off feature
volatile uint16_t button_held_counter; // how long has button been held?
volatile bool off_flag = false; // has the button been held long enough to turn off?

void led_off(void)
{
	PORTB = 0xff;
	DDRB = 0b11111110;
}

void led_on(void)
{
	PORTB = 0x1; // keep pull-up for button set
	DDRB = 0b11111110;
}

// random number seed (will give same flicker sequence each time)
volatile uint32_t lfsr = 0xbeefcace;

uint32_t rand(void)
{
	// http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	// Galois LFSR: taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
  	lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
	return lfsr;
}

// Enter sleep mode: Wake on INT0 interrupt
void do_sleep(void)
{
	led_off();

	GIMSK |= (1<<PCIE); // Pin change interrupt enabled
	PCMSK |= 1<<PCINT0;

	sei();
	set_sleep_mode(_BV(SM1)); // Power down sleep mode (sets MCUCR)
	sleep_mode();

	off_timer = ON_2H;
}

// Interrupt signal PCINT0: On PB0
ISR (PCINT0_vect)
{
	cli();
}

// Flicker by randomly setting pins PB1~PB4 on and off
void flicker(void)
{
	uint32_t r;
	uint8_t temp;

	// Inititalize timer
	TCCR0B = (1<<CS01); // Set Prescaler to clk/8 : 1 click = aprox 2us (using 4.6MHz internal clock)
	TIMSK0 |= (1<<TOIE0); // Enable Overflow Interrupt Enable
	TCNT0 = 0; // Initialize counter

	// LED off
	PORTB = 0xff;
	// PB0 as input
	DDRB = 0b11111110;
	// enable PB0 pull-up
	BUTTON_PORT |= _BV(BUTTON_BIT);

	GIMSK |= (1<<PCIE); // Pin change interrupt enabled
	PCMSK |= 1<<PCINT0;

	while(1)
	{
		r = rand();

		if (off_flag) {
			led_off();
		}
		else {
			temp = (uint8_t)r; // use lowermost 8 bits to set values for the LED pins

			// set PB1~PB4 according to value of temp
			if (temp & 1) { // off
				cbi(DDRB, PB1);
				sbi(PORTB, PB1);
			}
			else { // on
				sbi(DDRB, PB1);
				cbi(PORTB, PB1);
			}

			if (temp & (1<<1)) { // off
				cbi(DDRB, PB2);
				sbi(PORTB, PB2);
			}
			else { // on
				sbi(DDRB, PB2);
				cbi(PORTB, PB2);
			}
			
			if (temp & (1<<2)) { // off
				cbi(DDRB, PB3);
				sbi(PORTB, PB3);
			}
			else { // on
				sbi(DDRB, PB3);
				cbi(PORTB, PB3);
			}

			// Keep at least one pin on at all times to keep LED from turning off completely
			sbi(DDRB, PB4);
			cbi(PORTB, PB4);
		}

		// Use top byte to control delay between each intensity change
		// Tweaking these numbers will affect the speed of the flickering
		temp = (uint8_t) (r >> 24);
		_delay_loop_2(temp<<7);
	} 
}

// 1 click = aprox 2us*8 = 16us
// interrupt runs every 256 clicks = every 4096 us = 0.004096 seconds
// 244 interrupts = aprox 1 second ideally
// actual value tweaked to match CPU load
ISR(TIM0_OVF_vect)
{
	if (++int_counter == 285) { // count seconds
		sec_counter++;
		int_counter = 0;
	}
	
	if (BUTTON_PIN & _BV(BUTTON_BIT)) { // button is not held
		button_held = false;
		button_held_counter = 0;
	}
	else { // button is held
		button_held = true;
		button_held_counter++;
	}

	// time to go to sleep?
	if (off_timer != 0 && sec_counter >= off_timer && !button_held) {
		sec_counter = 0;
		off_flag = false;
		button_held_counter = 0;
		do_sleep();
		return;
	}
	
	// holding the button for 3 seconds turns the device off
	if (button_held_counter == 570) {
		off_flag = true;
		return;
	}
	
	// when off flag is set, wait for key up to go to sleep (otherwise the interrupt pin will wake the device up again)
	if (off_flag && !button_held) {
		off_flag = false;
		do_sleep();
	}
}

void main(void) __attribute__ ((noreturn));

void main(void)
{
	sei();
	flicker();
	while(1) {}
}


