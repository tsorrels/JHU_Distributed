

#define WINDOW_SIZE 10
#define PAYLOAD_SIZE 1390

typedef struct packet_header_type {
    int seq_num;
    int type;
    int size;
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
    char * ipAddress;
    int port;
    int status; /* syn sent, sending data, finish, whatever */

} connection;


/* receiver timer values in nanoseconds*/
const uint recv_ack_timer = 5000;



/* snder timer values in nanoseconds*/
const uint sender_fin_timeout = 5000;

