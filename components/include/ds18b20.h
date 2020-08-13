#ifndef _APP_TERM_
#define _APP_TERM_

#include <stdint.h>

#define DS18B20LOW		0 
#define DS18B20HIGH		1

typedef enum{
	DS18B20_OK		= 0,
	DS18B20_NOT_OK	= 1
} err_ds18b20_t;

void ds18b20_write(uint8_t * word, uint8_t word_len);
void ds18b20_read(uint8_t word[], uint8_t size);

void ds18b20_gpio_init(void);

err_ds18b20_t ds18b20_reset(void);
void ds18b20_skip_rom(void);
void ds18b20_read_rom(uint8_t * word);
void ds18b20_write_scratchpad(uint8_t * word);
void ds18b20_read_scratchpad(uint8_t * word, uint8_t bytes_to_read);
void ds18b20_convert(void);

float ds18b20_temp_to_float(uint8_t * word, uint8_t bits);

#endif
