
void pairing_time_out_handler(void *pvParameters);
void pairing_esp_restart(void *pvParameters);
bool is_pairing_command_valid(char *rx_buffer , char *prefix , char *suffix);
void get_device_wifi_info(char *device_wifi_info);
void udp_get_info_response(int sock , int ip4 , int account_index , char *rx_buffer);