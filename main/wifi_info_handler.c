#include "context.h"

esp_netif_t * p_netif;
uint8_t device_mac_address[10] = {0};

uint32_t get_device_ip_info_int(){
    
    // esp_netif_t * p_netif = esp_netif_create_default_wifi_ap();
    esp_netif_ip_info_t if_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif, &if_info));

    return if_info.ip.addr;
}

uint32_t get_gateway_ip_info_int(){

    esp_netif_ip_info_t if_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif, &if_info));

    return if_info.gw.addr;

}

void get_gateway_ip_info_array(int *array , uint32_t gateway_ip){

	uint32_t temp = gateway_ip;
	for(int i = 0 ; i < 4 ; i++){
		array[i] = temp & 0xFF;
		if(array[i] < 0){
			array[i] += 0xFF;
		}
		temp = temp >> 8;
	}

}

void init_mac_address(){

	for(int i = 0 ; i < 6 ; i++){
		char temp_value = 0;
		int index1 =  2 * i;
		int index2 = (2 * i) + 1;
		if(DEVICE_MAC_ADDRESS_STR[index1] >= 'a'){
			temp_value = DEVICE_MAC_ADDRESS_STR[index1] - 'a' + 10;
		}else if(DEVICE_MAC_ADDRESS_STR[index1] >= 'A'){
			temp_value = DEVICE_MAC_ADDRESS_STR[index1] - 'A' + 10;
		}else if(DEVICE_MAC_ADDRESS_STR[index1] >= '0'){
			temp_value = DEVICE_MAC_ADDRESS_STR[index1] - '0';
		}
		temp_value *= 16;

		if(DEVICE_MAC_ADDRESS_STR[index2] >= 'a'){
			temp_value += DEVICE_MAC_ADDRESS_STR[index2] - 'a' + 10;
		}else if(DEVICE_MAC_ADDRESS_STR[index2] >= 'A'){
			temp_value += DEVICE_MAC_ADDRESS_STR[index2] - 'A' + 10;
		}else if(DEVICE_MAC_ADDRESS_STR[index2] >= '0'){
			temp_value += DEVICE_MAC_ADDRESS_STR[index2] - '0';
		}
		device_mac_address[i] = temp_value;
	}
#ifdef WIFI_INFO_TEST_MODE
	printf("MAC: %.2X %.2X %.2X %.2X %.2X %.2X\r\n" , device_mac_address[0], device_mac_address[1], 
			device_mac_address[2], device_mac_address[3], device_mac_address[4], device_mac_address[5]);
#endif
	esp_base_mac_addr_set(device_mac_address);	

}


int get_ip4(char *addr_str , int len){

	int temp_dot_counter = 0;
	char temp_ip4_str[5] = "";
	int  temp_ip4 = 256;
	int  temp_ip4_counter = 0;
	for(int i = 0 ; i < len ; i++){

		if(temp_dot_counter == 3){
			if(addr_str[i] == '\0'){
				temp_ip4 = atoi(temp_ip4_str);
				break;
			}else{
				temp_ip4_str[temp_ip4_counter] = addr_str[i];
				temp_ip4_counter++;
			}
		}else{
			if(addr_str[i] == '.'){
				temp_dot_counter++;
			}
		}

	}

	return temp_ip4;

}