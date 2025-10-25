#include "context.h"

#define CLOSE_TCP_SOCKET_DELAY  100
#define MAX_TCP_ACCOUNT_SIZE    100

int  tcp_timeout_counter = 0;
int  tcp_rec_data_counter = 0;
bool is_tcp_timeout = false;
bool valid_data_received = false;

void process_tcp_data(char* rx_buffer , int rx_buffer_len , int sock){

#ifdef TCP_SERVER_DB    
    printf("%s\n" , rx_buffer);
#endif

    // if(ota_command_handler(rx_buffer , rx_buffer_len , sock)){
    //     xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    //     return;
    // }
    if(memcmp(rx_buffer , "https" , 5) == 0){
        memcpy(firmwareUrl , rx_buffer , rx_buffer_len);
        firmwareUrl[rx_buffer_len] = 0;
        xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
        send_data_len = sprintf(send_data , "%s\n" , PAIR_ACK_RESPONSE);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        send(sock, send_data, send_data_len, 0);
        return;
    }
    else if(memcmp(rx_buffer , START_PAIRING , strlen(START_PAIRING)) == 0){
		if(pairing_step == START_PAIRING_IND){
			pairing_step++;
			response_required = true;	
			send_data_len = sprintf(send_data , "%s\n" , PAIR_ACK_RESPONSE);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send(sock, send_data, send_data_len, 0);
		//	xTaskCreate(pairing_time_out_handler, "pairing_time_out_handler", 1024, NULL, 5, NULL);
		}

	} else if(is_pairing_command_valid(rx_buffer , SET_ROUTER_SSID_PREFIX , SET_ROUTER_SSID_SUFFIX) ){
	
		if(pairing_step == SET_ROUTER_SSID_IND){
			temp_ssid_len = strlen(rx_buffer) - strlen(SET_ROUTER_SSID_PREFIX) - strlen(SET_ROUTER_SSID_SUFFIX);
			memcpy(temp_ssid , (rx_buffer + strlen(SET_ROUTER_SSID_PREFIX)) , temp_ssid_len);
			temp_ssid[temp_ssid_len] = 0;
			temp_ssid_len += 1;
			pairing_step++;
			response_required = true;
			send_data_len = sprintf(send_data , "%s\n" , PAIR_ACK_RESPONSE);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send(sock, send_data, send_data_len, 0);
		}
	
	} else if(is_pairing_command_valid(rx_buffer , SET_ROUTER_PASS_PREFIX , SET_ROUTER_PASS_SUFFIX)){
		
		if(pairing_step == SET_ROUTER_PASS_IND){
			temp_password_len = strlen(rx_buffer) - strlen(SET_ROUTER_PASS_PREFIX) - strlen(SET_ROUTER_PASS_SUFFIX);
			memcpy(temp_password , (rx_buffer + strlen(SET_ROUTER_PASS_PREFIX)) , temp_password_len);
			temp_password[temp_password_len] = 0;
			temp_password_len += 1;
			pairing_step++;
			response_required = true;
			send_data_len = sprintf(send_data , "%s\n" , PAIR_ACK_RESPONSE);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send(sock, send_data, send_data_len, 0);
		}

	} else if(memcmp(rx_buffer , FINISH_PAIRING , strlen(FINISH_PAIRING)) == 0){
		
		if(pairing_step == FINISH_PAIRING_IND){
			flash_store_wifi_router_info(temp_ssid , temp_password , WIFI_STA_MODE , temp_ssid_len , temp_password_len);
			pairing_step = START_PAIRING_IND;
			response_required = true;
			send_data_len = sprintf(send_data , "%s\n" , PAIR_ACK_RESPONSE);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send(sock, send_data, send_data_len, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
			xTaskCreate(pairing_esp_restart, "pairing_esp_restart", 1024, NULL, 5, NULL);  
		}
		
	}	
    
    char *temp_response = (char *) calloc(256 , sizeof(char)); 
    bool is_data_valid = true;

    /**
     * For handling local commands enter your code here
     */
    // if(/* command */){

    // }
    // else{
    //     is_data_valid = false;
    // }

    int len = sprintf(temp_response , "Hi I'm ESP32 Smart Device: Local\n");
    printf("size: %d\n", len);
    if(is_data_valid){
        send_data_to_clients(SEND_TO_ALL, temp_response, len);
    }
    
    free(temp_response);

}

void check_tcp_recv_timeout_task(void *pvParameters){

    while(tcp_timeout_counter < 100){
        if(!valid_data_received){
            tcp_timeout_counter ++;
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }else{
            break;
        }
    }
    if(!valid_data_received){
        // tcp_rec_data_counter = 0;
        is_tcp_timeout = true;
    }

    vTaskDelete(NULL);
}

void receiving_tcp_data(void *pvParameters){

    char temp_rx_buffer[5];
    char rx_buffer[TCP_RECEIVE_DATA_LENGTH];

    valid_data_received = true;
    vTaskDelay(2 / portTICK_PERIOD_MS);
    valid_data_received = false;
    tcp_timeout_counter = 0;
    tcp_rec_data_counter = 0;
    is_tcp_timeout      = false;

    account_struct *temp_account = (account_struct *) pvParameters;

    while (1) {
        //vTaskDelay(50 / portTICK_RATE_MS);
#ifdef CONSTANT_TCP_RECEIVE_LEN        
        int len = recv(sock, rx_buffer, TCP_RECEIVE_DATA_LENGTH , 0);
#else        
        int len = recv(temp_account->sock, temp_rx_buffer, 1 , 0);
        tcp_rec_data_counter ++;
#endif        
        // Error occured during receiving
        if (len < 0) {
#ifdef TCP_SERVER_DB            
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
#endif            
            break;
        }
        // Connection closed
        else if (len == 0) {
#ifdef TCP_SERVER_DB            
            ESP_LOGI(TAG, "Connection closed");
#endif            
            break;
        }
        // Data received
        else {

#ifdef CONSTANT_TCP_RECEIVE_LEN        
            char temp_rx_buffer[TCP_RECEIVE_DATA_LENGTH + 1] = {0};
            for(int i = 0 ; i < TCP_RECEIVE_DATA_LENGTH ; i++){
                temp_rx_buffer[i] = rx_buffer[i];
            }
            temp_rx_buffer[TCP_RECEIVE_DATA_LENGTH] = 0;
            process_tcp_data(temp_rx_buffer , sock);
#else        
            rx_buffer[tcp_rec_data_counter - 1] = temp_rx_buffer[0];
            if(tcp_rec_data_counter >= TCP_RECEIVE_DATA_LENGTH){
                tcp_rec_data_counter = 0;
            }
            else if(tcp_rec_data_counter == 1){
                valid_data_received = false;
                tcp_timeout_counter = 0;
                is_tcp_timeout = false;
                xTaskCreate(check_tcp_recv_timeout_task, "check_tcp_recv_timeout", CHECK_TCP_TIMEOUT_TASK_STACK_DEPTH, NULL , 5, NULL);
            }
            else if (update_firmware_mode){
                    
                if(tcp_rec_data_counter >= update_firmware_buffer_size){
                    rx_buffer[tcp_rec_data_counter] = 0; 
                    tcp_rec_data_counter = 0;
#ifdef TCP_SERVER_DB 
                    ESP_LOGI(TAG, "%s   %d", rx_buffer , sizeof(rx_buffer));
#endif
                    valid_data_received = true;
                    process_tcp_data(rx_buffer , update_firmware_buffer_size , temp_account->sock);
                    vTaskDelay(1 / portTICK_PERIOD_MS);                       
                }
            }
            else if(tcp_rec_data_counter >= TCP_RECEIVE_DATA_SUFFIX_LENGTH ){
                    if(memcmp( (rx_buffer + tcp_rec_data_counter - TCP_RECEIVE_DATA_SUFFIX_LENGTH) , TCP_RECEIVE_DATA_SUFFIX , TCP_RECEIVE_DATA_SUFFIX_LENGTH ) == 0 ){
                        
                        valid_data_received = true;
                        rx_buffer[tcp_rec_data_counter] = 0;         
#ifdef TCP_SERVER_DB                         
                        ESP_LOGI(TAG, "%s   %d  %d\n", rx_buffer , sizeof(rx_buffer) , tcp_rec_data_counter);
#endif                          
                        process_tcp_data(rx_buffer , (tcp_rec_data_counter - 1) , temp_account->sock);
                        tcp_rec_data_counter = 0;
                        vTaskDelay(1 / portTICK_PERIOD_MS);

                    }else if(tcp_rec_data_counter == 1){
                        valid_data_received = false;
                        tcp_timeout_counter = 0;
                        is_tcp_timeout = false;
                        xTaskCreate(check_tcp_recv_timeout_task, "check_tcp_recv_timeout", CHECK_TCP_TIMEOUT_TASK_STACK_DEPTH, NULL , 5, NULL);
                    }
            }

#endif  

        }
        
    }
#ifdef TCP_SERVER_DB
    ESP_LOGE(TAG, "Shutting down socket and restarting...");
#endif   

    remove_account(temp_account->sock);

    shutdown(temp_account->sock, 0);
    close(temp_account->sock);

    vTaskDelete(NULL);
        
}

void test_tcp_send_data(void *pvParameters){

    int sock = (int)pvParameters;
    char *data = "Hello from ESP32\n";
    int data_size = strlen(data);

    while (1)
    {
        send(sock, data, data_size, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;
    int listen_sock;

    while(1){

        if (addr_family == AF_INET) {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(TCP_PORT);
            ip_protocol = IPPROTO_IP;
        }

        listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
#ifdef TCP_SERVER_DB
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
#endif                        
    
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef TCP_SERVER_DB
        ESP_LOGI(TAG, "Socket created");
#endif
        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
#ifdef TCP_SERVER_DB
        if (err != 0) {            
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(TAG, "IPPROTO: %d", addr_family);           
        //    goto CLEAN_UP;
        }      
        ESP_LOGI(TAG, "Socket bound, port %d", TCP_PORT);
#endif
        err = listen(listen_sock, 10);

#ifdef TCP_SERVER_DB
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        //    goto CLEAN_UP;
        }
#endif
        while (1) {
#ifdef TCP_SERVER_DB
            ESP_LOGI(TAG, "Socket listening");
#endif
            struct sockaddr_storage source_addr; 
            socklen_t addr_len = sizeof(source_addr);
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0) {
#ifdef TCP_SERVER_DB                
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
#endif                
                break;
            }

            if (source_addr.ss_family == PF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }
           
            int temp_ip4 = get_ip4(addr_str , sizeof(addr_str));
#ifdef TCP_SERVER_DB
            ESP_LOGI(TAG, "Socket accepted ip address: %d", temp_ip4);
#endif
            if(temp_ip4 < 255){
                add_account(sock , temp_ip4); 
                xTaskCreate(receiving_tcp_data, "receiving_tcp_data", TCP_SERVER_TASK_STACK_DEPTH, (void *)&account_struct_list[account_list_size - 1] , 5, NULL);
                // xTaskCreate(test_tcp_send_data, "test_tcp_send_data", 4096, (void *)sock , 5, NULL);
        }

        }
        
    }

    vTaskDelete(NULL);

}

