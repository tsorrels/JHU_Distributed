#include "net_include.h"
#include "recv_dbg.h"

#define WINDOW_SIZE 10000
#define PAYLOAD_SIZE (1400-sizeof(packet_header))
#define MAX_NAK 330
#define SEND_WINDOW_SIZE 500
#define MAX_MESSAGE 30
#define MAX_RAND 1000000
#define MAX_PROC 10
#define NUM_TOKEN_RESEND 20

//#define MCAST_ADDR 225 << 24 | 0 << 16 | 1 << 8 | 1
#define MCAST_ADDR  225 << 24 | 1 << 16 | 2 << 8 | 120 /* (225.1.2.120) */ 

#define TOKEN_RESEND_TIMER 100000 /* in us */
#define AWAITING_TOKEN_TIMER 1000000 /* in us */

typedef enum {
    DATA,
    TOKEN,
    START,
    FIN
} packet_type;

typedef enum {
    WAITING_START,
    TOKEN_SENT,
    AWAITING_TOKEN,
    FINISHED /* indicates process has sent all its messages */
} process_state_type;

typedef struct connection_type{
  int fd;
  struct sockaddr_in send_addr;
  
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
    int num_shutdown;
    int host_num[MAX_PROC];
} token_payload;


typedef struct packet_buffer_type{
    //int seq_num;
    int received;
    packet packet_bytes;
    //int size;
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
    packet_buffer packets[WINDOW_SIZE];
    int previous_ack;
    int has_address;
    int sendto_ip;
    int my_ip;
    FILE *fd;
} global_window;
