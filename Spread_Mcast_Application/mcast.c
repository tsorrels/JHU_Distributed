#include "sp.h"
#include "mcast.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100

int debug = 0;
int num_procs;
int proc_index;
int num_messages;
int message_index = -1;
struct timeval diff,end,start;
FILE *fd;

window_entry * global_window;

message send_message_buffer;


/* from class_user.c example code */
static	char	User[80];
static  char    Spread_name[80];
static  char	group[80];

static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;

static  int     To_exit = 0;


static  void    initialize(int argc, char *argv[]);

static  void	Bye();
static	void	Read_message();


int newRandomNumber(){
    return rand() % MAX_RAND + 1;
}

struct timeval diffTime(struct timeval left, struct timeval right)
{
    struct timeval diff;

    diff.tv_sec  = left.tv_sec - right.tv_sec;
    diff.tv_usec = left.tv_usec - right.tv_usec;

    if (diff.tv_usec < 0) {
        diff.tv_usec += 1000000;
        diff.tv_sec--;
    }

    if (diff.tv_sec < 0) {
        printf("WARNING: diffTime has negative result, returning 0!\n");
        diff.tv_sec = diff.tv_usec = 0;
    }

    return diff;
}



int main(int argc, char ** argv){
    int	ret;
    sp_time test_timeout;
    
    test_timeout.sec = 5;
    test_timeout.usec = 0;

    /* connect to spread */
    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, 
			      test_timeout );
    if( ret != ACCEPT_SESSION ) 
    {
	SP_error( ret );
	Bye();
    }
    // If spread does not work then no need to initialize.
    initialize(argc, argv);
    printf("User: connected to %s with private group %s\n", Spread_name, 
	   Private_group );

    E_init();

    // Join group "abrahmb1tsorrel3"
    sprintf( group, "abrahmb1tsorrel3" );
    ret = SP_join( Mbox, group );
    if( ret < 0 ) SP_error( ret );

    E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );

    E_handle_events();

    return 0;
}

void initialize(int argc, char ** argv){
    int i;

    if (argc < 4){
	printf("Usage: mcast <num_of_messages> <process_index> "); 
	printf("<num_of_processes> [debug]\n");
	exit(0);
    }    

    if (argc == 5){
	if (atoi(argv[4]) == 1){
	    debug = 1;
	}
    }

    srand (time(NULL));

    proc_index = atoi(argv[2]);
    num_procs = atoi(argv[3]);
    num_messages = atoi(argv[1]);
    sprintf( User, "%s.out", argv[2] );
    sprintf( Spread_name, "4803");

    fd = fopen(User, "w"); 

    global_window = malloc (sizeof(window_entry) * num_procs);
    
    for (i = 0 ; i < num_procs ; i ++){
	global_window[i].proc_id = i + 1;
	global_window[i].FIN = 0;
	global_window[i].seq_num = -1;
    }

}

static  void	Bye()
{
    To_exit = 1;

    gettimeofday(&end, NULL);
    diff = diffTime(end,start);
    printf("Total time taken for transfer = %lf seconds\n",
	 (diff.tv_sec+(diff.tv_usec)/1000000.0));

    SP_disconnect( Mbox );

exit( 0 );
}

void sendNewMessage(int FIN){
    int ret;

    message_index ++;

    send_message_buffer.header.proc_num = proc_index;
    send_message_buffer.header.seq_num = message_index;
    send_message_buffer.header.rand_num = newRandomNumber();
    send_message_buffer.header.FIN = FIN;
    //multicast the message to a single group.
    ret= SP_multicast( Mbox, AGREED_MESS,(const char *)group, 1, 
                    sizeof(message), (const char *)(&send_message_buffer) );
    if( ret < 0 ) 
    {
	SP_error( ret );
	Bye();
    }
   
}


void sendMessages(){
    int i;
    int min_seq_num = message_index;
    int num_messages_to_send = 0, num_fin = 0;

    /* Get the minimum sequence number sent by a process
     * that has not finished sending. Also check how many
     * processes have finished sending. */
    for (i = 0 ; i < num_procs ; i ++){
	if (global_window[i].FIN != 1 && 
	    global_window[i].seq_num < min_seq_num){
	    min_seq_num = global_window[i].seq_num;
	}
        if(global_window[i].FIN == 1)
	    num_fin ++;
    }

    if(global_window[proc_index - 1].FIN){
        /* If this process has finished sending 
         * and all other processes have finished sending,
         * then exit. */
	if(num_fin == num_procs){
	    fflush(fd);
	    Bye();
	}
        /* If there is at least one process that is still
         * not done, then just return. */
	return;
    }
    /* If this process has send all its packets then
     * send the FIN packet. */
    if(message_index == (num_messages - 1)){
	sendNewMessage(1);
        return;
    }	

    num_messages_to_send = min_seq_num + NUM_SEND_MESSAGE - message_index;

    /* If this process can send messages then send upto num_message_to_send
     * messages. */
    for (i = 0 ; i < num_messages_to_send && 
        message_index < (num_messages - 1); i ++){
	sendNewMessage(0); /* 0 means do not send FIN */
    }      
}

/* Process the received packet. */
void deliverMessage(char *mess)
{
    message *packet;
    message_header *header;
    packet = (message *)mess;
    header = &packet->header;
    /* If the received packet is a FIN packet, then
     * update the global_window. */
    if(header->FIN){
	global_window[header->proc_num - 1].FIN = 1;
	return;
    }
    /* If the packet is normal packet, then update the
     * global window and write to the file. */
    global_window[header->proc_num - 1].seq_num = header->seq_num;
    fprintf(fd, "%2d, %8d, %8d\n", header->proc_num, header->seq_num,
	    header->rand_num);
}

static	void	Read_message()
{

    static char		 mess[MAX_MESSLEN];
    char		 sender[MAX_GROUP_NAME];
    char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    membership_info  memb_info;
    vs_set_info      vssets[MAX_VSSETS];
    unsigned int     my_vsset_index;
    int              num_vs_sets;
    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
    int		 num_groups;
    int		 service_type;
    int16		 mess_type;
    int		 endian_mismatch;
    int		 i,j;
    int		 ret;

    service_type = 0;

    ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, 
		      target_groups, &mess_type, &endian_mismatch, 
		      sizeof(mess), mess );
    if( ret < 0 ) 
    {
	if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
	    service_type = DROP_RECV;
	    printf("\n========Buffers or Groups too Short=======\n");
	    ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, 
			      &num_groups, target_groups, &mess_type, 
			      &endian_mismatch, sizeof(mess), mess );
	}
    }
    if (ret < 0 )
    {
	if( ! To_exit )
	{
	    SP_error( ret );
	    printf("\n============================\n");
	    printf("\nBye.\n");
	}
	exit( 0 );
    }

    if( Is_regular_mess( service_type ) )
    {
	mess[ret] = 0;
        // Process the received message and send new messages accordingly.
        deliverMessage(mess);
        sendMessages();
    }else if( Is_membership_mess( service_type ) )
    {
	ret = SP_get_memb_info( mess, service_type, &memb_info );
	if (ret < 0) {
	    printf("BUG: membership message does not have valid body\n");
	    SP_error( ret );
	    exit( 1 );
	}
	if     ( Is_reg_memb_mess( service_type ) )
	{
	    // num_groups loaded with number of members in group

	    printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n", sender, num_groups, mess_type );
	    for( i=0; i < num_groups; i++ )
		printf("\t%s\n", &target_groups[i][0] );
	    printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

	    if( Is_caused_join_mess( service_type ) )
	    {
		printf("Due to the JOIN of %s\n", memb_info.changed_member );
	    }else if( Is_caused_leave_mess( service_type ) ){
		printf("Due to the LEAVE of %s\n", memb_info.changed_member );
	    }else if( Is_caused_disconnect_mess( service_type ) ){
		printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
	    }else if( Is_caused_network_mess( service_type ) ){
		printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
		num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
		if (num_vs_sets < 0) {
		    printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
		    SP_error( num_vs_sets );
		    exit( 1 );
		}
		for( i = 0; i < num_vs_sets; i++ )
		{
		    printf("%s VS set %d has %u members:\n",
			   (i  == my_vsset_index) ?
			   ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
		    ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
		    if (ret < 0) {
			printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
			SP_error( ret );
			exit( 1 );
		    }
		    for( j = 0; j < vssets[i].num_members; j++ )
			printf("\t%s\n", members[j] );
		}
	    }
	}else if( Is_transition_mess(   service_type ) ) {
	    printf("received TRANSITIONAL membership for group %s\n", sender );
	}else if( Is_caused_leave_mess( service_type ) ){
	    printf("received membership message that left group %s\n", sender );
	}else printf("received incorrecty membership message of type 0x%x\n", service_type );

	/********************* CHECK MEMBERSHIP *************************/
	if (num_groups == num_procs){
	    /* BEGIN EXECUTION */
            printf("Starting Transfer\n");
            gettimeofday(&start, NULL);
            sendMessages();
	}


    } else if ( Is_reject_mess( service_type ) )
    {
	printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
	       sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
    }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
}
