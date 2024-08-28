#include "context.h"

#define ESP_AP_MAX_STA_CONN   10

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    //     // ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    // } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    //     wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    //     // ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    // }
}

void wifi_init_accesspoint_mode()
{
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    p_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_AP_WIFI_SSID,
            .ssid_len = strlen(ESP_AP_WIFI_SSID),
            .password = ESP_AP_WIFI_PASS,
            .max_connection = ESP_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_AP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ESP_AP_WIFI_SSID, ESP_AP_WIFI_PASS);

    update_wifi_ip_ap();       
}

void update_wifi_ip_ap(){

    int tempIpSlice = get_device_ip_info_int();
    update_wifi_mode_ip(tempIpSlice);

    ip4_len = 0;
    if(ip4 < 10){
        ip4_len = 1;
    }else if(ip4 < 100){
        ip4_len = 2;
    }else if(ip4 < 1000){
        ip4_len = 3;
    }   
}