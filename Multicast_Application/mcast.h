
#define WINDOW_SIZE 340
#define PAYLOAD_SIZE (1400-sizeof(packet_header))
#define MAX_NAK 340



typedef enum {
    DATA,
    TOKEN
} packet_type;


typedef struct packet_header_type {
    int seq_num;
    int rand_num;
    int proc_num;
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


typedef struct window_type {
    int window_start;
    int window_end;
    packet_buffer packets [WINDOW_SIZE];
} window;
