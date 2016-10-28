#define PAYLOAD_SIZE 1200

typedef struct message_header_type {
    int proc_num;
    int seq_num;
    int rand_num;
} message_header;


typedef message_type{
    message_header header;
    char data[PAYLOAD_SIZE];
} message;


