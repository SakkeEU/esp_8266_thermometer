#ifndef _APP_WIFI_
#define _APP_WIFI_

#include "esp_wifi.h"
#include "event_groups.h"

#define CONNECTED_BIT (1 << 0)
#define DISCONNECTED_BIT (1 << 1)

#define SSID_WIFI "-"
#define PSWD_WIFI "-"

#define UDP_REMOTE_IP_BYTE3 192
#define UDP_REMOTE_IP_BYTE2 168
#define UDP_REMOTE_IP_BYTE1 1
#define UDP_REMOTE_IP_BYTE0 6

#define UDP_REMOTE_PORT 5005

typedef struct{
	char * m;
	uint8_t m_len;
} message_t;

EventGroupHandle_t wifi_event_group;
ip4_addr_t * app_ip;

void app_wifi();

#endif
