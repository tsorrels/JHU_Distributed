#define PAYLOAD_SIZE 1200


#define NUM_SEND_MESSAGE 25
#define MAX_RAND 1000000

/* Data structure to hold the header of a packet*/
typedef struct message_header_type {
    int proc_num;
    int seq_num;
    int rand_num;
    int FIN;  // this will inform if the sender is done sending.
} message_header;

/* Data structure to hold the information about all processes*/
typedef struct window_entry_type{
    int proc_id;
    int FIN;
    int seq_num;
} window_entry;

/* Data structure to store the actual packet. */
typedef struct message_type{
    message_header header;
    char data[PAYLOAD_SIZE];
} message;


