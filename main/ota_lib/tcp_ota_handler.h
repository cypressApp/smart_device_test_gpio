
#define CHECK_OTA_CREDENTIAL 

#define OTA_USERNAME  "Cypress"
#define OTA_PASSWORD  "123"

#define SELECT_OTA_WRITE_PARTITION_HEADER         0x84                       
#define SELECT_OTA_WRITE_PARTITION_FOOTER         0x0F                       
#define SELECT_OTA_ERASE_PARTITION_HEADER         0x87                       
#define SELECT_OTA_ERASE_PARTITION_FOOTER         0x0F                       
#define SELECT_OTA_READ_PARTITION_HEADER          0x89                       
#define SELECT_OTA_READ_PARTITION_FOOTER          0x0F                       
#define SELECT_OTA_ERASE_WRITE_PARTITION_HEADER   0x91                       
#define SELECT_OTA_ERASE_WRITE_PARTITION_FOOTER   0x0F                       
#define OTA_WRITE_TO_PARTITION_HEADER             0x8A                       
#define OTA_WRITE_TO_PARTITION_FOOTER             0x0F                       
#define OTA_READ_FROM_PARTITION_HEADER            0x8C                       
#define OTA_READ_FROM_PARTITION_FOOTER            0x0F                       
#define OTA_ERASE_PARTITION_HEADER                0x8D                       
#define OTA_ERASE_PARTITION_FOOTER                0x0F                       
#define OTA_ERASE_WRITE_PARTITION_HEADER          0x93                       
#define OTA_ERASE_WRITE_PARTITION_FOOTER          0x0F                       
#define OTA_END_WRITE_PARTITION_HEADER            0x8F                       
#define OTA_END_WRITE_PARTITION_FOOTER            0x0F                       
#define OTA_RESPONSE_HEADER                       0x94                       
#define OTA_RESPONSE_FOOTER                       0x0F                       
#define OTA_START_UPDATE_FIRMWARE_HEADER          0x95                       
#define OTA_START_UPDATE_FIRMWARE_FOOTER          0x0F 
#define OTA_END_UPDATE_FIRMWARE_HEADER            0x96                       
#define OTA_END_UPDATE_FIRMWARE_FOOTER            0x0F                       
#define GET_HARDWARE_FIRMWARE_VERSION_HEADER      0x8E                       
#define GET_HARDWARE_FIRMWARE_VERSION_FOOTER      0x0F 
#define OTA_ERROR_UPDATE_FIRMWARE_HEADER          0x97                       
#define OTA_ERROR_UPDATE_FIRMWARE_FOOTER          0x0F 
#define OTA_CANCEL_UPDATE_FIRMWARE_HEADER         0x98                       
#define OTA_CANCEL_UPDATE_FIRMWARE_FOOTER         0x0F 
#define OTA_SET_BOOT_PARTITION_HEADER             0x99               
#define OTA_SET_BOOT_PARTITION_FOOTER             0x0F

#define TCP_OTA_SELECET_OTA_WRITE_RESPONSE       "ACKS_W"
#define TCP_OTA_SELECET_OTA_ERASE_RESPONSE       "ACKS_E"
#define TCP_OTA_SELECET_OTA_READ_RESPONSE        "ACKS_R"
#define TCP_OTA_SELECET_OTA_ERASE_WRITE_RESPONSE "ACKSEW"
#define TCP_OTA_WRITE_RESPONSE                   "ACKO_W"
#define TCP_OTA_ERASE_RESPONSE                   "ACKO_E"
#define TCP_OTA_READ_RESPONSE                    "ACKO_R"
#define TCP_OTA_ERASE_WRITE_RESPONSE             "ACKOEW"
#define TCP_OTA_START_UPDATE_FIRMWARE_RESPONSE   "ACKSUF"
#define TCP_OTA_ERROR_UPDATE_FIRMWARE_RESPONSE   "ACKERR"
#define TCP_OTA_END_UPDATE_FIRMWARE_RESPONSE     "ACKEUF"
#define TCP_OTA_CANCEL_UPDATE_FIRMWARE_RESPONSE  "ACKCAN"
#define TCP_FW_HW_VERSION_RESPONSE               "ACKVER"
#define TCP_OTA_SET_BOOT_PARTITION_RESPONSE      "ACKSBP"

extern bool update_firmware_mode;
extern int  update_firmware_buffer_size;

bool ota_command_handler                 (char* rx_buffer , int rx_buffer_len , int sock);
void tcp_ota_response                    (int sock , char *response , int response_length , char header , char footer);
void tcp_select_ota_write_partition      (int sock , char *rx_buffer);
void tcp_select_ota_read_partition       (int sock , char *rx_buffer);
void tcp_select_ota_erase_partition      (int sock , char *rx_buffer);
void tcp_ota_write_to_partition          (int sock , char *rx_buffer);
void tcp_ota_read_from_partition         (int sock , char *rx_buffer);
void tcp_ota_erase_partition             (int sock , char *rx_buffer);
void tcp_ota_erase_write_partition       (int sock , char *rx_buffer , long int data_size);
void tcp_select_ota_erase_write_partition(int sock , char *rx_buffer);
void tcp_start_ota_update_firmware       (int sock , char *rx_buffer);
void tcp_end_ota_update_firmware         (int sock , char *rx_buffer , long int data_size);
void tcp_set_boot_partition              (int sock , char *rx_buffer);
void tcp_get_hardware_firmware_version   (int sock , char *rx_buffer);
void tcp_cancel_ota_update_firmware      (int sock , char *rx_buffer);