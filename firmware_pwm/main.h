#ifndef main_h
#define main_h

int main(void); 
void fade(uint8_t from, uint8_t to, uint8_t f_delay) __attribute__ ((noinline));
void delay(uint16_t ms) __attribute__ ((noinline));
void flicker(void);
void do_sleep(void);
uint32_t rand(void) __attribute__ ((noinline));

#endif
