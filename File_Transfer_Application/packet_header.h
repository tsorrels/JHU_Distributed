
#include "net_include.h"

#define WINDOW_SIZE 100
/* #define PAYLOAD_SIZE 1390 */
#define PAYLOAD_SIZE (1400-sizeof(packet_header))

#define RECV_NUM_RETRY_ACK 5
#define NUM_INTERMITENT_ACK 4


typedef enum {
    SYN,
    WAIT,
    GO,
    FIN,
    FINACK,
    DATA,
    ACK
} packet_type;


typedef enum {
    IDLE,
    WAITING_DATA, /* waiting for first data packet */
    RECV_DATA /* waiting for subsequent data packets */
} receiver_state_type;




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
    int length;
    packet packet_bytes;
} packet_buffer;


typedef struct ack_payload_type{
    int ack;
    int num_nak;
    int * naks;
} ack_payload;


typedef struct connection_type{
    struct sockaddr_in socket_address;
    int status; /* syn sent, sending data, finish, whatever */

} connection;




/* receiver timer values in microseconds*/
const uint recv_ack_timer = 5000;
const uint recv_go_timer = 5000;
const uint recv_data_timer = 5000;
const uint recv_ack_timer = 5000;

/* sender timer values in microseconds*/
const uint sender_fin_timeout = 5000;
const uint sender_syn_timer = 5000;
const uint sender_wait_timer = 5000;
const uint sender_data_timer = 20000;



