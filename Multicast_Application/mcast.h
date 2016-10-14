#include "net_include.h"

#define WINDOW_SIZE 340
#define PAYLOAD_SIZE (1400-sizeof(packet_header))
#define MAX_NAK 340
#define SEND_WINDOW_SIZE 200
#define MAX_MESSAGE 100
#define MAX_RAND 1000000

#define TOKEN_RESEND_TIMER 1000 /* in us */
#define AWAITING_TOKEN_TIMER 1000 /* in us */

typedef enum {
    DATA,
    TOKEN,
    START
} packet_type;

typedef enum {
    WAITING_START,
    TOKEN_SENT,
    AWAITING_TOKEN,
    CLOSING
} process_state_type;

typedef struct connection_type{
  int fd;
  struct sockaddr send_addr;
  
} connection;


typedef struct packet_header_type {
    int seq_num;
    int rand_num;
    int proc_num;
    packet_type type;
} packet_header;


typedef struct packet_type{
    packet_header header;
    char data [PAYLOAD_SIZE];
} packet;


typedef struct token_payload_type {
    int address;
    int seq_num;
    int ack;
    int num_nak;
    int nak[MAX_NAK];
} token_payload;


typedef struct packet_buffer_type{
    int seq_num;
    int received;
    packet packet_bytes;
    int size;
} packet_buffer;


typedef struct sender_window_type {
    int window_start; /* index within packets;' */
    int window_end; /* index within packets;' */
  //packet_buffer packets [SEND_WINDOW_SIZE];
    packet packets[SEND_WINDOW_SIZE];
    int num_built_packets;
    int num_sent_packets;
  packet previous_token; /* stored token in case it is lost */
} sender_window;

typedef struct gloal_window_type {
    int window_start; /* sequence number */
    int window_end; /* sequence number */
    packet_buffer packets [WINDOW_SIZE];
    int previous_ack;
    int has_lowered;
} global_window;
