/* tls-mutual-auth example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"

#include "esp_log.h"
#include "context.h"


#include "context.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

esp_err_t init_nvs_flash(){
  esp_err_t ret = nvs_flash_init();
  ESP_ERROR_CHECK(ret);
  return ret;

}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %"PRIu32" bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    
    init_mac_address();

    init_gpio_pins();

	if(init_nvs_flash() != ESP_OK){
		printf("#\r\nError NVS flash\r\n#\r\n");
	}

    reset_handler();

	flash_read_boot_partition();

    flash_read_wifi_router_info();

    if(memcmp(flash_wifi_mode , WIFI_STA_MODE , 6) == 0){

        sprintf(ssid_arg , flash_router_ssid);
        sprintf(pass_arg , flash_router_password);
        
        xTaskCreate(wifi_init_sta, "wifi_init_sta_task", WIFI_STA_MODE_TASK_STACK_DEPTH, NULL, 5, NULL);		
        wifi_if_mode = STA_MODE; 

    }else{

        wifi_init_accesspoint_mode();
        wifi_if_mode = AP_MODE;
        
        xTaskCreate(udp_server_task, "udp_server", UDP_SERVER_TASK_STACK_DEPTH, (void*)AF_INET, 5, NULL);
        xTaskCreate(tcp_server_task, "tcp_server", TCP_SERVER_TASK_STACK_DEPTH, (void*)AF_INET, 5, NULL);

    }

}
