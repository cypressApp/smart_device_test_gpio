#include "esp_event.h"

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_accesspoint_mode();
void update_wifi_ip_ap();
