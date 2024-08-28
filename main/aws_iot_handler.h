#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "demo_config.h"
#include "core_mqtt.h"
#include "core_mqtt_state.h"
#include "network_transport.h"
#include "backoff_algorithm.h"
#include "clock.h"
#include "constants.h"

#undef DB_AWS_IOT

#ifdef CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR
    #include "esp_secure_cert_read.h"    
#endif

#ifndef CLIENT_IDENTIFIER
    #error "Please define a unique client identifier, CLIENT_IDENTIFIER, in menuconfig"
#endif

/* The AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client. */
#ifdef CLIENT_USERNAME
/* If a username is defined, a client password also would need to be defined for
 * client authentication. */
    #ifndef CLIENT_PASSWORD
        #error "Please define client password(CLIENT_PASSWORD) in demo_config.h for client authentication based on username/password."
    #endif
/* AWS IoT MQTT broker port needs to be 443 for client authentication based on
 * username/password. */
    #if AWS_MQTT_PORT != 443
        #error "Broker port, AWS_MQTT_PORT, should be defined as 443 in demo_config.h for client authentication based on username/password."
    #endif
#else /* !CLIENT_USERNAME */
/*
 *!!! Please note democonfigCLIENT_PRIVATE_KEY_PEM in used for
 *!!! convenience of demonstration only.  Production devices should
 *!!! store keys securely, such as within a secure element.
 */
    #ifndef CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR
        extern const char client_cert_start[] asm("_binary_client_crt_start");
        extern const char client_cert_end[] asm("_binary_client_crt_end");
        extern const char client_key_start[] asm("_binary_client_key_start");
        extern const char client_key_end[] asm("_binary_client_key_end");
    #endif /* CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR */
#endif /* CLIENT_USERNAME */

extern MQTTContext_t mqttContext;

extern const char root_cert_auth_start[]   asm("_binary_root_cert_auth_crt_start");
extern const char root_cert_auth_end[]   asm("_binary_root_cert_auth_crt_end");

#define AWS_IOT_ENDPOINT_LENGTH         ( ( uint16_t ) ( sizeof( AWS_IOT_ENDPOINT ) - 1 ) )
#define CLIENT_IDENTIFIER_LENGTH        ( ( uint16_t ) ( sizeof( CLIENT_IDENTIFIER ) - 1 ) )
#define AWS_IOT_MQTT_ALPN               "x-amzn-mqtt-ca"
#define AWS_IOT_MQTT_ALPN_LENGTH        ( ( uint16_t ) ( sizeof( AWS_IOT_MQTT_ALPN ) - 1 ) )
#define AWS_IOT_PASSWORD_ALPN           "mqtt"
#define AWS_IOT_PASSWORD_ALPN_LENGTH    ( ( uint16_t ) ( sizeof( AWS_IOT_PASSWORD_ALPN ) - 1 ) )
#define CONNECTION_RETRY_MAX_ATTEMPTS            ( 5U )
#define CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS    ( 5000U )
#define CONNECTION_RETRY_BACKOFF_BASE_MS         ( 500U )
#define CONNACK_RECV_TIMEOUT_MS                  ( 1000U )

#define MQTT_COMMAND_TOPIC                  TERMINAL_NAME "/" DEVICE_TYPE "/" MAJOR_ID MINOR_ID "/command"  
#define MQTT_COMMAND_TOPIC_LENGTH           ( ( uint16_t ) ( sizeof( MQTT_COMMAND_TOPIC ) - 1 ) )

#define MQTT_RESPONSE_TOPIC                  TERMINAL_NAME "/" DEVICE_TYPE "/" MAJOR_ID MINOR_ID "/response"  
#define MQTT_RESPONSE_TOPIC_LENGTH           ( ( uint16_t ) ( sizeof( MQTT_RESPONSE_TOPIC ) - 1 ) )

#define MAX_OUTGOING_PUBLISHES              ( 5U )
#define MQTT_PACKET_ID_INVALID              ( ( uint16_t ) 0U )
#define MQTT_PROCESS_LOOP_TIMEOUT_MS        ( 5000U )
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    ( 60U )
#define DELAY_BETWEEN_PUBLISHES_SECONDS     ( 1U )
#define MQTT_PUBLISH_COUNT_PER_LOOP         ( 5U )
#define MQTT_SUBPUB_LOOP_DELAY_SECONDS      ( 2U )
#define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 1500U )
#define METRICS_STRING                      "?SDK=" OS_NAME "&Version=" OS_VERSION "&Platform=" HARDWARE_PLATFORM_NAME "&MQTTLib=" MQTT_LIB
#define METRICS_STRING_LENGTH               ( ( uint16_t ) ( sizeof( METRICS_STRING ) - 1 ) )


#ifdef CLIENT_USERNAME

/**
 * @brief Append the username with the metrics string if #CLIENT_USERNAME is defined.
 *
 * This is to support both metrics reporting and username/password based client
 * authentication by AWS IoT.
 */
    #define CLIENT_USERNAME_WITH_METRICS    CLIENT_USERNAME METRICS_STRING
#endif
#define OUTGOING_PUBLISH_RECORD_LEN    ( 10U )
#define INCOMING_PUBLISH_RECORD_LEN    ( 10U )

extern uint16_t globalAckPacketIdentifier;
extern uint16_t globalSubscribePacketIdentifier;
extern uint16_t globalUnsubscribePacketIdentifier;
extern MQTTSubscribeInfo_t pGlobalSubscriptionList[ 1 ];
extern uint8_t buffer[ NETWORK_BUFFER_SIZE ];
extern MQTTSubAckStatus_t globalSubAckStatus;
extern MQTTPubAckInfo_t pOutgoingPublishRecords[ OUTGOING_PUBLISH_RECORD_LEN ];
extern MQTTPubAckInfo_t pIncomingPublishRecords[ INCOMING_PUBLISH_RECORD_LEN ];
extern StaticSemaphore_t xTlsContextSemaphoreBuffer;

void aws_iot_task(void *pvParameters);
void command_buffer_handler();
int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent );
int publishToTopic( MQTTContext_t * pMqttContext , char *msg_topic  , char *message );