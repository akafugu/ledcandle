#ifndef main_h
#define main_h

int main(void); 
void fade(uint8_t from, uint8_t to, uint8_t f_delay);
void delay(uint16_t ms);
void flicker(void);
void do_sleep(void);
void set_brightness(uint8_t value);
uint32_t rand(void);

#endif
