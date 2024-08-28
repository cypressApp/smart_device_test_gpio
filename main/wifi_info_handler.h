#include "esp_netif.h"

#define  AP_MODE     1
#define  STA_MODE    2

#define WIFI_STA_MODE "WiMStA\r\n"
#define WIFI_AP_MODE  "WiMAcP\r\n"

extern esp_netif_t * p_netif;
extern uint8_t device_mac_address[10];

uint32_t get_device_ip_info_int();
uint32_t get_gateway_ip_info_int();
void get_gateway_ip_info_array(int *array , uint32_t gateway_ip);
void init_mac_address();
int get_ip4(char *addr_str , int len);