#define PAYLOAD_SIZE 1200
#define NUM_SEND_MESSAGE 25
#define MAX_RAND 1000000
#define MAX_USERS 100
#define MAX_MESSAGE_SIZE 1000
#define MAX_USER_LENGTH 25
#define MAX_SUBJECT_LENGTH 25
#define MAX_EMAILS 100
#define UPDATE_PAYLOAD_SIZE 1000
#define MESSAGE_PAYLOAD_SIZE 1300
#define MAX_UPDATE 1000
#define SIZE_PRIVATE_GROUP 256 ///// can be set from spread header
#define MAX_CONNECTIONS 50
#define NUM_SERVERS 5

/* System wide message types */
typedef enum {
    COMMAND,
    UPDATE,
    VECTOR,
} message_type;

/* Update message types */
typedef enum {
    NEWMAILMSG,
    NEWUSERMSG,
    DELETEMAILMSG,
    READMAILMSG,
} update_type;

/* Command message types */
typedef enum {
    NEWMAILCMD,
    NEWUSERCMD,
    DELETEMAILCMD,
    READMAILCMD,
    LISTMAILCMD,
    CONNECTCMD,
    SHOWMEMBERSHIPCMD,
} command_type;

/******* DRAFT **********/
typedef struct connection_type {
    char private_group [SIZE_PRIVATE_GROUP];  // spread private group
    char current_user [SIZE_PRIVATE_GROUP];
} connection;

/******* DRAFT **********/
typedef struct connect_payload_type {
    char private_group [SIZE_PRIVATE_GROUP] ;
} connect_payload;


/* Header for all system wide messages, sent through spread */
typedef struct message_header_type {
    message_type type;
    int proc_num;
} message_header;

/* Message type, send to spread API for group multicast */
typedef struct message_type{
    message_header header;
    char payload[MESSAGE_PAYLOAD_SIZE];
} message;


/* Structure for update */
typedef struct update_type{
    update_type type;
    int procID;
    char payload[UPDATE_PAYLOAD_SIZE];
} update;


/* Structure to cache updates from all processes */
typedef struct update_buffer_type{
    update proc1[MAX_UPDATE];
    update proc2[MAX_UPDATE];
    update proc3[MAX_UPDATE];
    update proc4[MAX_UPDATE];
    update proc5[MAX_UPDATE];
} update_buffer;

/* Structure to maintain latest update index from all processes */
typedef struct update_vector_type{
    int latest_update[NUM_SERVERS];
} update_vector;


/* Email data type */
typedef struct email_type{
    int read;
    int procCreated;
    int emailIndex;
    char from[MAX_USER_LENGTH];
    char to[MAX_USER_LENGTH];
    char subject[MAX_SUBJECT_LENGTH];
    int date;
    int valid;
    char MESSAGE[MAX_MESSAGE_SIZE];
} email;




/* user data type*/
typedef struct user_type{
    char name [MAX_USER_LENGTH];
    email emails[MAX_EMAILS];
} user;


/* Entry in state for user type */
typedef struct user_entry_type{
    user userData;
    int valid;
} user_entry;


/* State local to each server */
typedef struct state_type{
    user_entry users[MAX_USERS];
    update_vector local_update_vector;
    update_buffer local_update_buffer;
    connection connections[MAX_CONNECTIONS] ;
} state;
