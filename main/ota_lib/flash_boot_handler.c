#include "../context.h"

esp_err_t flash_read_boot_partition(){
    
    ota_set_new_read_partition ((size_t) BOOT_PARTION_TYPE , (size_t) BOOT_PARTITION_SUBTYPE);

    char temp_boot_partition[FLASH_BOOT_PARTITION_SIZE + 1];

    esp_err_t result = ota_read(FLASH_BOOT_PARTITION_OFFSET , temp_boot_partition , FLASH_BOOT_PARTITION_SIZE);

    if(result != ESP_OK){
        return result;
    }

#ifdef FLASH_BOOT_DB
    printf("Boot Partition: %.2X\r\n" , temp_boot_partition[1]);
#endif

    if(memcmp(temp_boot_partition , FLASH_BOOT_PARTITION_KEY , FLASH_BOOT_PARTITION_KEY_SIZE) == 0){
        
        if(temp_boot_partition[1] == ESP_PARTITION_SUBTYPE_APP_FACTORY ||
           temp_boot_partition[1] == ESP_PARTITION_SUBTYPE_APP_OTA_0   || 
           temp_boot_partition[1] == ESP_PARTITION_SUBTYPE_APP_OTA_1   ){

            if(ota_set_boot_firmware(temp_boot_partition[1]) == ESP_OK){
                flash_erase_boot_partition();
                esp_restart();
            }

        }

    }

    return ESP_OK;

}

esp_err_t flash_erase_boot_partition(){

    ota_set_new_erase_partition((size_t) BOOT_PARTION_TYPE , (size_t) BOOT_PARTITION_SUBTYPE);

    return ota_erase(FLASH_BOOT_PARTITION_OFFSET , 4096);

}

esp_err_t flash_erase_write_boot_partition(char boot_partition){

    ota_set_new_erase_write_partition((size_t) BOOT_PARTION_TYPE , (size_t) BOOT_PARTITION_SUBTYPE);

    char temp[8];
    temp[0] = FLASH_BOOT_PARTITION_KEY[0]; 
    temp[1] = boot_partition;
    return ota_erase_write(FLASH_BOOT_PARTITION_OFFSET , temp , 2);

}