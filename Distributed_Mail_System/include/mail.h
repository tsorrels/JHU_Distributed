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
#define MAX_COMMAND_LENGTH 30
#define NUM_SERVERS 5
//#define PORT 18537
#define PORT 10470

#define TEST_TIMEOUT_SEC 5
#define TEST_TIMEOUT_USEC 0


#define SERVER_GROUP_NAME "abrahmb1tsorrel3servers"

/* for spread */
#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100




/* System wide message types */
typedef enum {
    RECONCILE,
    PARTITIONED,
    NORMAL,
} server_status_type;


/* System wide message types */
typedef enum {
    COMMAND,
    UPDATE,
    MATRIX,
    REPLY,
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

/* Structure for update */
typedef struct update_type{
    update_type type;
    int procID;
    int updateIndex;
    char user_name[MAX_USER_LENGTH];
    mail_id mailID;
    char payload[UPDATE_PAYLOAD_SIZE];
    struct update_type *next;
} update;


typedef struct update_vector_type{
    int size;
    int capacity;
    update * updates;

} update_vector;


/* Structure to cache updates from all processes */
typedef struct update_buffer_type{
    update_vector procVectors [NUM_SERVERS];
    update proc1[MAX_UPDATE];
    update proc2[MAX_UPDATE];
    update proc3[MAX_UPDATE];
    update proc4[MAX_UPDATE];
    update proc5[MAX_UPDATE];
} update_buffer;

/* Structure to maintain latest update index from all processes */
typedef struct update_matrix_type{
    /* first dimmension is server the vector is from, second is vector */
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
    char message[MAX_MESSAGE_SIZE];
    struct email_type * next;
} email;

/* Structure for command */
typedef struct command_type{
    command_type type;
    int ret;
    /*char user_name[MAX_USER_LENGTH];
    char send_to[MAX_USER_LENGTH];
    char subject[MAX_SUBJECT_LENGTH];
    mail_id mailID;*/
    char payload[UPDATE_PAYLOAD_SIZE];
    //email mail;
    char private_group[SIZE_PRIVATE_GROUP];
} command;

/* For vector library */
typedef struct email_vector_type{
    int size;
    int capacity;
    email * emails;

} email_vector;





/* user data type*/
typedef struct user_type{
    char name [MAX_USER_LENGTH];
    //email emails[MAX_EMAILS];
    email_vector emails;
    struct user_type * next;
} user;


/* Entry in state for user type */
typedef struct user_entry_type{
    user userData;
    int valid;
} user_entry;

typedef struct user_vector_type{
	user * user_head;
} user_vector;

/* State local to each server */
typedef struct state_type{
    int proc_ID; /* this server's ID */
    server_status_type status; 
    char server_group[MAX_GROUP_NAME]; /* spread group name for server */
    char private_group[MAX_GROUP_NAME]; /* server's private group */
    int updateIndex; /* counter of updates received, for LTS */
    //user_entry users[MAX_USERS];
    user_vector users;
    update_matrix local_update_matrix;
    update_buffer local_update_buffer;
    int recoveryFD;
    connection connections[MAX_CONNECTIONS];
} state;




/* function declarations */
extern void loadState(state * local_state);

//extern int email_vector_init(email_vector * vector);
extern int email_vector_insert(email_vector * vector, email * emailPtr);
extern int email_vector_delete(email_vector * vector, mail_id target);
extern email * email_vector_get(email_vector * vector, mail_id target);

//extern int update_vector_init(update_vector * vector);
extern int update_vector_insert(update_vector * vector, update * updatePtr);
extern int update_vector_delete(update_vector * vector, mail_id target);
extern update * update_vector_get(update_vector * vector, int updateIndex);

extern int user_vector_insert(user_vector * vector, char * name);
extern int user_vector_delete(user_vector * vector, char * name);
extern user * user_vector_get(user_vector * vector, char * name);





