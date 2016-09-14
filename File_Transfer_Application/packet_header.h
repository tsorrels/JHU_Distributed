

#define WINDOW_SIZE 10
#define PAYLOAD_SIZE 1390


typedef struct packet_header_type {
    int seq_num;
    int type;
    int size;
} packet_header;


typedef struct packet_type{
    packet_header;
    char[PAYLOAD_SIZE];
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




