#include "http_ota_handler.h"


char firmwareUrl[4096] = "https://cypressterminalkmp859943801f75ec42958c747b781c886fea-sarvdev.s3.eu-north-1.amazonaws.com/protected/mjbahmanii%40gmail.com/terminal12/Defined_devtype121_HW_1.2_FW_1.3_firmware_20251018225257.txt?response-content-disposition=inline&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Security-Token=IQoJb3JpZ2luX2VjEMX%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaCmV1LW5vcnRoLTEiRjBEAiABA8TbCd57ReOcsv9n8k8alQPkCsytmrmZCQNNQVR25QIgXMAzaxb6Cgz%2FTKJZEo4UlGrAjiTZ8pcqK0CSeNR7BCcquQMIfxAAGgwzODQ2NDg4MDQwNzUiDPf5L15rjFAz4Ws4PyqWA%2FVe6L5YKAQDMrYMSbOzFpvQ0ym5N5KGuSRgacUy4oL52YaabkQYdFzcQHVSvrFLRJ9oSDpsSjsy2o%2FPk3DfykXcjt%2Fwi3YdNwCgvhvGNw3iezvhdhUdxt8tckOfQ1Ss55WC1bnIHlt0THi%2FAvxl7oSqldeP1FPVR%2Fg0AZDmiROANbBBMMJlsRn5Vltyemz8zMdqVNYCIxqUsODGxdxyjfM13rY4LgY%2F1c8mBxX1HeR9JMRYGMeW0pjNmWfOvP%2BfGYo2ZDCjrZ36gyJSUg5mjds0NxXIZLgQCqbmFH3xOeYFvPU1766rC7kNqKqT09IKYrfGNLvag%2FSdFLuHEU%2BJ7rcvVEDRcXacnHo8p7fl%2BzagOf%2FzT%2FBL4MQJklJXpJwNco6%2BqpV6RCTjeVWQ7POlE14OuIU%2FclCt2Rq1pdzuQ3NllQLZJmY%2FchM6PqgWHVxL%2Fi53Huqq32%2BDEFwCluohKRCEjk%2BBWT3%2BFNsjpzrzTtqG460%2BVGR%2BRR%2BNvFHHPY4rxgXdjjN4hhbVVkSub%2FbgHXTd6RjGIGgw37zyxwY63wLiVpO3WX3tpRDdgEINB0aho5gxz9jBiUsAXNDeqsX7HCHVCyd%2Byf4spDSTg%2FrLI4NRlNZs6iYbSEh1u5XsHjWLpb6H1RZzJ6Ipr2Kyz3NWZ5nFshODaqnbQ5xqTx8semKJEs9Nuk8MtSbhn5GuZXdk7%2FcAaMmVG6ogQqty2HYJkRf%2FDIZzLnePXLgZPPY7qOsFJbdG8e0F7jW4dgAZSn4bpNyhBMYb8jBxk%2BrVJ7HaozKvxhOkfAzjjUxFDAjRCVJuYZGZKW7QlWqRKatJ66tHdj26aIsDMZdSdKUKp7xKDylUHBlyOl5KgiAuCkE0iPY8E9h7Go1G4sLi1rk9hqepGkren5VkZbHoeK6Fq00QxaqWJxDEB4RoInbyXhp8LwyqAS%2BemPdNfv%2FwQu49t1a1YX7I4C6fu8ewPMKVRrwzSvptZLtqXw5Sbs1EJcsgN6YZCvVrEYwIUfx45NdCVTE%3D&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIAVTDW263V7X7V5TD4%2F20251025%2Feu-north-1%2Fs3%2Faws4_request&X-Amz-Date=20251025T213611Z&X-Amz-Expires=43200&X-Amz-SignedHeaders=host&X-Amz-Signature=a4eec0a58f2f5e74a7c4a84bdd7408821d03f36920d8210673e48e4ab2c40095";

void ota_task(void *pvParameter){

    ESP_LOGI(TAG, "Starting OTA update...");

    esp_http_client_config_t http_config = {
        .url = firmwareUrl,
        .cert_pem = AWS_IOT_ROOT_CA,
        .buffer_size = 4096,
        .buffer_size_tx = 4096,
        .skip_cert_common_name_check = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };
    
    // esp_err_t ret = esp_https_ota(&ota_config);
    // if (ret == ESP_OK) {
    //     esp_restart();
    // } else {
    //     ESP_LOGE("OTA", "Firmware upgrade failed");
    // }

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t ret = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA Begin failed");
        return;
    }

    // Get total size (Content-Length from HTTP headers)
    int total_size = esp_https_ota_get_image_size(https_ota_handle);
    ESP_LOGI(TAG, "Firmware size: %d bytes", total_size);

    int last_percent = -1;
    while (1) {
        ret = esp_https_ota_perform(https_ota_handle);
        if (ret != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        // Get how much has been read
        int bytes_read = esp_https_ota_get_image_len_read(https_ota_handle);

        if (total_size > 0) {
            int percent = (bytes_read * 100) / total_size;
            if (percent != last_percent) {
                ESP_LOGI(TAG, "Downloaded %d%%", percent);
                last_percent = percent;
            }
        } else {
            ESP_LOGI(TAG, "Downloaded %d bytes", bytes_read);
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) == true) {
        ESP_LOGI(TAG, "OTA Success, restarting...");
        esp_https_ota_finish(https_ota_handle);
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Failed");
        esp_https_ota_abort(https_ota_handle);
}

    vTaskDelete(NULL);
}