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

int aws_iot_demo_main( int argc, char ** argv );

EventGroupHandle_t s_wifi_event_group;

int  s_retry_num      = 0    ;
bool all_sockets_init = false;
char ssid_arg[128];
char pass_arg[128];

bool isConnectedToWifi = false;
int xre4 = 0;

int  wifi_if_mode= 1;
int  ip1 = 0; 
int  ip2 = 0; 
int  ip3 = 0; 
int  ip4 = 0; 
int  ip4_len = 0;

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        isConnectedToWifi = false;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        isConnectedToWifi = false;
        // if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
             esp_wifi_connect();
        //     s_retry_num++;
#ifdef WIFI_STA_DB          
             ESP_LOGI(TAG, "retry to connect to the AP");
#endif             
        // } else {
        //     xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        // }
#ifdef WIFI_STA_DB          
        ESP_LOGI(TAG,"connect to the AP fail");
#endif        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
#ifdef WIFI_STA_DB        
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
#endif
        // p_netif = *(&event->esp_netif);

#ifdef ENABLE_WIFI_STATIC_IP        
        if (esp_netif_dhcpc_stop(event->esp_netif) != ESP_OK) {

            isConnectedToWifi = true;
            if(!all_sockets_init){
                
                all_sockets_init = true;
                // Enable UDP Server
                xTaskCreate(udp_server_task, "udp_server", UDP_SERVER_TASK_STACK_DEPTH , (void*)AF_INET, 5, NULL);
                // Enable TCP Server
                xTaskCreate(tcp_server_task, "tcp_server", TCP_SERVER_TASK_STACK_DEPTH  , (void*)AF_INET, 5, NULL);
                // Check Remote Server
                // if(isCheckingRemoteServerIdle){
                //     xTaskCreate(check_remote_server_status, "http_test", 4096, NULL, 5, NULL);
                // }
                // xTaskCreate(pingMainTask, "mainTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
            
            }     
            ESP_LOGE(TAG, "Failed to stop dhcp client");      
            return;
        }

        esp_netif_ip_info_t temp_ip_info = (event)->ip_info;

        int temp_gateway_array[4] = {0};
        get_gateway_ip_info_array(temp_gateway_array , event->ip_info.gw.addr);

        int unused_ip = find_unused_ip(temp_gateway_array[0] , temp_gateway_array[1] , 
                                        temp_gateway_array[2] , temp_gateway_array[3]);

        if(unused_ip != -1){
            esp_netif_set_ip4_addr(&(temp_ip_info.ip) ,  
                                    temp_gateway_array[0] , temp_gateway_array[1] , 
                                    temp_gateway_array[2] , unused_ip);
            // esp_netif_set_ip4_addr(&(temp_ip_info.ip) , 172 , 16 , 225 , 13);      
            esp_err_t res = esp_netif_set_ip_info(event->esp_netif, &temp_ip_info);
            printf("STATIC IP RES: %X\r\n" , res);

            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&temp_ip_info.ip));
            update_wifi_ip_sta();                
        }

#else
        update_wifi_ip_sta();  
        isConnectedToWifi = true;
        if(!all_sockets_init){
            all_sockets_init = true;            
            xTaskCreate(tcp_server_task, "tcp_server", TCP_SERVER_TASK_STACK_DEPTH , (void*)AF_INET, 5, NULL);
            xTaskCreate(udp_server_task, "udp_server", UDP_SERVER_TASK_STACK_DEPTH , (void*)AF_INET, 5, NULL);          
        }

#endif        
        
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
    }
}

void update_wifi_ip_sta(){

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

void update_wifi_mode_ip(int tempIpSlice){

    ip1 = tempIpSlice & 0xFF;
    if(ip1 < 0){
        ip1 += 0xFF;
    }
    tempIpSlice = (tempIpSlice >> 8);
    ip2 = tempIpSlice & 0xFF;
    if(ip2 < 0){
        ip2 += 0xFF;
    }
    tempIpSlice = (tempIpSlice >> 8);
    ip3 = tempIpSlice & 0xFF;
    if(ip3 < 0){
        ip3 += 0xFF;
    }
    tempIpSlice = (tempIpSlice >> 8);
    ip4 = tempIpSlice & 0xFF;
    if(ip4 < 0){
        ip4 += 255;
    }	
	
}

void wifi_init_sta()
{

	int ssid_arg_len = 0;
	int pass_arg_len = 0;

	for(int i = 0 ; i < 128 ; i++){
	
		if(ssid_arg[i] == '\0'){
			ssid_arg_len = i;
			break;
		}
		
	}

	for(int i = 0 ; i < 128 ; i++){
	
		if(pass_arg[i] == '\0'){
			pass_arg_len = i;
			break;
		}
		
	}

    // //Initialize NVS
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //   ESP_ERROR_CHECK(nvs_flash_erase());
    //   ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);
#ifdef WIFI_STA_DB  
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
#endif
    s_wifi_event_group = xEventGroupCreate();

     ESP_ERROR_CHECK(esp_netif_init());

     ESP_ERROR_CHECK(esp_event_loop_create_default());
     p_netif = esp_netif_create_default_wifi_sta();

     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = {0},
            .password = {0},

               .pmf_cfg = {
                .capable = true,
                .required = false
            },         
        },
    };
	
	for(int i = 0; i < ssid_arg_len; i++){
		wifi_config.sta.ssid[i] = ssid_arg[i];
	}
	
	for(int i = 0; i < pass_arg_len; i++){
		wifi_config.sta.password[i] = pass_arg[i];
	}

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
#ifdef WIFI_STA_DB  
    ESP_LOGI(TAG, "wifi_init_sta finished.");
#endif
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        //ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid_arg, pass_arg);
#ifdef WIFI_STA_DB          
        ESP_LOGI(TAG, "connected to AP");
#endif
        isConnectedToWifi = true;
#if IS_REMOTE_CON_ENABLE == 1      
        xTaskCreate(aws_iot_task, "aws_iot_task", AWS_IOT_TASK_STACK_DEPTH, NULL, 5, NULL);
#endif        	
    } else if (bits & WIFI_FAIL_BIT) {
        //ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ssid_arg, pass_arg);
#ifdef WIFI_STA_DB          
        ESP_LOGI(TAG, "Failed to connect to AP");
#endif        
        isConnectedToWifi = false;
    } else {
#ifdef WIFI_STA_DB          
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
#endif        
        isConnectedToWifi = false;
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);

    vTaskDelete(NULL);
}

