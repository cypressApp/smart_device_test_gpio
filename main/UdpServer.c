#include "context.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define MULTICAST_TTL 20
#define MULTICAST_IPV6_ADDR "FF02::FC"
#define MULTICAST_IPV4_ADDR "232.10.11.12"
#define UDP_DB

#define V4TAG "V4TAG"

int  pairing_step = 0;
int  ap_step = 0;
char temp_ssid[128] = "";
char temp_password[128] = "";
char send_data[3072];
int  temp_ssid_len = 0;
int  temp_password_len = 0;
int  send_data_len = 0;
bool response_required = false;
long int random_code = 111111;

#define CY_UDP_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define CY_UDP_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

void processData(int sock, int ip4 , char *rx_buffer , struct sockaddr *sourceAddr , int len){
	
	response_required = false;
	 	
	if(memcmp(rx_buffer , SCAN_COMMAND , SCAN_COMMAND_LEN) == 0){     
	
		udp_get_info_response(sock, ip4 ,0 , rx_buffer);
	
	}else if(memcmp(rx_buffer , START_PAIRING , strlen(START_PAIRING)) == 0){
		
		if(pairing_step == START_PAIRING_IND){
			pairing_step++;
			response_required = true;	
			send_data_len = sprintf(send_data , "%s" , PAIR_ACK_RESPONSE);
			xTaskCreate(pairing_time_out_handler, "pairing_time_out_handler", 1024, NULL, 5, NULL);
		}

	} else if(is_pairing_command_valid(rx_buffer , SET_ROUTER_SSID_PREFIX , SET_ROUTER_SSID_SUFFIX) ){
	
		if(pairing_step == SET_ROUTER_SSID_IND){
			temp_ssid_len = strlen(rx_buffer) - strlen(SET_ROUTER_SSID_PREFIX) - strlen(SET_ROUTER_SSID_SUFFIX);
			memcpy(temp_ssid , (rx_buffer + strlen(SET_ROUTER_SSID_PREFIX)) , temp_ssid_len);
			temp_ssid[temp_ssid_len] = 0;
			temp_ssid_len += 1;
			pairing_step++;
			response_required = true;
			send_data_len = sprintf(send_data , "%s" , PAIR_ACK_RESPONSE);
		}
	
	} else if(is_pairing_command_valid(rx_buffer , SET_ROUTER_PASS_PREFIX , SET_ROUTER_PASS_SUFFIX)){
		
		if(pairing_step == SET_ROUTER_PASS_IND){
			temp_password_len = strlen(rx_buffer) - strlen(SET_ROUTER_PASS_PREFIX) - strlen(SET_ROUTER_PASS_SUFFIX);
			memcpy(temp_password , (rx_buffer + strlen(SET_ROUTER_PASS_PREFIX)) , temp_password_len);
			temp_password[temp_password_len] = 0;
			temp_password_len += 1;
			pairing_step++;
			response_required = true;
			send_data_len = sprintf(send_data , "%s" , PAIR_ACK_RESPONSE);
		}

	} else if(memcmp(rx_buffer , FINISH_PAIRING , strlen(FINISH_PAIRING)) == 0){
		
		if(pairing_step == FINISH_PAIRING_IND){
			flash_store_wifi_router_info(temp_ssid , temp_password , WIFI_STA_MODE , temp_ssid_len , temp_password_len);
			pairing_step = START_PAIRING_IND;
			response_required = true;
			send_data_len = sprintf(send_data , "%s" , PAIR_ACK_RESPONSE);
			xTaskCreate(pairing_esp_restart, "pairing_esp_restart", 1024, NULL, 5, NULL);  
		}
		
	}	
	
}

void test_udp_multicast_loopback(){
	
	int sock;
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    char rx_buffer[128];

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
		CY_UDP_LOGE("Failed to create socket. errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(UDP_PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        CY_UDP_LOGE("Bind failed. errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    inet_aton(MULTICAST_IPV4_ADDR, &mreq.imr_multiaddr.s_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); 
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        CY_UDP_LOGE("Failed to join multicast group. errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    uint8_t loop = 0;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(UDP_PORT);
    inet_aton(MULTICAST_IPV4_ADDR, &multicast_addr.sin_addr);

    while (1) {
      
        char msg[] = "Hello Multicast Loopback!";
        int err = sendto(sock, msg, strlen(msg), 0,
                         (struct sockaddr *)&multicast_addr, sizeof(multicast_addr));
        if (err < 0) {
            CY_UDP_LOGE("Error sending multicast: errno %d", errno);
        } else {
            CY_UDP_LOGI("Sent multicast: %s", msg);
        }

        
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                           (struct sockaddr *)&source_addr, &socklen);
        if (len > 0) {
            rx_buffer[len] = 0;
            CY_UDP_LOGI("Received multicast (loopback): %s", rx_buffer);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    close(sock);
    vTaskDelete(NULL);
}

int socket_add_ipv4_multicast_group(int sock)
{
    struct ip_mreq imreq = { 0 };
    int err = 0;
    imreq.imr_interface.s_addr = IPADDR_ANY;
    err = inet_aton(MULTICAST_IPV4_ADDR, &imreq.imr_multiaddr.s_addr);
    if (err != 1) {
        ESP_LOGE(TAG, "Configured IPV4 multicast address '%s' is invalid.", MULTICAST_IPV4_ADDR);
        err = -1;
        return err;
    }
    ESP_LOGI(TAG, "Configured IPV4 Multicast address %s", inet_ntoa(imreq.imr_multiaddr.s_addr));
    if (!IP_MULTICAST(ntohl(imreq.imr_multiaddr.s_addr))) {
        ESP_LOGW(TAG, "Configured IPV4 multicast address '%s' is not a valid multicast address. This will probably not work.", MULTICAST_IPV4_ADDR);
    }

    err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                         &imreq, sizeof(struct ip_mreq));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno);
        return err;
    }

    return 0;
}

int socket_add_multicast_ipv6_group(int sock , int netif_index){
	struct ipv6_mreq v6imreq = { 0 };
	int err = 0;
	
    err = inet6_aton(MULTICAST_IPV6_ADDR, &v6imreq.ipv6mr_multiaddr);
    if (err != 1) {
        ESP_LOGE(TAG, "Configured IPV6 multicast address '%s' is invalid.", MULTICAST_IPV6_ADDR);
        return err;
    }
    ESP_LOGI(TAG, "Configured IPV6 Multicast address %s", inet6_ntoa(v6imreq.ipv6mr_multiaddr));
    
	ip6_addr_t multi_addr;
    inet6_addr_to_ip6addr(&multi_addr, &v6imreq.ipv6mr_multiaddr);
    if (!ip6_addr_ismulticast(&multi_addr)) {
        ESP_LOGW(TAG, "Configured IPV6 multicast address '%s' is not a valid multicast address. This will probably not work.", MULTICAST_IPV6_ADDR);
    }

    v6imreq.ipv6mr_interface = (unsigned int)netif_index;
    err = setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                     &v6imreq, sizeof(struct ipv6_mreq));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IPV6_ADD_MEMBERSHIP. Error %d", errno);
        return err;
    }
	
	return err;

}

int create_udp_socket(int *sock){

	struct sockaddr_in6 dest_addr;
	dest_addr.sin6_family = AF_INET6;
	dest_addr.sin6_port = htons(UDP_PORT);
	bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
	int err = -1;
	
	*sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
	if (*sock < 0) {
		CY_UDP_LOGE("Unable to create socket: errno %d", errno);
		goto error_line;
	}
 		
	CY_UDP_LOGI("Socket created");

	int reuse = 1;
	setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	int v6only = 0;
	setsockopt(*sock, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));

	err = bind(*sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err < 0) {
		CY_UDP_LOGE("Socket unable to bind: errno %d", errno);
		goto error_line;			
	}
	
	int netif_index = esp_netif_get_netif_impl_index(p_netif_sta);
	if(netif_index < 0) {
		CY_UDP_LOGE("Failed to get netif index");
		goto error_line;
	}

	err = setsockopt(*sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &netif_index, sizeof(unsigned int));
	if (err < 0) {
		CY_UDP_LOGE("Failed to set IPV6_MULTICAST_IF. Error %d", errno);
		goto error_line;
	}

	uint8_t ttl = MULTICAST_TTL;
	setsockopt(*sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(uint8_t));
	if (err < 0) {
		CY_UDP_LOGE("Failed to set IPV6_MULTICAST_HOPS. Error %d", errno);
		goto error_line;
	}
	err = socket_add_multicast_ipv6_group(*sock , netif_index);
	if (err < 0) {
		CY_UDP_LOGE("Failed to add multicast IPv6 group. Error %d", errno);
		goto error_line;
	}

	err = socket_add_ipv4_multicast_group(*sock);
	if (err < 0) {
		CY_UDP_LOGE("Failed to add multicast IPv4 group. Error %d", errno);
		goto error_line;
	}
	
	return err;

error_line:
	if(err < 0 && *sock != -1){
		close(*sock);
		*sock = -1;
	}
	return err;
}

void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str [128];
    
	int sock = -1;

    while (1) {

		if (create_udp_socket(&sock) < 0) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

        while (1) {
			CY_UDP_LOGI("Waiting for data");			
			struct sockaddr_storage source_addr; 
			socklen_t socklen = sizeof(source_addr);
			int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

			if (len < 0) {
				CY_UDP_LOGE("recvfrom failed: errno %d", errno);
				break;
			}
			else {
				if (source_addr.ss_family == PF_INET) {
					inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
				} else if (source_addr.ss_family == PF_INET6) {
					inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
				}

				int temp_ip4 = get_ip4(addr_str , sizeof(addr_str));

				rx_buffer[len] = 0; 
			
				unsigned char mac_str[20];
				CY_UDP_LOGI("Received %d bytes from %s:", len, addr_str);
				CY_UDP_LOGI("%s", rx_buffer);
				esp_base_mac_addr_get(mac_str);
				CY_UDP_LOGI("BASE: %02X %02X %02X %02X %02X %02X", mac_str[0], mac_str[1], mac_str[2], mac_str[3], mac_str[4], mac_str[5]);
				esp_efuse_mac_get_default(mac_str);				
				CY_UDP_LOGI("EFUSE: %02X %02X %02X %02X %02X %02X", mac_str[0], mac_str[1], mac_str[2], mac_str[3], mac_str[4], mac_str[5]);
				if(temp_ip4 < 256){
					processData(sock , temp_ip4 , rx_buffer , (struct sockaddr *)&source_addr , len);
				}
	
				if(response_required){						
					int err = sendto(sock, send_data, send_data_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
					
					if (err < 0) {
						CY_UDP_LOGE("Error occured during sending: errno %d", errno);
						break;
					}

				}

			}

        }

        if (sock != -1) {
            CY_UDP_LOGE("Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
	
}