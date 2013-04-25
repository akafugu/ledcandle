#ifndef main_h
#define main_h

int main(void) __attribute__ ((noreturn));
void fade_in(void);
void delay(uint16_t ms);
void flicker(void);
void do_sleep(void);
void set_brightness(uint8_t value);
uint32_t rand(void);

#endif
