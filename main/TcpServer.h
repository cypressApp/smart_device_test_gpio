#include "stdbool.h"

#undef  TCP_SERVER_DB

#undef  CONSTANT_TCP_RECEIVE_LEN 
#define TCP_RECEIVE_DATA_LENGTH        10240 //49152 
#define TCP_SERVER_TASK_STACK_DEPTH    20480//70000 //20480
#define UDP_SERVER_TASK_STACK_DEPTH    20480
#define AWS_IOT_STACK_DEPTH            20480
#define TCP_RECEIVE_DATA_SUFFIX        "\n"
#define TCP_RECEIVE_DATA_SUFFIX_LENGTH  1
#define SEND_TO_ALL                    -2

extern int  tcp_rec_data_counter;
extern int  temp_sock;
extern int  tcp_timeout_counter;
extern int  tcp_rec_data_counter;
extern bool is_tcp_timeout;
extern bool valid_data_received;

void execute_tcp_send(char *data);
void process_tcp_data(char* rx_buffer , int rx_buffer_len , int sock);
void check_tcp_recv_timeout_task();
void tcp_server_task(void *pvParameters);
