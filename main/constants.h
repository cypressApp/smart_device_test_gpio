
#include <stdio.h>

#define TAG "Cypress"

#define WIFI_STA_MODE_TASK_STACK_DEPTH       20480
#define AWS_IOT_TASK_STACK_DEPTH             20480
#define CHECK_TCP_TIMEOUT_TASK_STACK_DEPTH   4096

#define HARDWARE_VERSION "01.20"
#define FIRMWARE_VERSION "01.20"

#define IS_REMOTE_CON_ENABLE 1

#define DEVICE_UNIQUE_ID_STR    "0"
#define DEVICE_MAC_ADDRESS_STR  "C0428A15753C"
#define ESP_AP_WIFI_SSID        "Cypress_WiFi_15753C"
#define ESP_AP_WIFI_PASS        "123456789"  

#define SET_ROUTER_SSID_PREFIX "ssid:"
#define SET_ROUTER_SSID_SUFFIX "ends"
#define SET_ROUTER_PASS_PREFIX "pass:"
#define SET_ROUTER_PASS_SUFFIX "endp"

#define TCP_PORT    1234
#define UDP_PORT    1234

#define SCAN_COMMAND     "GET_INFO\r\n"
#define SCAN_COMMAND_LEN (sizeof(SCAN_COMMAND) - 3)

#define AWS_SCAN_RESPONSE     "ACK"
#define AWS_SCAN_RESPONSE_LEN (sizeof(AWS_SCAN_RESPONSE) - 1)

#define DEVICE_NAME   "device_name"    // Device name
#define DEVICE_TYPE   "dev_type"       // Must be lowercase: Device type in phone app
#define TERMINAL_NAME "terminal_name"  // Must be lowercase: Terminal name in phone app 
#define MAJOR_ID      ""               // Unique ID in phone app
#define MINOR_ID      "12345"          // Each define should have a unique minorId

#define AWS_IOT_CERTIFICATE ""

#define AWS_IOT_PRIVATE_KEY ""

// Do not change root ca
#define AWS_IOT_ROOT_CA "-----BEGIN CERTIFICATE-----\n\
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n\
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n\
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n\
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n\
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n\
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n\
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n\
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n\
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n\
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n\
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n\
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n\
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n\
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n\
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n\
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n\
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n\
rqXRfboQnoZsG4q5WTP468SQvvG5\n\
-----END CERTIFICATE-----"
