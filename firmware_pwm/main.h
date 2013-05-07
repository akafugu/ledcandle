#ifndef main_h
#define main_h

// defines for on time (in seconds)
#define ON_1H 3600
#define ON_2H 7200
#define ON_3H 10800
#define ON_4H 14400
#define ON_5h 18000

#ifdef DEBUG
	// A different dev-board was used to develop the firmware
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
	// Values for the original Akafugu LED candle hardware
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

// not used, but nice to have
#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

int main(void); 
void fade(uint8_t from, uint8_t to, uint8_t f_delay) __attribute__ ((noinline));
void delay(uint16_t ms) __attribute__ ((noinline));
void flicker(void);
void do_sleep(void);
uint32_t rand(void) __attribute__ ((noinline));

#endif
