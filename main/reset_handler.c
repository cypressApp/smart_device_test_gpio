#include "context.h"

void reset_handler(){
    
    if(flash_read_reset_counter() == ESP_OK){
        registeration_reset_counter++;

        if(registeration_reset_counter > 4){
            flash_store_wifi_router_info("0" , "0" , (char *) WIFI_AP_MODE , 2 , 2);
            registeration_reset_counter = 0;
            flash_erase_write_reset_counter(registeration_reset_counter);
            esp_restart();            
        }else{
            flash_erase_write_reset_counter(registeration_reset_counter);
        }

    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    registeration_reset_counter = 0;
    flash_erase_write_reset_counter(registeration_reset_counter);

}