#include "context.h"

#undef TCP_OTA_DB

int update_firmware_timeout_counter = 0;
bool new_update_firmware_data_received = false;
bool stop_firmware_timeout = false;
bool update_firmware_mode = false;
int  update_firmware_buffer_size = 0;

bool ota_command_handler(char* rx_buffer , int rx_buffer_len , int sock){

    bool is_command_ota = true;

    // Select Write Partition
    if(rx_buffer[0] == SELECT_OTA_WRITE_PARTITION_HEADER && rx_buffer[5] == SELECT_OTA_WRITE_PARTITION_FOOTER){
        tcp_select_ota_write_partition(sock , rx_buffer);        
    }
    // Select Read Partition
    else if(rx_buffer[0] == SELECT_OTA_READ_PARTITION_HEADER && rx_buffer[5] == SELECT_OTA_READ_PARTITION_FOOTER){
        tcp_select_ota_read_partition(sock , rx_buffer);        
    }
    // Select Erase Partition
    else if(rx_buffer[0] == SELECT_OTA_ERASE_PARTITION_HEADER && rx_buffer[5] == SELECT_OTA_ERASE_PARTITION_FOOTER){
        tcp_select_ota_erase_partition(sock , rx_buffer);       
    }
    // Select Erase Write Partition
    else if(rx_buffer[0] == SELECT_OTA_ERASE_WRITE_PARTITION_HEADER && rx_buffer[5] == SELECT_OTA_ERASE_WRITE_PARTITION_FOOTER){       
        tcp_select_ota_erase_write_partition(sock , rx_buffer);        
    }
    // Write Partition Data
    else if(rx_buffer[0] == OTA_WRITE_TO_PARTITION_HEADER){//  && rx_buffer[rx_buffer[1] - 3] == OTA_WRITE_TO_PARTITION_FOOTER){
        tcp_ota_write_to_partition(sock , rx_buffer);
    }  
    // Read Partition Data
    else if(rx_buffer[0] == OTA_READ_FROM_PARTITION_HEADER  && rx_buffer[5] == OTA_READ_FROM_PARTITION_FOOTER){
        tcp_ota_read_from_partition(sock , rx_buffer);        
    }   
    // Erase Partition Data
    else if(rx_buffer[0] == OTA_ERASE_PARTITION_HEADER  && rx_buffer[5] == OTA_ERASE_PARTITION_FOOTER){
        tcp_ota_erase_partition(sock , rx_buffer);        
    }  
    // Erase Write Partition Data
    else if(rx_buffer[0] == OTA_ERASE_WRITE_PARTITION_HEADER){//  && rx_buffer[rx_buffer[1] - 3] == OTA_ERASE_WRITE_PARTITION_FOOTER){
        int data_size = rx_buffer[2];
        if(rx_buffer[1] != 0xFF){
            data_size = ((rx_buffer[1] * 256) + rx_buffer[2]);
        }
        tcp_ota_erase_write_partition(sock , rx_buffer , data_size);  
    } 
    // Start Update Firmware
    else if(rx_buffer[0] == OTA_START_UPDATE_FIRMWARE_HEADER){// && rx_buffer[8] == OTA_START_UPDATE_FIRMWARE_FOOTER){
        tcp_start_ota_update_firmware(sock , rx_buffer);         
    }
    // End Update Firmware
    else if(rx_buffer[0] == OTA_END_UPDATE_FIRMWARE_HEADER){// && rx_buffer[5] == OTA_END_UPDATE_FIRMWARE_FOOTER){
        int data_size = rx_buffer[2];
        if(rx_buffer[1] != 0xFF){
            data_size = ((rx_buffer[1] * 256) + rx_buffer[2]);
        }
        tcp_end_ota_update_firmware(sock , rx_buffer , data_size);         
    }
    // Set Partition
    else if(rx_buffer[0] == OTA_SET_BOOT_PARTITION_HEADER){// && rx_buffer[5] == OTA_END_UPDATE_FIRMWARE_FOOTER){
        tcp_set_boot_partition(sock , rx_buffer);         
    }
    // Get Firmware Version
    else if(rx_buffer[0] == GET_HARDWARE_FIRMWARE_VERSION_HEADER && rx_buffer[5] == GET_HARDWARE_FIRMWARE_VERSION_FOOTER){
        tcp_get_hardware_firmware_version(sock , rx_buffer);        
    }
    // Cancel Update Firmware
    else if(rx_buffer[0] == OTA_CANCEL_UPDATE_FIRMWARE_HEADER && rx_buffer[5] == OTA_CANCEL_UPDATE_FIRMWARE_FOOTER){
        tcp_cancel_ota_update_firmware(sock , rx_buffer);       
    }else{
        is_command_ota = false;
    }

    return is_command_ota;

}

void update_firmware_timeout(){

    update_firmware_timeout_counter = 0;
    stop_firmware_timeout = false;
    while (update_firmware_timeout_counter < 50 && !stop_firmware_timeout){
        if(new_update_firmware_data_received){
            new_update_firmware_data_received = false;
            update_firmware_timeout_counter = 0;
        }else{
            update_firmware_timeout_counter++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

#ifdef TCP_OTA_DB 
    printf("Timeout\r\n");
#endif

    tcp_rec_data_counter = 0;
    update_firmware_mode = false;
    vTaskDelete(NULL);

}

void tcp_ota_response(int sock , char *response , int response_length , char header , char footer){
	
    char send_tcp_data[128];
	
	send_tcp_data[0] = OTA_RESPONSE_HEADER;
    long int temp_sum = send_tcp_data[0];
    int i = 0;
    for(i = 0 ; i < response_length ; i++){
        send_tcp_data[i + 1] = response[i];
        temp_sum += send_tcp_data[i + 1];       
    }
	
	send_tcp_data[i + 1] = OTA_RESPONSE_FOOTER;
    temp_sum += footer;
	send_tcp_data[i + 2] = (temp_sum) & 0xFF;
	send_tcp_data[i + 3] = 0x0D;
	send_tcp_data[i + 4] = 0x0A;
  
    send(sock, send_tcp_data, (i + 5), 0);
}

void tcp_select_ota_write_partition(int sock , char *rx_buffer){

    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }

    new_update_firmware_data_received = true;

    ota_set_new_write_partition(rx_buffer[2] , rx_buffer[4]);            
    tcp_ota_response(sock , TCP_OTA_SELECET_OTA_WRITE_RESPONSE , 6 ,
                        SELECT_OTA_WRITE_PARTITION_HEADER , SELECT_OTA_WRITE_PARTITION_FOOTER);
                
}

void tcp_select_ota_erase_partition(int sock , char *rx_buffer){
    
    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }
    new_update_firmware_data_received = true;

    ota_set_new_erase_partition(rx_buffer[2] , rx_buffer[4]);            

    tcp_ota_response(sock , TCP_OTA_SELECET_OTA_ERASE_RESPONSE , 6 , 
                        SELECT_OTA_ERASE_PARTITION_HEADER , SELECT_OTA_ERASE_PARTITION_FOOTER);
            
}

void tcp_select_ota_read_partition(int sock , char *rx_buffer){

    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }
    new_update_firmware_data_received = true;

    ota_set_new_read_partition(rx_buffer[2] , rx_buffer[4]);            
    tcp_ota_response(sock , TCP_OTA_SELECET_OTA_READ_RESPONSE , 6, 
                        SELECT_OTA_READ_PARTITION_HEADER , SELECT_OTA_READ_PARTITION_FOOTER);
            
}

void tcp_select_ota_erase_write_partition(int sock , char *rx_buffer){

    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }
    new_update_firmware_data_received = true;

#ifdef TCP_OTA_DB 
    printf("tcp_select_ota_erase_write_partition\r\n");
#endif        
           
    char temp_type = rx_buffer[2];
    char temp_subtype = rx_buffer[4];

    if(temp_type == 0x30){
        temp_type -= 0x30;
    }
    if(temp_subtype == 0x30){
        temp_subtype -= 0x30;
    }

    ota_set_new_erase_write_partition(temp_type , temp_subtype);    
            
    tcp_ota_response(sock , TCP_OTA_SELECET_OTA_ERASE_WRITE_RESPONSE , 6, 
                        SELECT_OTA_ERASE_WRITE_PARTITION_HEADER , SELECT_OTA_ERASE_WRITE_PARTITION_FOOTER);
            
}


void tcp_ota_write_to_partition(int sock , char *rx_buffer){
    new_update_firmware_data_received = true;
    ota_write(rx_buffer[2] , rx_buffer , 256); 
}

void tcp_ota_read_from_partition(int sock , char *rx_buffer){
    new_update_firmware_data_received = true;
    ota_read(rx_buffer[2] , rx_buffer , 256);  
}

void tcp_ota_erase_partition(int sock , char *rx_buffer){
    new_update_firmware_data_received = true;
    ota_erase(rx_buffer[2] , 256);            
}

void tcp_ota_erase_write_partition(int sock , char *rx_buffer , long int data_size){

#ifdef TCP_OTA_DB     
    printf("tcp_ota_erase_write_partition\r\n");
#endif

#ifdef TCP_OTA_DB         
    printf("Erase_write %ld %d %d %d\r\n" , data_size , rx_buffer[data_size - 3] , rx_buffer[data_size - 2] , rx_buffer[data_size - 1]);
#endif

    if(!is_checksum_correct(rx_buffer , 0 , data_size - 3 , data_size - 2)){
        return;
    }

    new_update_firmware_data_received = true;

#ifdef TCP_OTA_DB 
    printf("Data size: %ld\r\n" , data_size);
#endif        

    long int temp_offset = 0;
    for(int i = 3 ; i < 11 ; i++){
        if(rx_buffer[i] >= 0x30 && rx_buffer[i] <= 0x39){
            temp_offset += (rx_buffer[i] - 0x30);
            temp_offset *= 10;
        }else{
#ifdef TCP_OTA_DB                 
            printf("Offset error\r\n");
#endif                
            return;
        }
    }
    temp_offset /= 10;
#ifdef TCP_OTA_DB         
    printf("Address offset: %ld\r\n" , temp_offset);
#endif
    ota_erase_write(temp_offset , (rx_buffer + 11) , data_size - 14);

    tcp_ota_response(sock , TCP_OTA_ERASE_WRITE_RESPONSE , 6, 
                        OTA_ERASE_WRITE_PARTITION_HEADER , OTA_ERASE_WRITE_PARTITION_FOOTER);
            
}

void tcp_start_ota_update_firmware(int sock , char *rx_buffer){
    
    new_update_firmware_data_received = true;

    last_erase_offset = 0;

    long int temp_update_firmware_buffer_size = 0;
    for(int i = 1 ; i < 8 ; i++){
        if(rx_buffer[i] >= 0x30 && rx_buffer[i] <= 0x39){
            temp_update_firmware_buffer_size += (rx_buffer[i] - 0x30);
            temp_update_firmware_buffer_size *= 10;
        }else{
#ifdef TCP_OTA_DB                     
            printf("Error: Firmware buffer size\r\n");
#endif                    
            return;
        }
    }

    int  temp_username_len = 0;
    char temp_username[1024] = {0};
    for(int i = 8 ; i < 1024 ; i++){
        if(rx_buffer[i] == 0x20){
            break;
        }else{
            temp_username[temp_username_len] = rx_buffer[i];
            temp_username_len++;
        }
    }
#ifdef TCP_OTA_DB        
    printf("Username: %s %d %d\n" , temp_username , temp_username_len , strlen(OTA_USERNAME));
#endif
    int  temp_password_len = 0;
    char temp_password[1024] = {0};
    for(int i = (temp_username_len + 1 + 8)  ; i < temp_username_len + 1 + 8 + 1024 ; i++){
        if(rx_buffer[i] == 0x20){
            break;
        }else{
            temp_password[temp_password_len] = rx_buffer[i];
            temp_password_len++;
        }
    }
#ifdef TCP_OTA_DB        
    printf("Password: %s %d %d\n" , temp_password , temp_password_len , strlen(OTA_PASSWORD));
#endif

#ifdef CHECK_OTA_CREDENTIAL

    if(strlen(OTA_USERNAME) != temp_username_len || strlen(OTA_PASSWORD) != temp_password_len ){
        
        tcp_ota_response(sock , TCP_OTA_ERROR_UPDATE_FIRMWARE_RESPONSE , 6 , 
                OTA_ERROR_UPDATE_FIRMWARE_HEADER , OTA_ERROR_UPDATE_FIRMWARE_FOOTER);

        return;
    }

    if(memcmp(temp_username , OTA_USERNAME , temp_username_len) != 0 ||
            memcmp(temp_password , OTA_PASSWORD , temp_password_len) != 0){
            
        tcp_ota_response(sock , TCP_OTA_ERROR_UPDATE_FIRMWARE_RESPONSE , 6 , 
                OTA_ERROR_UPDATE_FIRMWARE_HEADER , OTA_ERROR_UPDATE_FIRMWARE_FOOTER);

        return;
    }

#endif

    temp_update_firmware_buffer_size /= 10;
    update_firmware_buffer_size = temp_update_firmware_buffer_size;
    update_firmware_mode = true;
    xTaskCreate(update_firmware_timeout, "update_firmware_timeout", 1024, (void*)AF_INET, 5, NULL);
    
#ifdef TCP_OTA_DB             
    printf("Firmware buffer size: %d\r\n" , update_firmware_buffer_size);
#endif
    tcp_ota_response(sock , TCP_OTA_START_UPDATE_FIRMWARE_RESPONSE , 6 , 
                        OTA_START_UPDATE_FIRMWARE_HEADER , OTA_START_UPDATE_FIRMWARE_FOOTER);
            
}

void tcp_end_ota_update_firmware(int sock , char *rx_buffer , long int data_size){

#ifdef TCP_OTA_DB        
    printf("tcp_end_ota_update_firmware\r\n");
#endif

    if(!is_checksum_correct(rx_buffer , 0 , data_size - 3 , data_size - 2)){
        return;
    }

    new_update_firmware_data_received = true;

    flash_erase_write_boot_partition(rx_buffer[11]);
    
    update_firmware_buffer_size = 0;
    update_firmware_mode = false;

    tcp_ota_response(sock , TCP_OTA_END_UPDATE_FIRMWARE_RESPONSE , 6 , 
                            OTA_END_UPDATE_FIRMWARE_HEADER , OTA_END_UPDATE_FIRMWARE_FOOTER);

    vTaskDelay(pdMS_TO_TICKS(300));

    esp_restart();


}

void tcp_set_boot_partition(int sock , char *rx_buffer){

#ifdef TCP_OTA_DB        
    printf("set_partition\r\n");
#endif

    int  temp_username_len = 0;
    char temp_username[1024] = {0};
    for(int i = 1 ; i < 1024 + 1 ; i++){
        if(rx_buffer[i] == 0x20){
            break;
        }else{
            temp_username[temp_username_len] = rx_buffer[i];
            temp_username_len++;
        }
    }
#ifdef TCP_OTA_DB        
    printf("Username: %s %d %d\n" , temp_username , temp_username_len , strlen(OTA_USERNAME));
#endif
    int  temp_password_len = 0;
    char temp_password[1024] = {0};
    for(int i = (temp_username_len + 2)  ; i < temp_username_len + 2 + 1024 ; i++){
        if(rx_buffer[i] == 0x20){
            break;
        }else{
            temp_password[temp_password_len] = rx_buffer[i];
            temp_password_len++;
        }
    }
#ifdef TCP_OTA_DB        
    printf("Password: %s %d %d\n" , temp_password , temp_password_len , strlen(OTA_PASSWORD));
#endif

#ifdef CHECK_OTA_CREDENTIAL

    if(strlen(OTA_USERNAME) != temp_username_len || strlen(OTA_PASSWORD) != temp_password_len ){
        
        tcp_ota_response(sock , TCP_OTA_ERROR_UPDATE_FIRMWARE_RESPONSE , 6 , 
                OTA_ERROR_UPDATE_FIRMWARE_HEADER , OTA_ERROR_UPDATE_FIRMWARE_FOOTER);

        return;
    }

    if(memcmp(temp_username , OTA_USERNAME , temp_username_len) != 0 ||
            memcmp(temp_password , OTA_PASSWORD , temp_password_len) != 0){
            
        tcp_ota_response(sock , TCP_OTA_ERROR_UPDATE_FIRMWARE_RESPONSE , 6 , 
                OTA_ERROR_UPDATE_FIRMWARE_HEADER , OTA_ERROR_UPDATE_FIRMWARE_FOOTER);

        return;
    }

#endif

    flash_erase_write_boot_partition(rx_buffer[1 + temp_username_len + 1 + temp_password_len + 1]);
    
    update_firmware_buffer_size = 0;
    update_firmware_mode = false;

    tcp_ota_response(sock , TCP_OTA_SET_BOOT_PARTITION_RESPONSE , 6 , 
                            OTA_SET_BOOT_PARTITION_HEADER , OTA_SET_BOOT_PARTITION_FOOTER);

    vTaskDelay(pdMS_TO_TICKS(300));

    esp_restart();

}

void tcp_get_hardware_firmware_version(int sock , char *rx_buffer){

    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }

    new_update_firmware_data_received = true;

    char  temp_response[64] = {0};

    int len = sprintf(temp_response , "%s%s,%s" , TCP_FW_HW_VERSION_RESPONSE , FIRMWARE_VERSION , HARDWARE_VERSION);

    tcp_ota_response(sock , temp_response , len , 
                        GET_HARDWARE_FIRMWARE_VERSION_HEADER , GET_HARDWARE_FIRMWARE_VERSION_FOOTER);
            
}

void tcp_cancel_ota_update_firmware(int sock , char *rx_buffer){

    if(!is_checksum_correct(rx_buffer , 0 , 5 , 6)){
        return;
    }
    stop_firmware_timeout = true;
    tcp_rec_data_counter = 0;
    update_firmware_mode = false;

    tcp_ota_response(sock , TCP_OTA_CANCEL_UPDATE_FIRMWARE_RESPONSE , 6 , 
                        OTA_CANCEL_UPDATE_FIRMWARE_HEADER , OTA_CANCEL_UPDATE_FIRMWARE_FOOTER);
            
}