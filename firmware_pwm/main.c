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

// how long to stay on
uint16_t off_timer = ON_2H;

// for hold to turn off feature
volatile bool off_flag = false;	// has the button been held long enough to turn off?

// for flickering intensity
volatile bool fast_flicker = true;

// random number seed (will give same flicker sequence each time)
uint32_t lfsr = 0xbeefcace;

uint32_t rand(void)
{
	// http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	// Galois LFSR: taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
	lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
	return lfsr;
}

uint8_t brightness = 0;

void set_brightness(uint8_t value)
{
	brightness = value;
}

// Enter sleep mode: Wake on INT0 interrupt
void do_sleep(void)
{
	set_brightness(0);

	GIMSK |= (1 << PCIE);	// Pin change interrupt enabled
	PCMSK |= 1 << PCINT0;

	set_sleep_mode(_BV(SM1));	// Power down sleep mode (sets MCUCR)
	sleep_mode();

	off_timer = ON_2H;
}

// Interrupt signal PCINT0: On PB0
ISR(PCINT0_vect)
{
	// disable pin-change interrupt, so switch-bounce doesn't kill us
	GIMSK &= ~_BV(PCIE);
}

// Flicker by randomly setting pins PB1~PB4 on and off
void flicker(void)
{
	uint32_t r;
	uint8_t temp;

	r = rand();

	if (off_flag) {
		set_brightness(0);
	} else {
		temp = (uint8_t) r;	// use lowermost 8 bits to set values for the LED pins
		set_brightness(temp);
	}

	// Use top byte to control delay between each intensity change
	// Tweaking these numbers will affect the speed of the flickering
	temp = (uint8_t) (r >> 24);
	_delay_loop_2(temp << 7);
}

ISR(TIM0_COMPA_vect)
{
	static uint16_t bitmask = 0x0001;
	static uint16_t sec_counter = 0;
	static uint16_t PWM_cycle_counter = 0;
	static uint16_t button_held_counter = 0;
	uint8_t OCR0A_next;

	// save TIMER0 prescaler
	uint8_t TCCR0B__saved = TCCR0B;
	// stop TIMER0
	TCCR0B = 0;
	// increase system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = 0;		// set system-clock prescaler to 1 --> full 9.6MHz

	if (brightness & bitmask) {
		PORTB |= 0b00011110;	// PB1...PB4
	} else {
		PORTB &= ~(0b00011110);	// PB1...PB4
	}

	OCR0A_next = bitmask;
	bitmask = bitmask << 1;

	if (bitmask == 9) {
		bitmask = 0x0001;
		OCR0A_next = 2;
		PWM_cycle_counter++;
	}

	OCR0A = OCR0A_next;	// when to run next time
	TCNT0 = 0;		// clear timer to compensate for code runtime above
	TIFR0 = _BV(OCF0A);	// clear interrupt flag to kill any erroneously pending interrupt in the queue

	if (PWM_cycle_counter == 293) {	// count seconds
		sec_counter++;
		PWM_cycle_counter = 0;
	}

	if (PINB & _BV(PB0)) {	// button is not held
		if (button_held_counter > 0) {	// button up detected
			fast_flicker = !fast_flicker;
		}

		button_held = false;
		button_held_counter = 0;
	} else {		// button is held
		button_held = true;
		button_held_counter++;
	}

	// time to go to sleep?
	if (off_timer != 0 && sec_counter >= off_timer && !button_held) {
		sec_counter = 0;
		off_flag = false;
		button_held_counter = 0;
		do_sleep();
	}
	// holding the button for 3 seconds turns the device off
	if (button_held_counter == 879) {
		off_flag = true;
	}
	// when off flag is set, wait for key up to go to sleep (otherwise the interrupt pin will wake the device up again)
	if (off_flag && !button_held) {
		off_flag = false;
		do_sleep();
	}
	// decrease system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS2);	// set system-clock prescaler to 1/16 --> 9.6MHz / 16 = 600kHz
	// restore TIMER0 prescaler
	TCCR0B = TCCR0B__saved;
}

void main(void) __attribute__ ((noreturn));

void main(void)
{
	// set system-clock prescaler to 1/16
	// 9.6MHz RC-oscillator --> 600kHz system-clock
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS2);

	// configure TIMER0
	TCCR0A = _BV(WGM01);	// set CTC mode
	TCCR0B = _BV(CS01);	// set prescaler to 8
	// enable COMPA isr
	TIMSK0 = _BV(OCIE0A);
	// set top value for TCNT0
	OCR0A = 10;		// just some start value

	// pull-up on PB0
	PORTB |= _BV(PB0);
	// PB0 as input, PB1...PB4 as output
	DDRB = 0b00011110;

	// globally enable interrupts
	// necessary to wake up from sleep via pin-change interrupt
	sei();

	while (1) {
		flicker();
	}
}
