#include <stdio.h>
#include "app.h"
#include "app_wifi.h"
#include "app_tasks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"

void app_main(){
	
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
		
	wifi_event_group = xEventGroupCreate();
    app_wifi();
    
    xTaskCreate(&ds18b20_task, "ds18b20_task", 2048, NULL, 11, NULL);
    for(;;){
		vTaskDelay(100000 / portTICK_RATE_MS);
	}
}
