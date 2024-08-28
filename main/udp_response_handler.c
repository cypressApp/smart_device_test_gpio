#include "context.h"

#define UDP_DB

int pairing_time_out_counter = 0;

void pairing_time_out_handler(void *pvParameters){
	
	while (pairing_step > START_PAIRING_IND){
		pairing_time_out_counter ++;
		if(pairing_time_out_counter > 100){
			pairing_time_out_counter = 0;
			pairing_step = START_PAIRING_IND;
#ifdef UDP_DB			
			printf("TIME_OUT\r\n");
#endif			
		}
		vTaskDelay(30 / portTICK_PERIOD_MS);
	}
	pairing_time_out_counter = 0;
	vTaskDelete(NULL);
}

void pairing_esp_restart(void *pvParameters){
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
	vTaskDelete(NULL);
}

bool is_pairing_command_valid(char *rx_buffer , char *prefix , char *suffix){
	
	if(strlen(suffix) > 0){
		
		if(memcmp(rx_buffer , prefix , strlen(prefix) - 1) == 0){

			if(memcmp( (rx_buffer + strlen(rx_buffer) - strlen(suffix)) , suffix , strlen(suffix) - 1) == 0){
				return true;
			}

		}
	}else{

		if(memcmp(rx_buffer , prefix , strlen(prefix) - 1) == 0){
			return true;
		}

	}


	return false;

}

void get_device_wifi_info(char *device_wifi_info){

	unsigned char mac_str[20];

	esp_base_mac_addr_get(mac_str);
	
	// uint32_t tempIpSlice = get_device_ip_info_int();
	// uint32_t ip[4] = {0};
	// for(int i = 0 ; i < 4 ; i++){
	// 	ip[i] = tempIpSlice & 0xFF;
	// 	tempIpSlice = (tempIpSlice >> 8);
	// }

	sprintf(device_wifi_info , "%d.%d.%d.%d,%02X%02X%02X%02X%02X%02X" , \
					            ip1 , ip2 , ip3 , ip4 , \
					            mac_str[0], mac_str[1], mac_str[2], mac_str[3], mac_str[4], mac_str[5]);
    
}

void udp_get_info_response(int sock , int ip4 , int account_index , char *rx_buffer){

    char device_wifi_info[64];
    get_device_wifi_info(device_wifi_info);
#ifdef UDP_DB 	
	printf("udp_get_info_response\n");
#endif
	response_required = true;
	
	send_data_len = sprintf(send_data , "%s,%s,%s,%s,%s,%s,%s" , device_wifi_info , DEVICE_UNIQUE_ID_STR , DEVICE_NAME , DEVICE_TYPE, MINOR_ID , FIRMWARE_VERSION , HARDWARE_VERSION);

	strcat(send_data , "\r\n");
	send_data_len += 2;

#ifdef UDP_DB 
	printf(send_data);
#endif

}
