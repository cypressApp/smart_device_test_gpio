
#undef OTA_DB

extern int last_erase_offset;

void ota_read_otadata_partition();
void ota_read_factory_partition();
void ota_read_ota_1_partition();
esp_err_t ota_set_boot_firmware(int partition);
void write_info_partition();
void read_info_partition();

esp_err_t flash_store_wifi_router_info(char *ssid , char *password , char *new_wifi_mode , int ssid_len, int password_len);
esp_err_t flash_read_wifi_router_info();

void ota_set_new_write_partition(size_t type , size_t subtype);
void ota_set_new_erase_partition(size_t type , size_t subtype);
void ota_set_new_read_partition (size_t type , size_t subtype);
void ota_set_new_erase_write_partition(size_t type , size_t subtype);

esp_err_t ota_write(long int offset , char *data , long int data_length);
esp_err_t ota_erase(size_t   offset , size_t  length);
esp_err_t ota_read (long int offset , char *data , long int data_length);
esp_err_t ota_erase_write(long int offset , char *data , long int data_length);