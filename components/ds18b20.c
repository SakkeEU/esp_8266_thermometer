#include <stdio.h>
#include <string.h>
#include <math.h>
#include "thermo.h"
#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "driver/gpio.h"

static gpio_num_t pin;

static err_ds18b20_t ds18b20_wait(uint8_t wait_time, uint8_t wait_level){
	
	uint8_t level = 0;
	wait_time += wait_time % 5;
	
	//wait until level == wait_level or wait_time == 0
	gpio_set_direction(pin, GPIO_MODE_INPUT);
	for(; wait_time > 0; wait_time -= 5){
		level = gpio_get_level(pin);
		if(level != wait_level){
			ets_delay_us(5);
			continue;
		}else
			return DS18B20_OK;
	}
	return DS18B20_NOT_OK;
}

static void ds18b20_write_bit(uint8_t bit){
	
	//write time slot 15us < ts < 60us
	gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	if(bit){
		taskENTER_CRITICAL();
			gpio_set_level(pin, 0);
			ets_delay_us(5);
			
			gpio_set_level(pin, 1);
			ets_delay_us(60);
		taskEXIT_CRITICAL();
	}else{
		taskENTER_CRITICAL();
			gpio_set_level(pin, 0);
			ets_delay_us(65);
			
			gpio_set_level(pin, 1);
			ets_delay_us(2);
		taskEXIT_CRITICAL();
	}
}

static void ds18b20_write_byte(uint8_t byte){
	
	for(uint8_t i = 0x01; i; i <<= 1){
		if(byte & i){
			ds18b20_write_bit(1);
		}
		else{
			ds18b20_write_bit(0);
		}
	}
}

static uint8_t ds18b20_read_bit(void){
	
	uint8_t bit = 0;
	
	//read time slot 1us < ts < 15us
	//then recovery time until 60us
	gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	taskENTER_CRITICAL();
		gpio_set_level(pin, 0);
		ets_delay_us(2);
		gpio_set_level(pin, 1);
		
		gpio_set_direction(pin, GPIO_MODE_INPUT);
		ets_delay_us(10);
		bit = gpio_get_level(pin);
	taskEXIT_CRITICAL();
	
	ets_delay_us(55);
	return bit;
}

static uint8_t ds18b20_read_byte(void){
	
	uint8_t byte = 0;
	for(uint8_t i = 0x01; i; i <<= 1){
		if(ds18b20_read_bit())
			byte |= i;
	}
	return byte;
}

void ds18b20_write(uint8_t * word, uint8_t word_len){
	
	for(uint8_t i = 0; i < word_len; i++){
		ds18b20_write_byte(word[i]);
	}
}


void ds18b20_read(uint8_t * word, uint8_t word_len){
	
	for(uint8_t i = 0; i < word_len; i++){
		word[i] = ds18b20_read_byte();
	}
}

void ds18b20_gpio_init(void){
	
	pin = GPIO_NUM_0;
	gpio_config_t conf = {
		.pin_bit_mask 	= (1 << pin),
		.mode 			= GPIO_MODE_INPUT,
		.pull_up_en 	= GPIO_PULLUP_DISABLE,
		.pull_down_en 	= GPIO_PULLDOWN_DISABLE,
		.intr_type 		= GPIO_INTR_DISABLE
	};
	ESP_ERROR_CHECK(gpio_config(&conf));
}

err_ds18b20_t ds18b20_reset(void){
	
	err_ds18b20_t err = DS18B20_OK;
	
	ESP_LOGD(TAG_DS18B20, "ds18b20_reset");
	//wait for the level to go up after gpio init
	if(ds18b20_wait(250, DS18B20HIGH))
		return DS18B20_NOT_OK;
	
	gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
	//reset ds18b20 with a low level of 480us min duration
	//and wait for presence pulse
	taskENTER_CRITICAL();
		gpio_set_level(pin, 0);
		ets_delay_us(500);
		
		gpio_set_level(pin, 1);
		err = ds18b20_wait(80, DS18B20LOW);
	taskEXIT_CRITICAL();
	
	//wait for presence pulse to end (240us max duration)
	ds18b20_wait(250, DS18B20HIGH);
	return err;
}

void ds18b20_skip_rom(void){
	
	uint8_t word[] = {0xCC};
	ESP_LOGD(TAG_DS18B20, "skip_rom: CCh");
	ds18b20_write(word, 1);
}

void ds18b20_read_rom(uint8_t * word){
	
	word[0] = 0x33;
	ESP_LOGD(TAG_DS18B20, "read_rom: 33h");
	ds18b20_write(word, 1);
	ds18b20_read(word, 8);
}

//bytes order TH - TL - CONFIG
void ds18b20_write_scratchpad(uint8_t * word){
	
	uint8_t cmd = 0x4E;
	uint8_t * ptr_cmd = &cmd;
	ESP_LOGD(TAG_DS18B20, "write_scratchpad: 4Eh");
	ds18b20_write(ptr_cmd, 1);
	ds18b20_write(word, 3);
}

void ds18b20_read_scratchpad(uint8_t * word, uint8_t bytes_to_read){
	
	word[0] = 0xBE;
	ESP_LOGD(TAG_DS18B20, "read_scratchpad: BEh");
	ds18b20_write(word, 1);
	ds18b20_read(word, bytes_to_read);
}

void ds18b20_copy_scratchpad(void){
	
	uint8_t word[] = {0x48};
	ESP_LOGD(TAG_DS18B20, "copy_scratchpad: 48h");
	ds18b20_write(word, 1);
	vTaskDelay(20 / portTICK_RATE_MS);
}

void ds18b20_convert(void){
	
	uint8_t word[] = {0x44};
	ESP_LOGD(TAG_DS18B20, "convert_T: 44h");
	ds18b20_write(word, 1);
}

float ds18b20_temp_to_float(uint8_t * word, uint8_t bits){
	
	int16_t temp_2s = word[0] | (word[1] << 8);
	uint8_t n_floats = bits % 8;
	int8_t temp_int8 = (int8_t)(temp_2s >> n_floats);
	
	float temp_f = (float)temp_int8;
	for(int8_t i = 0; i < n_floats; i++){
		if((temp_2s >> (n_floats - i)) & 1)
			temp_f += powf(2, -(i + 1));
	}
	return temp_f;
}






