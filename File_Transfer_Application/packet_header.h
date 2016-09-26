
#include "net_include.h"

#define WINDOW_SIZE 200
/* #define PAYLOAD_SIZE 1390 */
#define PAYLOAD_SIZE (1400-sizeof(packet_header))

#define RECV_NUM_RETRY_ACK 5
#define NUM_INTERMITENT_ACK 1
#define HUN_MB 104857600


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
    RECV_DATA, /* waiting for subsequent data packets */
    WAIT_RESPONSE /* wait for data following ack/nak */
} receiver_state_type;




typedef struct packet_header_type {
    int seq_num;
    packet_type type;
} packet_header;

typedef struct ack_packet_type{
    packet_header header;
    int ack;
    int num_nak;
    int naks [WINDOW_SIZE];
} ack_packet;
  



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

/*
typedef struct ack_payload_type{
    int ack;
    int num_nak;
    int naks [WINDOW_SIZE];
} ack_payload;
*/


typedef struct connection_type{
    struct sockaddr_in socket_address;
    int status; /* syn sent, sending data, finish, whatever */
    struct timeval startTime;
} connection;




/* receiver timer values in microseconds*/
const uint recv_window_timer = 200000;
const uint recv_go_timer = 500000;
const uint recv_data_timer = 100000;

/* sender timer values in microseconds*/
const uint sender_fin_timeout = 100000;
const uint sender_syn_timer = 100000;
const uint sender_wait_timer = 500000;
const uint sender_data_timer = 5000000;



