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
#define PORT 18537

#define TEST_TIMEOUT_SEC 5
#define TEST_TIMEOUT_USEC 0


#define SERVER_GROUP_NAME "abrahmb1tsorrel3servers"

/* for spread */
#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100







/* System wide message types */
typedef enum {
    COMMAND,
    UPDATE,
    MATRIX,
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
    LISTMAILCMD,	// does not modify state
    CONNECTCMD,		// does not modify state
    SHOWMEMBERSHIPCMD,	// does not modify state
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



typedef struct mail_id_type{
    int procID;
    int index; // global counter, can count emails or udpates
} mail_id;


/* Structure for command */
typedef struct command_type{
    command_type type;
    char user_name[MAX_USER_LENGTH];
    mail_id mailID;
    char payload[UPDATE_PAYLOAD_SIZE];
} command;


/* Structure for update */
typedef struct update_type{
    update_type type;
    int procID;
    int updateIndex;
    char user_name[MAX_USER_LENGTH];
    mail_id mailID;
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
typedef struct update_matrix_type{
    int latest_update[NUM_SERVERS][NUM_SERVERS];
} update_matrix;


/* Email data type */
typedef struct email_type{
    int read;
    mail_id mailID;
    //int procCreated;
    //int emailIndex;
    char from[MAX_USER_LENGTH];
    char to[MAX_USER_LENGTH];
    char subject[MAX_SUBJECT_LENGTH];
    int date;
    int valid;
    char MESSAGE[MAX_MESSAGE_SIZE];
} email;




/* For vector library */
typedef struct email_vector_type{
    int size;
    int capacity;
    email * emails;

} email_vector;


typedef struct update_vector_type{
    int size;
    int capacity;
    update * updates;

} update_vector;



/* user data type*/
typedef struct user_type{
    char name [MAX_USER_LENGTH];
    //email emails[MAX_EMAILS];
    email_vector emails;
} user;


/* Entry in state for user type */
typedef struct user_entry_type{
    user userData;
    int valid;
} user_entry;


/* State local to each server */
typedef struct state_type{
    int proc_ID;
    char server_group[MAX_GROUP_NAME];
    char private_group[MAX_GROUP_NAME];
    int updateIndex;
    user_entry users[MAX_USERS];
    update_matrix local_update_matrix;
    update_buffer local_update_buffer;
    connection connections[MAX_CONNECTIONS];
} state;





/* function declarations */
extern void loadState();
extern int email_vector_init(email_vector * vector);
extern int email_vector_insert(email_vector * vector, email * emailPtr);





