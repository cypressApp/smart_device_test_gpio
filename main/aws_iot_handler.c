#include "context.h"
#include "freertos/semphr.h"

bool is_connected_to_server = false;

typedef struct PublishPackets
{
    uint16_t packetId;
    MQTTPublishInfo_t pubInfo;
} PublishPackets_t;

uint16_t globalAckPacketIdentifier = 0U;
uint16_t globalSubscribePacketIdentifier = 0U;
uint16_t globalUnsubscribePacketIdentifier = 0U;
PublishPackets_t outgoingPublishPackets[ MAX_OUTGOING_PUBLISHES ] = { 0 };
MQTTSubscribeInfo_t pGlobalSubscriptionList[ 1 ];
uint8_t buffer[ NETWORK_BUFFER_SIZE ];
MQTTSubAckStatus_t globalSubAckStatus = MQTTSubAckFailure;
MQTTPubAckInfo_t pOutgoingPublishRecords[ OUTGOING_PUBLISH_RECORD_LEN ];
MQTTPubAckInfo_t pIncomingPublishRecords[ INCOMING_PUBLISH_RECORD_LEN ];
StaticSemaphore_t xTlsContextSemaphoreBuffer;

char aws_iot_command_buffer[1024][32];
int new_aws_iot_command_buffer_counter = 0;
int last_aws_iot_command_buffer_counter = 0;
char is_aws_iot_command_buffer_handler_idle = true;

// void command_buffer_handler(){
    
//     if(!is_aws_iot_command_buffer_handler_idle){
//         return
//     }

//     is_aws_iot_command_buffer_handler_idle = false;

//     while((last_aws_iot_command_buffer_counter < new_aws_iot_command_buffer_counter) || 
//             (last_aws_iot_command_buffer_counter > 900) && (last_aws_iot_command_buffer_counter < 200) ){
                
//         printf("%s\r\n" , aws_iot_command_buffer[last_aws_iot_command_buffer_counter]);

//         last_aws_iot_command_buffer_counter++;
//         if(last_aws_iot_command_buffer_counter >= 2048){
//             last_aws_iot_command_buffer_counter = 0;
//         }
//     }
    
//     is_aws_iot_command_buffer_handler_idle = true;

// }

static int initializeMqtt( MQTTContext_t * pMqttContext,
                           NetworkContext_t * pNetworkContext );

void process_aws_iot_command( MQTTContext_t * pMqttContext, const char *command , int command_length){

    memcpy( aws_iot_command_buffer[new_aws_iot_command_buffer_counter] , command , sizeof(aws_iot_command_buffer[new_aws_iot_command_buffer_counter]));
    new_aws_iot_command_buffer_counter++;
    if(new_aws_iot_command_buffer_counter >= 1024){
        new_aws_iot_command_buffer_counter = 0;
    }

    char *temp_response = (char *) calloc(256 , sizeof(char)); 
    bool is_command_valid = true;

    if(!memcmp(command, SCAN_COMMAND , SCAN_COMMAND_LEN)){
        memcpy(temp_response , AWS_SCAN_RESPONSE , AWS_SCAN_RESPONSE_LEN);
        publishToTopic(pMqttContext , MQTT_RESPONSE_TOPIC , temp_response);
        free(temp_response);
        return;
    }
    /**
     * For handling remote commands enter your code here
     */
    // else if(/* command */){


        // is_command_valid = true;
    // }
    else{
        is_command_valid = false;
    }

    if(is_command_valid){
        update_response_id();
        char *temp_response2 = (char *) calloc(256 , sizeof(char)); 
        sprintf(temp_response2, "%d:%s", response_id , temp_response);
        publishToTopic(pMqttContext , MQTT_RESPONSE_TOPIC , temp_response2);
    }
#ifdef DB_AWS_IOT
    printf("%.*s   %d\r\n" , command_length , command , command_length);
#endif
    free(temp_response);

}


static uint32_t generateRandomNumber()
{
    return( rand() );
}

static void cleanupESPSecureMgrCerts( NetworkContext_t * pNetworkContext )
{
#ifdef CONFIG_EXAMPLE_USE_SECURE_ELEMENT
    /* Nothing to be freed */
#elif defined(CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR)
    esp_secure_cert_free_device_cert(&pNetworkContext->pcClientCert);
#ifdef CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL
    esp_secure_cert_free_ds_ctx(pNetworkContext->ds_data);
#else /* !CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */
    esp_secure_cert_free_priv_key(&pNetworkContext->pcClientKey);
#endif /* CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */

#else /* !CONFIG_EXAMPLE_USE_SECURE_ELEMENT && !CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR  */
    /* Nothing to be freed */
#endif
    return;
}

static int connectToServerWithBackoffRetries( NetworkContext_t * pNetworkContext,
                                              MQTTContext_t * pMqttContext,
                                              bool * pClientSessionPresent,
                                              bool * pBrokerSessionPresent )
{

    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    TlsTransportStatus_t tlsStatus = TLS_TRANSPORT_SUCCESS;
    BackoffAlgorithmContext_t reconnectParams;
    bool createCleanSession;

    pNetworkContext->pcHostname = AWS_IOT_ENDPOINT;
    pNetworkContext->xPort = AWS_MQTT_PORT;
    pNetworkContext->pxTls = NULL;
    pNetworkContext->xTlsContextSemaphore = xSemaphoreCreateMutexStatic(&xTlsContextSemaphoreBuffer);

    pNetworkContext->disableSni = 0;
    uint16_t nextRetryBackOff;

    /* Initialize credentials for establishing TLS session. */
    pNetworkContext->pcServerRootCA = AWS_IOT_ROOT_CA;//root_cert_auth_start;
    pNetworkContext->pcServerRootCASize = sizeof(AWS_IOT_ROOT_CA);//root_cert_auth_end - root_cert_auth_start;

    /* If #CLIENT_USERNAME is defined, username/password is used for authenticating
     * the client. */
#ifdef CONFIG_EXAMPLE_USE_SECURE_ELEMENT
    pNetworkContext->use_secure_element = true;

#elif defined(CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR)
    if (esp_secure_cert_get_device_cert(&pNetworkContext->pcClientCert, &pNetworkContext->pcClientCertSize) != ESP_OK) {
#ifdef DB_AWS_IOT        
        LogError( ( "Failed to obtain flash address of device cert") );
#endif
        return EXIT_FAILURE;
    }
#ifdef CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL
    pNetworkContext->ds_data = esp_secure_cert_get_ds_ctx();
    if (pNetworkContext->ds_data == NULL) {
#ifdef DB_AWS_IOT        
        LogError( ( "Failed to obtain the ds context") );
#endif        
        return EXIT_FAILURE;
    }
#else /* !CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */
    if (esp_secure_cert_get_priv_key(&pNetworkContext->pcClientKey, &pNetworkContext->pcClientKeySize) != ESP_OK) {
#ifdef DB_AWS_IOT        
        LogError( ( "Failed to obtain flash address of private_key") );
#endif        
        return EXIT_FAILURE;
    }
#endif /* CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */

#else /* !CONFIG_EXAMPLE_USE_SECURE_ELEMENT && !CONFIG_EXAMPLE_USE_ESP_SECURE_CERT_MGR  */
    #ifndef CLIENT_USERNAME
        pNetworkContext->pcClientCert = AWS_IOT_CERTIFICATE;//client_cert_start;
        pNetworkContext->pcClientCertSize = sizeof(AWS_IOT_CERTIFICATE);//client_cert_end - client_cert_start;
        pNetworkContext->pcClientKey = AWS_IOT_PRIVATE_KEY;//client_key_start;
        pNetworkContext->pcClientKeySize = sizeof(AWS_IOT_PRIVATE_KEY);//client_key_end - client_key_start;
    #endif
#endif
    /* AWS IoT requires devices to send the Server Name Indication (SNI)
     * extension to the Transport Layer Security (TLS) protocol and provide
     * the complete endpoint address in the host_name field. Details about
     * SNI for AWS IoT can be found in the link below.
     * https://docs.aws.amazon.com/iot/latest/developerguide/transport-security.html */

    if( AWS_MQTT_PORT == 443 )
    {
        /* Pass the ALPN protocol name depending on the port being used.
         * Please see more details about the ALPN protocol for the AWS IoT MQTT
         * endpoint in the link below.
         * https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/
         *
         * For username and password based authentication in AWS IoT,
         * #AWS_IOT_PASSWORD_ALPN is used. More details can be found in the
         * link below.
         * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
         */

        static const char * pcAlpnProtocols[] = { NULL, NULL };

        #ifdef CLIENT_USERNAME
            pcAlpnProtocols[0] = AWS_IOT_PASSWORD_ALPN;
        #else
            pcAlpnProtocols[0] = AWS_IOT_MQTT_ALPN;
        #endif

        pNetworkContext->pAlpnProtos = pcAlpnProtocols;
    } else {
        pNetworkContext->pAlpnProtos = NULL;
    }

    /* Initialize reconnect attempts and interval */
    BackoffAlgorithm_InitializeParams( &reconnectParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase until maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects
         * to the MQTT broker as specified in AWS_IOT_ENDPOINT and AWS_MQTT_PORT
         * at the demo config header. */
#ifdef DB_AWS_IOT        
        LogInfo( ( "Establishing a TLS session to %.*s:%d.",
                   AWS_IOT_ENDPOINT_LENGTH,
                   AWS_IOT_ENDPOINT,
                   AWS_MQTT_PORT ) );
#endif
        // tlsStatus = xTlsDisconnect ( pNetworkContext );
        tlsStatus = xTlsConnect ( pNetworkContext );

        if( tlsStatus == TLS_TRANSPORT_SUCCESS )
        {
            /* A clean MQTT session needs to be created, if there is no session saved
             * in this MQTT client. */
            createCleanSession = ( *pClientSessionPresent == true ) ? false : true;
            /* Sends an MQTT Connect packet using the established TLS session,
             * then waits for connection acknowledgment (CONNACK) packet. */
            returnStatus = establishMqttSession( pMqttContext, createCleanSession, pBrokerSessionPresent );

            if( returnStatus == EXIT_FAILURE )
            {
                /* End TLS session, then close TCP connection. */
                cleanupESPSecureMgrCerts( pNetworkContext );
                ( void ) xTlsDisconnect( pNetworkContext );
            }
        }else if (tlsStatus == TLS_TRANSPORT_CONNECT_FAILURE){
            cleanupESPSecureMgrCerts( pNetworkContext );
            ( void ) xTlsDisconnect( pNetworkContext );
            returnStatus = EXIT_FAILURE;
        }

        if( returnStatus == EXIT_FAILURE )
        {
            /* Generate a random number and get back-off value (in milliseconds) for the next connection retry. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &reconnectParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
#ifdef DB_AWS_IOT                
                LogError( ( "Connection to the broker failed, all attempts exhausted." ) );
#endif
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
#ifdef DB_AWS_IOT                
                LogWarn( ( "Connection to the broker failed. Retrying connection "
                           "after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff ) );
#endif
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( returnStatus == EXIT_FAILURE ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int getNextFreeIndexForOutgoingPublishes( uint8_t * pIndex )
{
    int returnStatus = EXIT_FAILURE;
    uint8_t index = 0;

    assert( outgoingPublishPackets != NULL );
    assert( pIndex != NULL );

    for( index = 0; index < MAX_OUTGOING_PUBLISHES; index++ )
    {
        /* A free index is marked by invalid packet id.
         * Check if the the index has a free slot. */
        if( outgoingPublishPackets[ index ].packetId == MQTT_PACKET_ID_INVALID )
        {
            returnStatus = EXIT_SUCCESS;
            break;
        }
    }

    /* Copy the available index into the output param. */
    *pIndex = index;

    return returnStatus;
}
/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishAt( uint8_t index )
{

    assert( outgoingPublishPackets != NULL );
    assert( index < MAX_OUTGOING_PUBLISHES );

    /* Clear the outgoing publish packet. */
    ( void ) memset( &( outgoingPublishPackets[ index ] ),
                     0x00,
                     sizeof( outgoingPublishPackets[ index ] ) );
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishes( void )
{

    assert( outgoingPublishPackets != NULL );

    /* Clean up all the outgoing publish packets. */
    ( void ) memset( outgoingPublishPackets, 0x00, sizeof( outgoingPublishPackets ) );
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishWithPacketID( uint16_t packetId )
{

    uint8_t index = 0;

    assert( outgoingPublishPackets != NULL );
    assert( packetId != MQTT_PACKET_ID_INVALID );

    /* Clean up all the saved outgoing publishes. */
    for( ; index < MAX_OUTGOING_PUBLISHES; index++ )
    {
        if( outgoingPublishPackets[ index ].packetId == packetId )
        {
            cleanupOutgoingPublishAt( index );
#ifdef DB_AWS_IOT            
            LogInfo( ( "Cleaned up outgoing publish packet with packet id %u.\n\n",
                       packetId ) );
#endif
            break;
        }
    }
}

/*-----------------------------------------------------------*/

static int handlePublishResend( MQTTContext_t * pMqttContext )
{

    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t index = 0U;
    MQTTStateCursor_t cursor = MQTT_STATE_CURSOR_INITIALIZER;
    uint16_t packetIdToResend = MQTT_PACKET_ID_INVALID;
    bool foundPacketId = false;

    assert( pMqttContext != NULL );
    assert( outgoingPublishPackets != NULL );

    /* MQTT_PublishToResend() provides a packet ID of the next PUBLISH packet
     * that should be resent. In accordance with the MQTT v3.1.1 spec,
     * MQTT_PublishToResend() preserves the ordering of when the original
     * PUBLISH packets were sent. The outgoingPublishPackets array is searched
     * through for the associated packet ID. If the application requires
     * increased efficiency in the look up of the packet ID, then a hashmap of
     * packetId key and PublishPacket_t values may be used instead. */
    packetIdToResend = MQTT_PublishToResend( pMqttContext, &cursor );

    while( packetIdToResend != MQTT_PACKET_ID_INVALID )
    {
        foundPacketId = false;

        for( index = 0U; index < MAX_OUTGOING_PUBLISHES; index++ )
        {
            if( outgoingPublishPackets[ index ].packetId == packetIdToResend )
            {
                foundPacketId = true;
                outgoingPublishPackets[ index ].pubInfo.dup = true;
#ifdef DB_AWS_IOT
                LogInfo( ( "Sending duplicate PUBLISH with packet id %u.",
                           outgoingPublishPackets[ index ].packetId ) );
#endif
                mqttStatus = MQTT_Publish( pMqttContext,
                                           &outgoingPublishPackets[ index ].pubInfo,
                                           outgoingPublishPackets[ index ].packetId );

                if( mqttStatus != MQTTSuccess )
                {
#ifdef DB_AWS_IOT                    
                    LogError( ( "Sending duplicate PUBLISH for packet id %u "
                                " failed with status %s.",
                                outgoingPublishPackets[ index ].packetId,
                                MQTT_Status_strerror( mqttStatus ) ) );
#endif
                    returnStatus = EXIT_FAILURE;
                    break;
                }
                else
                {
#ifdef DB_AWS_IOT                    
                    LogInfo( ( "Sent duplicate PUBLISH successfully for packet id %u.\n\n",
                               outgoingPublishPackets[ index ].packetId ) );
#endif
                }
            }
        }

        if( foundPacketId == false )
        {
#ifdef DB_AWS_IOT            
            LogError( ( "Packet id %u requires resend, but was not found in "
                        "outgoingPublishPackets.",
                        packetIdToResend ) );
#endif
            returnStatus = EXIT_FAILURE;
            break;
        }
        else
        {
            /* Get the next packetID to be resent. */
            packetIdToResend = MQTT_PublishToResend( pMqttContext, &cursor );
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void handleIncomingPublish( MQTTContext_t * pMqttContext, 
                                    MQTTPublishInfo_t * pPublishInfo,
                                   uint16_t packetIdentifier )
{

    assert( pPublishInfo != NULL );
#ifdef DB_AWS_IOT
    /* Process incoming Publish. */
    LogInfo( ( "Incoming QOS : %d.", pPublishInfo->qos ) );
#endif
    /* Verify the received publish is for the topic we have subscribed to. */
    if( ( pPublishInfo->topicNameLength == MQTT_COMMAND_TOPIC_LENGTH ) &&
        ( 0 == strncmp( MQTT_COMMAND_TOPIC,
                        pPublishInfo->pTopicName,
                        pPublishInfo->topicNameLength ) ) )
    {
#ifdef DB_AWS_IOT        
        LogInfo( ( "Incoming Publish Topic Name: %.*s matches subscribed topic.\n"
                   "Incoming Publish message Packet Id is %u.\n"
                   "Incoming Publish Message : %.*s.\n\n",
                   pPublishInfo->topicNameLength,
                   pPublishInfo->pTopicName,
                   packetIdentifier,
                   ( int ) pPublishInfo->payloadLength,
                   ( const char * ) pPublishInfo->pPayload ) );
#endif                   
        process_aws_iot_command(pMqttContext , ( const char * ) pPublishInfo->pPayload , ( int ) pPublishInfo->payloadLength);           
    }
    else
    {
#ifdef DB_AWS_IOT        
        LogInfo( ( "Incoming Publish Topic Name: %.*s does not match subscribed topic.",
                   pPublishInfo->topicNameLength,
                   pPublishInfo->pTopicName ) );
#endif
    }
}

/*-----------------------------------------------------------*/

static void updateSubAckStatus( MQTTPacketInfo_t * pPacketInfo )
{

    uint8_t * pPayload = NULL;
    size_t pSize = 0;

    MQTTStatus_t mqttStatus = MQTT_GetSubAckStatusCodes( pPacketInfo, &pPayload, &pSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    assert( mqttStatus == MQTTSuccess );

    /* Suppress unused variable warning when asserts are disabled in build. */
    ( void ) mqttStatus;

    /* Demo only subscribes to one topic, so only one status code is returned. */
    globalSubAckStatus = ( MQTTSubAckStatus_t ) pPayload[ 0 ];
}


static void eventCallback( MQTTContext_t * pMqttContext,
                           MQTTPacketInfo_t * pPacketInfo,
                           MQTTDeserializedInfo_t * pDeserializedInfo )
{
    uint16_t packetIdentifier;

    assert( pMqttContext != NULL );
    assert( pPacketInfo != NULL );
    assert( pDeserializedInfo != NULL );

    /* Suppress unused parameter warning when asserts are disabled in build. */
    ( void ) pMqttContext;

    packetIdentifier = pDeserializedInfo->packetIdentifier;

    /* Handle incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if( ( pPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( pDeserializedInfo->pPublishInfo != NULL );
        /* Handle incoming publish. */
        handleIncomingPublish(pMqttContext, pDeserializedInfo->pPublishInfo, packetIdentifier );
    }
    else
    {
        /* Handle other packets. */
        switch( pPacketInfo->type )
        {
            case MQTT_PACKET_TYPE_SUBACK:

                /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
                 * It contains the status code indicating server approval/rejection for the subscription to the single topic
                 * requested. The SUBACK will be parsed to obtain the status code, and this status code will be stored in global
                 * variable globalSubAckStatus. */
                updateSubAckStatus( pPacketInfo );

                /* Check status of the subscription request. If globalSubAckStatus does not indicate
                 * server refusal of the request (MQTTSubAckFailure), it contains the QoS level granted
                 * by the server, indicating a successful subscription attempt. */
                if( globalSubAckStatus != MQTTSubAckFailure )
                {
#ifdef DB_AWS_IOT                      
                    LogInfo( ( "Subscribed to the topic %.*s. with maximum QoS %u.\n\n",
                               MQTT_COMMAND_TOPIC_LENGTH,
                               MQTT_COMMAND_TOPIC,
                               globalSubAckStatus ) );
#endif                               
                }

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalSubscribePacketIdentifier == packetIdentifier );

                /* Update the global ACK packet identifier. */
                globalAckPacketIdentifier = packetIdentifier;
                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
#ifdef DB_AWS_IOT              
                LogInfo( ( "Unsubscribed from the topic %.*s.\n\n",
                           MQTT_COMMAND_TOPIC_LENGTH,
                           MQTT_COMMAND_TOPIC ) );
#endif
                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalUnsubscribePacketIdentifier == packetIdentifier );

                /* Update the global ACK packet identifier. */
                globalAckPacketIdentifier = packetIdentifier;
                break;

            case MQTT_PACKET_TYPE_PINGRESP:
#ifdef DB_AWS_IOT  
                /* Nothing to be done from application as library handles
                 * PINGRESP. */
                LogWarn( ( "PINGRESP should not be handled by the application "
                           "callback when using MQTT_ProcessLoop.\n\n" ) );
#endif                
                break;

            case MQTT_PACKET_TYPE_PUBACK:
#ifdef DB_AWS_IOT  
                LogInfo( ( "PUBACK received for packet id %u.\n\n",
                           packetIdentifier ) );
#endif
                /* Cleanup publish packet when a PUBACK is received. */
                cleanupOutgoingPublishWithPacketID( packetIdentifier );

                /* Update the global ACK packet identifier. */
                globalAckPacketIdentifier = packetIdentifier;
                break;

            /* Any other packet type is invalid. */
            default:
#ifdef DB_AWS_IOT  
                LogError( ( "Unknown packet type received:(%02x).\n\n",
                            pPacketInfo->type ) );
#endif
        }
    }
}

/*-----------------------------------------------------------*/

int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo = { 0 };

    assert( pMqttContext != NULL );
    assert( pSessionPresent != NULL );

    /* Establish MQTT session by sending a CONNECT packet. */

    /* If #createCleanSession is true, start with a clean session
     * i.e. direct the MQTT broker to discard any previous session data.
     * If #createCleanSession is false, directs the broker to attempt to
     * reestablish a session which was already present. */
    connectInfo.cleanSession = createCleanSession;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    connectInfo.pClientIdentifier = CLIENT_IDENTIFIER;
    connectInfo.clientIdentifierLength = CLIENT_IDENTIFIER_LENGTH;

    /* The maximum time interval in seconds which is allowed to elapse
     * between two Control Packets.
     * It is the responsibility of the Client to ensure that the interval between
     * Control Packets being sent does not exceed the this Keep Alive value. In the
     * absence of sending any other Control Packets, the Client MUST send a
     * PINGREQ Packet. */
    connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    /* Use the username and password for authentication, if they are defined.
     * Refer to the AWS IoT documentation below for details regarding client
     * authentication with a username and password.
     * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
     * An authorizer setup needs to be done, as mentioned in the above link, to use
     * username/password based client authentication.
     *
     * The username field is populated with voluntary metrics to AWS IoT.
     * The metrics collected by AWS IoT are the operating system, the operating
     * system's version, the hardware platform, and the MQTT Client library
     * information. These metrics help AWS IoT improve security and provide
     * better technical support.
     *
     * If client authentication is based on username/password in AWS IoT,
     * the metrics string is appended to the username to support both client
     * authentication and metrics collection. */
    #ifdef CLIENT_USERNAME
        connectInfo.pUserName = CLIENT_USERNAME_WITH_METRICS;
        connectInfo.userNameLength = strlen( CLIENT_USERNAME_WITH_METRICS );
        connectInfo.pPassword = CLIENT_PASSWORD;
        connectInfo.passwordLength = strlen( CLIENT_PASSWORD );
    #else
        connectInfo.pUserName = METRICS_STRING;
        connectInfo.userNameLength = METRICS_STRING_LENGTH;
        /* Password for authentication is not used. */
        connectInfo.pPassword = NULL;
        connectInfo.passwordLength = 0U;
    #endif /* ifdef CLIENT_USERNAME */

    /* Send MQTT CONNECT packet to broker. */
    mqttStatus = MQTT_Connect( pMqttContext, &connectInfo, NULL, CONNACK_RECV_TIMEOUT_MS, pSessionPresent );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
#ifdef DB_AWS_IOT          
        LogError( ( "Connection with MQTT broker failed with status %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
#endif    
    }
    else
    {
#ifdef DB_AWS_IOT          
        LogInfo( ( "MQTT connection successfully established with broker.\n\n" ) );
#endif    
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int disconnectMqttSession( MQTTContext_t * pMqttContext )
{
    MQTTStatus_t mqttStatus = MQTTSuccess;
    int returnStatus = EXIT_SUCCESS;

    assert( pMqttContext != NULL );

    /* Send DISCONNECT. */
    mqttStatus = MQTT_Disconnect( pMqttContext );

    if( mqttStatus != MQTTSuccess )
    {
#ifdef DB_AWS_IOT          
        LogError( ( "Sending MQTT DISCONNECT failed with status=%s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
#endif                    
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int subscribeToTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS1;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = MQTT_COMMAND_TOPIC;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = MQTT_COMMAND_TOPIC_LENGTH;

    /* Generate packet identifier for the SUBSCRIBE packet. */
    globalSubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send SUBSCRIBE packet. */
    mqttStatus = MQTT_Subscribe( pMqttContext,
                                 pGlobalSubscriptionList,
                                 sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                 globalSubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
#ifdef DB_AWS_IOT          
        LogError( ( "Failed to send SUBSCRIBE packet to broker with error = %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
#endif                    
        returnStatus = EXIT_FAILURE;
    }
    else
    {
#ifdef DB_AWS_IOT          
        LogInfo( ( "SUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_COMMAND_TOPIC_LENGTH,
                   MQTT_COMMAND_TOPIC ) );
#endif
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int unsubscribeFromTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    /* This example subscribes to and unsubscribes from only one topic
     * and uses QOS1. */
    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS1;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = MQTT_COMMAND_TOPIC;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = MQTT_COMMAND_TOPIC_LENGTH;

    /* Generate packet identifier for the UNSUBSCRIBE packet. */
    globalUnsubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send UNSUBSCRIBE packet. */
    mqttStatus = MQTT_Unsubscribe( pMqttContext,
                                   pGlobalSubscriptionList,
                                   sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                   globalUnsubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
#ifdef DB_AWS_IOT  
        LogError( ( "Failed to send UNSUBSCRIBE packet to broker with error = %s.",
                    MQTT_Status_strerror( mqttStatus ) ) );
#endif
        returnStatus = EXIT_FAILURE;
    }
    else
    {
#ifdef DB_AWS_IOT          
        LogInfo( ( "UNSUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_COMMAND_TOPIC_LENGTH,
                   MQTT_COMMAND_TOPIC ) );
#endif
    }

    return returnStatus;
}


int publishToTopic( MQTTContext_t * pMqttContext , char *msg_topic , char *message )
{
    
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t publishIndex = MAX_OUTGOING_PUBLISHES;

    assert( pMqttContext != NULL );


    /* Get the next free index for the outgoing publish. All QoS1 outgoing
     * publishes are stored until a PUBACK is received. These messages are
     * stored for supporting a resend if a network connection is broken before
     * receiving a PUBACK. */
    returnStatus = getNextFreeIndexForOutgoingPublishes( &publishIndex );

    if( returnStatus == EXIT_FAILURE )
    {
#ifdef DB_AWS_IOT          
        LogError( ( "Unable to find a free spot for outgoing PUBLISH message.\n\n" ) );
#endif
    }
    else
    {
        /* This example publishes to only one topic and uses QOS1. */
        outgoingPublishPackets[ publishIndex ].pubInfo.qos = MQTTQoS1;
        outgoingPublishPackets[ publishIndex ].pubInfo.pTopicName = msg_topic;
        outgoingPublishPackets[ publishIndex ].pubInfo.topicNameLength = strlen(msg_topic);
        outgoingPublishPackets[ publishIndex ].pubInfo.pPayload = message;
        outgoingPublishPackets[ publishIndex ].pubInfo.payloadLength = strlen(message);
        outgoingPublishPackets[ publishIndex ].pubInfo.retain = false;
        outgoingPublishPackets[ publishIndex ].pubInfo.dup = false;

        /* Get a new packet id. */
        outgoingPublishPackets[ publishIndex ].packetId = MQTT_GetPacketId( pMqttContext );

        /* Send PUBLISH packet. */
        mqttStatus = MQTT_Publish( pMqttContext,
                                &outgoingPublishPackets[ publishIndex ].pubInfo,
                                outgoingPublishPackets[ publishIndex ].packetId ); //

        if( mqttStatus != MQTTSuccess )
        {
#ifdef DB_AWS_IOT  
            LogError( ( "Failed to send PUBLISH packet to broker with error = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
#endif
            cleanupOutgoingPublishAt( publishIndex );
            // reconnect_to_server();
            returnStatus = EXIT_FAILURE;
        }
        else
        {
            // cleanupOutgoingPublishAt( publishIndex );
#ifdef DB_AWS_IOT  
            LogInfo( ( "PUBLISH sent for topic %.*s to broker with packet ID %u.\n\n",
                    MQTT_RESPONSE_TOPIC_LENGTH,
                    MQTT_RESPONSE_TOPIC,
                    outgoingPublishPackets[ publishIndex ].packetId ) );
#endif
        }



    }
    return returnStatus;
}

static int initializeMqtt( MQTTContext_t * pMqttContext,
                           NetworkContext_t * pNetworkContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t networkBuffer;
    TransportInterface_t transport = { 0 };

    assert( pMqttContext != NULL );
    assert( pNetworkContext != NULL );

    /* Fill in TransportInterface send and receive function pointers.
     * For this demo, TCP sockets are used to send and receive data
     * from network. Network context is SSL context for OpenSSL.*/
    transport.pNetworkContext = pNetworkContext;
    transport.send = espTlsTransportSend;
    transport.recv = espTlsTransportRecv;
    transport.writev = NULL;

    /* Fill the values for network buffer. */
    networkBuffer.pBuffer = buffer;
    networkBuffer.size = NETWORK_BUFFER_SIZE;
    /* Initialize MQTT library. */
    mqttStatus = MQTT_Init( pMqttContext,
                            &transport,
                            Clock_GetTimeMs,
                            eventCallback,
                            &networkBuffer );
    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
#ifdef DB_AWS_IOT          
        LogError( ( "MQTT_Init failed: Status = %s.", MQTT_Status_strerror( mqttStatus ) ) );
#endif
    }
    else
    {
        mqttStatus = MQTT_InitStatefulQoS( pMqttContext,
                                           pOutgoingPublishRecords,
                                           OUTGOING_PUBLISH_RECORD_LEN,
                                           pIncomingPublishRecords,
                                           INCOMING_PUBLISH_RECORD_LEN );
        if( mqttStatus != MQTTSuccess )
        {
            returnStatus = EXIT_FAILURE;
#ifdef DB_AWS_IOT  
            LogError( ( "MQTT_InitStatefulQoS failed: Status = %s.", MQTT_Status_strerror( mqttStatus ) ) );
#endif
        }
    }

    return returnStatus;
}

static int waitForPacketAck( MQTTContext_t * pMqttContext,
                             uint16_t usPacketIdentifier,
                             uint32_t ulTimeout )
{
    // uint32_t ulMqttProcessLoopEntryTime;
    uint32_t ulMqttProcessLoopTimeoutTime;
    uint32_t ulCurrentTime;

    MQTTStatus_t eMqttStatus = MQTTSuccess;
    int returnStatus = EXIT_FAILURE;

    /* Reset the ACK packet identifier being received. */
    globalAckPacketIdentifier = 0U;

    ulCurrentTime = pMqttContext->getTime();
    // ulMqttProcessLoopEntryTime = ulCurrentTime;
    ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeout;

    /* Call MQTT_ProcessLoop multiple times until the expected packet ACK
     * is received, a timeout happens, or MQTT_ProcessLoop fails. */
    while( ( globalAckPacketIdentifier != usPacketIdentifier ) &&
           ( ulCurrentTime < ulMqttProcessLoopTimeoutTime ) &&
           ( eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes ) )
    {
        /* Event callback will set #globalAckPacketIdentifier when receiving
         * appropriate packet. */
        eMqttStatus = MQTT_ProcessLoop( pMqttContext );
        ulCurrentTime = pMqttContext->getTime();
    }

    if( ( ( eMqttStatus != MQTTSuccess ) && ( eMqttStatus != MQTTNeedMoreBytes ) ) ||
        ( globalAckPacketIdentifier != usPacketIdentifier ) )
    {
#ifdef DB_AWS_IOT  
        // LogError( ( "MQTT_ProcessLoop failed to receive ACK packet: Expected ACK Packet ID=%02"PRIx16", LoopDuration=%"PRIu32", Status=%s",
        //             usPacketIdentifier,
        //             ( ulCurrentTime - ulMqttProcessLoopEntryTime ),
        //             MQTT_Status_strerror( eMqttStatus ) ) );
#endif
    }
    else
    {
        returnStatus = EXIT_SUCCESS;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/


static MQTTStatus_t processLoopWithTimeout( MQTTContext_t * pMqttContext,
                                            uint32_t ulTimeoutMs )
{
    uint32_t ulMqttProcessLoopTimeoutTime;
    uint32_t ulCurrentTime;

    MQTTStatus_t eMqttStatus = MQTTSuccess;

    ulCurrentTime = pMqttContext->getTime();
    ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeoutMs;

    /* Call MQTT_ProcessLoop multiple times a timeout happens, or
     * MQTT_ProcessLoop fails. */
    while( ( ulCurrentTime < ulMqttProcessLoopTimeoutTime ) &&
           ( eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes ) )
    {
        
        // printf("MQTT_ProcessLoop\r\n");
        if(send_mqtt_message){
            for(int i = 0 ; i < mqtt_response_counter ; i++){
                publishToTopic(pMqttContext , MQTT_RESPONSE_TOPIC , mqtt_response_list[i]);
            }
            mqtt_response_counter = 0;
            send_mqtt_message = false;
        }
        eMqttStatus = MQTT_ProcessLoop( pMqttContext );
        ulCurrentTime = pMqttContext->getTime();
        
    }

    return eMqttStatus;
}

// void close_tcp_connection(){
//     /* End TLS session, then close TCP connection. */
//     cleanupESPSecureMgrCerts( &xNetworkContext );
//     ( void ) xTlsDisconnect( &xNetworkContext );
// }

int handleResubscribe( MQTTContext_t * pMqttContext )
{

    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryParams;
    uint16_t nextRetryBackOff = 0U;

    assert( pMqttContext != NULL );

    /* Initialize retry attempts and interval. */
    BackoffAlgorithm_InitializeParams( &retryParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    do
    {
        /* Send SUBSCRIBE packet.
         * Note: reusing the value specified in globalSubscribePacketIdentifier is acceptable here
         * because this function is entered only after the receipt of a SUBACK, at which point
         * its associated packet id is free to use. */
        mqttStatus = MQTT_Subscribe( pMqttContext,
                                     pGlobalSubscriptionList,
                                     sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                     globalSubscribePacketIdentifier );

        if( mqttStatus != MQTTSuccess )
        {
#ifdef DB_AWS_IOT              
            LogError( ( "Failed to send RESUBSCRIBE packet to broker with error = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
#endif
            returnStatus = EXIT_FAILURE;
            break;
        }
#ifdef DB_AWS_IOT  
        LogInfo( ( "RESUBSCRIBE sent for topic %.*s to broker.\n\n",
                   MQTT_COMMAND_TOPIC_LENGTH,
                   MQTT_COMMAND_TOPIC ) );
#endif
        /* Process incoming packet. */
        returnStatus = waitForPacketAck( pMqttContext,
                                         globalSubscribePacketIdentifier,
                                         MQTT_PROCESS_LOOP_TIMEOUT_MS );

        if( returnStatus == EXIT_FAILURE )
        {
            break;
        }

        /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
         * in eventCallback to reflect the status of the SUBACK sent by the broker. It represents
         * either the QoS level granted by the server upon subscription, or acknowledgement of
         * server rejection of the subscription request. */
        if( globalSubAckStatus == MQTTSubAckFailure )
        {
            /* Generate a random number and get back-off value (in milliseconds) for the next re-subscribe attempt. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &retryParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
#ifdef DB_AWS_IOT                  
                LogError( ( "Subscription to topic failed, all attempts exhausted." ) );
#endif
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
#ifdef DB_AWS_IOT                  
                LogWarn( ( "Server rejected subscription request. Retrying "
                           "connection after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff ) );
#endif
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( globalSubAckStatus == MQTTSubAckFailure ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

void aws_iot_task(void *pvParameters){
    
    for( ; ; ){
        if(!isConnectedToWifi){

            printf("Wifi is not connected!\r\n");
            sleep( 2 );   
            continue;
        } 
        sleep( 2 );
        MQTTStatus_t status = MQTTSuccess;
        MQTTContext_t mqttContext = { 0 };
        NetworkContext_t xNetworkContext = { 0 };
        int returnStatus = EXIT_SUCCESS;
        bool clientSessionPresent = false;
        bool brokerSessionPresent = false;
        is_connected_to_server = false;
        send_mqtt_message = false;
        mqtt_response_counter = 0; 

        globalAckPacketIdentifier = 0U;
        globalSubscribePacketIdentifier = 0U;
        globalUnsubscribePacketIdentifier = 0U;

        memset(&outgoingPublishPackets, 0, sizeof(outgoingPublishPackets));
        memset(&pGlobalSubscriptionList, 0, sizeof(pGlobalSubscriptionList));
        memset(&buffer, 0, sizeof(buffer));
        memset(&globalSubAckStatus, 0, sizeof(globalSubAckStatus));
        memset(&pOutgoingPublishRecords, 0, sizeof(pOutgoingPublishRecords));
        memset(&pIncomingPublishRecords, 0, sizeof(pIncomingPublishRecords));
        memset(&xTlsContextSemaphoreBuffer, 0, sizeof(xTlsContextSemaphoreBuffer));

        memset(&mqttContext, 0, sizeof(mqttContext));
        memset(&xNetworkContext, 0, sizeof(xNetworkContext));

        returnStatus = initializeMqtt( &mqttContext, &xNetworkContext );
        
        // disconnectMqttSession(&mqttContext);
        // unsubscribeFromTopic(&mqttContext);
        if( returnStatus == EXIT_SUCCESS )
        {
            for( ; ; )
            {
                returnStatus = connectToServerWithBackoffRetries( &xNetworkContext, &mqttContext, &clientSessionPresent, &brokerSessionPresent );

                if( returnStatus == EXIT_FAILURE )
                {
                    /* Log error to indicate connection failure after all
                    * reconnect attempts are over. */
#ifdef DB_AWS_IOT                     
                    LogError( ( "Failed to connect to MQTT broker %.*s.",
                                AWS_IOT_ENDPOINT_LENGTH,
                                AWS_IOT_ENDPOINT ) );
#endif
                    sleep( MQTT_SUBPUB_LOOP_DELAY_SECONDS );            
                }
                else
                {

                    clientSessionPresent = true;
                    if( brokerSessionPresent == true )
                    {
#ifdef DB_AWS_IOT                          
                        LogInfo( ( "An MQTT session with broker is re-established. "
                                "Resending unacked publishes." ) );
#endif
                        /* Handle all the resend of publish messages. */
                        returnStatus = handlePublishResend( &mqttContext );
                    }
                    else
                    {
#ifdef DB_AWS_IOT                          
                        LogInfo( ( "A clean MQTT connection is established."
                                " Cleaning up all the stored outgoing publishes.\n\n" ) );
#endif
                        /* Clean up the outgoing publishes waiting for ack as this new
                        * connection doesn't re-establish an existing session. */
                        cleanupOutgoingPublishes();
                    }
                    is_connected_to_server = true;
                    break;

                }

            }

            if( returnStatus == EXIT_SUCCESS ){
#ifdef DB_AWS_IOT                  
                LogInfo( ( "Subscribing to the MQTT topic %.*s.",
                        MQTT_COMMAND_TOPIC_LENGTH,
                        MQTT_COMMAND_TOPIC ) );
#endif
                returnStatus = subscribeToTopic( &mqttContext );
            }

            if( returnStatus == EXIT_SUCCESS ){
                returnStatus = waitForPacketAck( &mqttContext,
                                                globalSubscribePacketIdentifier,
                                                MQTT_PROCESS_LOOP_TIMEOUT_MS );
            }

            if( ( returnStatus == EXIT_SUCCESS ) && ( globalSubAckStatus == MQTTSubAckFailure ) )
            {
#ifdef DB_AWS_IOT                  
                LogInfo( ( "Server rejected initial subscription request. Attempting to re-subscribe to topic %.*s.",
                        MQTT_COMMAND_TOPIC_LENGTH,
                        MQTT_COMMAND_TOPIC ) );
#endif                        
                returnStatus = handleResubscribe( &mqttContext );
            }

            if(returnStatus == EXIT_SUCCESS){

                for( ; ; ){

                    status = processLoopWithTimeout( &mqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );  
                    
                    if(status == MQTTRecvFailed){
                        sleep( MQTT_SUBPUB_LOOP_DELAY_SECONDS );
                        break;
                    } 
                    
                }

            }

        }
        xTlsDisconnect ( &xNetworkContext );
    }
    
	// vTaskDelay(1000 / portTICK_PERIOD_MS);
	// xTaskCreate(aws_iot_task, "aws_iot_task", UDP_SERVER_TASK_STACK_DEPTH, (void*)AF_INET, 5, NULL);

    vTaskDelete(NULL);



}