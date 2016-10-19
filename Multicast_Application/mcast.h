#include "net_include.h"
#include "recv_dbg.h"

#define WINDOW_SIZE 10000
#define PAYLOAD_SIZE (1400-sizeof(packet_header))
#define MAX_NAK 330
#define SEND_WINDOW_SIZE 500
#define MAX_MESSAGE 160
#define MAX_RAND 1000000
#define MAX_PROC 10     // Assuming that there will be max 10 processes for this assignment
#define NUM_TOKEN_RESEND 20

//#define MCAST_ADDR 225 << 24 | 0 << 16 | 1 << 8 | 1
#define MCAST_ADDR  225 << 24 | 1 << 16 | 2 << 8 | 120 /* (225.1.2.120) */ 

#define TOKEN_RESEND_TIMER 100000 /* in us */
#define AWAITING_TOKEN_TIMER 1000000 /* in us */

// Different types of packets that will be sent over the network
typedef enum {
    DATA,
    TOKEN,
    START,
    FIN
} packet_type;

// Different types of states that our process can be in
typedef enum {
    WAITING_START,
    TOKEN_SENT,
    AWAITING_TOKEN,
    FINISHED /* indicates process has sent all its messages */
} process_state_type;

// This is to store the socket and send_addr
typedef struct connection_type{
  int fd;
  struct sockaddr_in send_addr;
  
} connection;

// The header of the packet that will be sent on the network
typedef struct packet_header_type {
    int seq_num;
    int rand_num;
    int proc_num;
    packet_type type;
} packet_header;

// The actual packet that will be sent on the network
typedef struct packet_type{
    packet_header header;
    char data [PAYLOAD_SIZE];
} packet;

// This is the token that will be stored as a header inside the token packet
typedef struct token_payload_type {
    int address;
    int seq_num;
    int ack;
    int num_nak;
    int nak[MAX_NAK];
    int num_shutdown;
    int host_num[MAX_PROC]; // This stores the address of each process at their corresponding index
} token_payload;

/* This is to encapsulate whether the packet that we have was received or not.
 * If not, then we can overwrite the packet */
typedef struct packet_buffer_type{
    int received;
    packet packet_bytes;
} packet_buffer;

/* This includes complete information of the sender buffer
 * which stores the packets that are to be send over the network,
 * the start and end index of the buffer,
 * total number of packets built so far,
 * total number of packets sent so far,
 * the token that was sent by this process in the previous round */
typedef struct sender_window_type {
    int window_start; /* index within packets */
    int window_end; /* index within packets */
    packet packets[SEND_WINDOW_SIZE];
    int num_built_packets;
    int num_sent_packets;
    packet previous_token; /* stored token in case it is lost */
} sender_window;

/* This includes the complete information of the global buffer
 * which stores the packets sent by all the process(including itself),
 * the start and end sequence number of packets that it is expecting,
 * the ack value that was sent by this process in the previous round,
 * a has_address flag to specify whether this process has the address of the next process or not,
 * sendto_ip stores the actual address of the next process(this will be used for unicasting the token)
 * my_ip stores the address of this process(this will be included in every token it sends)
 * fd stores the file pointer of the file that will be written */
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
