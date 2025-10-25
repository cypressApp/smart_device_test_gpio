
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "constants.h"
#include "esp_log.h"

extern char firmwareUrl[4096];

void ota_task(void *pvParameter);