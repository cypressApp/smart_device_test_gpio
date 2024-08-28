
#ifndef ACCOUNT_STRUCT
#define ACCOUNT_STRUCT

typedef struct account_struct {
  int  sock;
  int  ip4; 
} account_struct;

extern account_struct account_struct_list[100];

#endif

extern int   account_list_size;
extern int   response_id;
extern char  mqtt_response_list[1024][32];
extern int   mqtt_response_counter; 
extern bool  send_mqtt_message;

void update_response_id();
void remove_account(int sock);
void add_account(int sock , int temp_ip4);
void send_data_to_clients(int sock , char *data , int data_size);