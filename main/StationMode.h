
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

#define WIFI_STA_DB
#undef ENABLE_WIFI_STATIC_IP

#define EXAMPLE_ESP_WIFI_SSID      ""
#define EXAMPLE_ESP_WIFI_PASS      ""
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
#define WIFI_CONNECTED_BIT         BIT0
#define WIFI_FAIL_BIT              BIT1

extern EventGroupHandle_t s_wifi_event_group;
extern bool               isConnectedToWifi ;

extern int  s_retry_num;
extern bool all_sockets_init;
extern char ssid_arg[128];
extern char pass_arg[128];

extern int  wifi_if_mode; 
extern int  ip1; 
extern int  ip2; 
extern int  ip3; 
extern int  ip4; 
extern int  ip4_len;

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void update_wifi_ip_sta();
void update_wifi_mode_ip(int tempIpSlice);
void wifi_init_sta();
