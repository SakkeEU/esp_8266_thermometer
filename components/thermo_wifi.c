#include "thermo.h"
#include "thermo_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

static esp_err_t event_handler(void *ctx, system_event_t *event){
	
	static uint8_t n_tries = 0;
	
    switch (event->event_id) {
		
        case SYSTEM_EVENT_STA_START:
        
            ESP_LOGI(TAG_WIFI, "SYSTEM_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
        
            ESP_LOGI(TAG_WIFI, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(TAG_WIFI, "Got IP: %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            
            thermo_ip = &event->event_info.got_ip.ip_info.ip;
            
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
        
			xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, DISCONNECTED_BIT);
            ESP_LOGI(TAG_WIFI, "SYSTEM_EVENT_STA_DISCONNECTED");
            
            n_tries++;
            ESP_LOGI(TAG_WIFI, "tries : %u", n_tries);
            if(n_tries > 4){
				ESP_LOGE(TAG_WIFI, "CONNECTION TRIES EXPIRED");
				sleep_prep(TAG_WIFI);
			}
            else ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        default:
            break;
    }
    return ESP_OK;
}

void thermo_wifi(void){
	
	tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t wifi_config = {
		
		.sta = {
			
			.ssid = SSID_WIFI,
			.password = PSWD_WIFI,
			.scan_method = 0,
			.bssid_set = 0,
			.bssid = {0},
			.channel = 0,
			.listen_interval = 0,
			.sort_method = 0,
			.threshold = {.rssi = -100, .authmode = 3,},
		},
	};
	
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");
    ESP_LOGI(TAG_WIFI, "connect to ap SSID:%s password:%s", SSID_WIFI, PSWD_WIFI);
	
}
