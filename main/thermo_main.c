#include <stdio.h>
#include "thermo.h"
#include "thermo_wifi.h"
#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/udp.h"
#include "esp_log.h"

static void error_check(err_t err, char * err_message){
	
	if(err < 0){
		ESP_LOGE(TAG_SEND, "%s: %u", err_message, err);
		ESP_LOGI(TAG_SEND, "restarting in 3...");
		vTaskDelay(3000 / portTICK_RATE_MS);
		esp_restart();
	}
	return;
}

void send_task(void *pvParameter){
	
	err_t udp_err;
	struct udp_pcb * thermo_udp;
	
	//allocate UDP Protocol Control Block
	thermo_udp = udp_new();
	if(thermo_udp == NULL) error_check(-1, "UDP PCB CANNOT BE ALLOCATED");
	
	//bind host ip and port to PCB
	udp_err = udp_bind(thermo_udp, thermo_ip, 0);
	error_check(udp_err, "BIND ERROR");
		
	//prepare the remote ip
	ip4_addr_t  ip4_addr_temp;
	ip4_addr_t * thermo_udp_rip = & ip4_addr_temp;
	IP_ADDR4(thermo_udp_rip, UDP_REMOTE_IP_BYTE3, UDP_REMOTE_IP_BYTE2, UDP_REMOTE_IP_BYTE1, UDP_REMOTE_IP_BYTE0);
	
	ESP_LOGI(TAG_SEND, "SENDING...");	

	//connect to remote ip	
	udp_err = udp_connect(thermo_udp, thermo_udp_rip, UDP_REMOTE_PORT);
	error_check(udp_err, "CONNECTION ERROR");
	
	//prepare packet to send
	message_t * ptr_m = pvParameter;
	struct pbuf *packet = pbuf_alloc(PBUF_TRANSPORT, ptr_m->m_len, PBUF_RAM);
	pbuf_take(packet, ptr_m->m, ptr_m->m_len);

	//send packet to remote
	udp_err = udp_send(thermo_udp, packet);
	error_check(udp_err, "SEND ERROR");

	//free stuff
	udp_disconnect(thermo_udp);
	pbuf_free(packet);
	
	udp_remove(thermo_udp);
	ESP_LOGI(TAG_SEND, "SENDING DONE");	
	vTaskDelete(NULL);
}

void ds18b20_task(void *pvParameter){
	
	err_ds18b20_t err_ds18b20;
	
	//initialize gpio for ds18b20
	ds18b20_gpio_init();
	
	for(;;){
		
		uint8_t word[15] = {0};
		uint8_t word_len;
		
		//reset ds18b20
		err_ds18b20 = ds18b20_reset();
		ESP_LOGD(TAG_DS_TASK, "reset: %u", err_ds18b20);
		
		//skip rom
		ds18b20_skip_rom();
		
		//convert temperature
		ds18b20_convert();
		vTaskDelay(1000 / portTICK_RATE_MS);
		
		//reset ds18b20
		err_ds18b20 = ds18b20_reset();
		ESP_LOGD(TAG_DS_TASK, "reset: %u", err_ds18b20);
		
		//skip rom
		ds18b20_skip_rom();
		
		//read scratchpad	
		word_len = 2;
		ds18b20_read_scratchpad(word, word_len);

		uint16_t temps[2] = {0};
		ds18b20_temp_printable(word, 12, temps);
		
		ESP_LOGD(TAG_DS_TASK, "TEMPERATURE: %u,%u", temps[0], temps[1]);
		
		//prepare message to send
		uint8_t str_len = snprintf(NULL, 0, "TEMP: %u.%u", temps[0], temps[1]);
		str_len += 1;
		char str[str_len];
		snprintf(str, str_len, "TEMP: %u.%u", temps[0], temps[1]);
		
		message_t message = {
			.m = str,
			.m_len = str_len
		};
		
		ESP_LOGD(TAG_DS_TASK, "message to be sent: %s", message.m);
		message_t * ptr_m = &message;
		
		//create thermo_send task
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, 25000 / portTICK_RATE_MS);
		if((xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT) == 0) {
			ESP_LOGE(TAG_DS_TASK, "CONNECTION TIMEOUT");
			sleep_prep(TAG_MAIN);
		}
		//thermo_send(ptr_m);
		xTaskCreate(&send_task, "send_task", 2048, (void *) ptr_m, 6, NULL);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void app_main(){
	
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
		
	wifi_event_group = xEventGroupCreate();
    thermo_wifi();
    
    xTaskCreate(&ds18b20_task, "ds18b20_task", 2048, NULL, 11, NULL);
    for(;;){
		vTaskDelay(100000 / portTICK_RATE_MS);
	}
}
