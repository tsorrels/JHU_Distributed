
#include "net_include.h"

#define WINDOW_SIZE 10
#define PAYLOAD_SIZE 1390

typedef enum {
    SYN,
    WAIT,
    GO,
    FIN,
    FINACK,
    DATA
} packet_type;

typedef struct packet_header_type {
    int seq_num;
    packet_type type;
} packet_header;


typedef struct packet_type{
    packet_header header;
    char data [PAYLOAD_SIZE];
} packet;


typedef struct packet_buffer_type{
    int received;
    int seq_num;
    void * packet;
} packet_buffer;


typedef struct ack_payload_type{
    int ack;
    int num_nak;
    int * num_nacks;
} ack_payload;


typedef struct connection_type{
    struct sockaddr_in socket_address;
    int status; /* syn sent, sending data, finish, whatever */

} connection;


/* receiver timer values in microseconds*/
const uint recv_ack_timer = 5000;


/* sender timer values in microseconds*/
const uint sender_fin_timeout = 5000;



