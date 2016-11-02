#define PAYLOAD_SIZE 1200


#define NUM_SEND_MESSAGE 25
#define MAX_RAND 1000000




typedef struct message_header_type {
    int proc_num;
    int seq_num;
    int rand_num;
    int FIN;
} message_header;


typedef struct window_entry_type{
    int proc_id;
    int FIN;
    int seq_num;
} window_entry;

typedef struct message_type{
    message_header header;
    char data[PAYLOAD_SIZE];
} message;


