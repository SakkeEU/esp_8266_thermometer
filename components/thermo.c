#include "app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"


void sleep_prep(const char * TAG){
	
	ESP_LOGI(TAG, "RESTARTING ...");
	vTaskDelay(5000 / portTICK_RATE_MS);
	
	#ifdef _APP_WIFI_
		esp_wifi_stop();
		esp_wifi_deinit();
	#endif
	esp_restart();
}
