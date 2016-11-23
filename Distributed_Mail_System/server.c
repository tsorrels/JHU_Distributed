#include "sp.h"
#include "mail.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>


state local_state;
int debug;

/* for spread */
static	char	User[80];
static  char    Spread_name[80];
static  char	group[80];
static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;
static  int     To_exit = 0;



void initialize();
void init();
void Bye();
static	void	readSpreadMessage();

int main (int argc, char ** argv)
{
    int	ret;
    sp_time test_timeout;

    
    test_timeout.sec = TEST_TIMEOUT_SEC;
    test_timeout.usec = TEST_TIMEOUT_USEC;

    
    initialize(argc, argv);
    init(); // TODO: this function doesn't do anything


    /* connect to spread */
    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, 
			      local_state.private_group, test_timeout );
    if( ret != ACCEPT_SESSION ) 
    {
	SP_error( ret );
	Bye();
    }
    
    loadState(); // define in recover.c


    printf("User: connected to %s with private group %s\n", Spread_name, 
	   Private_group );

    E_init();


    // Join group "abrahmb1tsorrel3servers"
    sprintf( group, SERVER_GROUP_NAME);
    ret = SP_join( Mbox, group );
    if( ret < 0 ) SP_error( ret );

    E_attach_fd( Mbox, READ_FD, readSpreadMessage, 0, NULL, HIGH_PRIORITY );

    E_handle_events();

    return 0;
}

/* generates an update message to send to other servers 
 * returns a pointer to a full message */
message * generateUpdate(char * mess){
    command * commandPtr;
    message * messagePtr;
    message * updateMessage;
    update * updatePtr;

    messagePtr = (message *) mess;
    commandPtr = (command *) messagePtr->payload;
    updateMessage = malloc(sizeof(message));
    updateMessage->header.type = UPDATE;

    /* load message update with message type, procID, and user_name */
    updatePtr = (update *) updateMessage->payload;
    updatePtr->procID = local_state.proc_ID;
    memcpy(updatePtr->user_name, commandPtr->user_name, MAX_USER_LENGTH);


    if (commandPtr->type == NEWUSERCMD){
	updatePtr->type = NEWUSERMSG;
    }

    else if (commandPtr->type == DELETEMAILCMD){
	updatePtr->type = DELETEMAILMSG;
	updatePtr->mailID = commandPtr->mailID;
    }

    else if (commandPtr->type == READMAILCMD){
	updatePtr->type = READMAILMSG;
	updatePtr->mailID = commandPtr->mailID;
	memcpy(updatePtr->payload, commandPtr->payload, UPDATE_PAYLOAD_SIZE);
    }

    else if (commandPtr->type == NEWMAILCMD){
	updatePtr->type = NEWMAILMSG;
	updatePtr->mailID = commandPtr->mailID;
	memcpy(updatePtr->payload, commandPtr->payload, UPDATE_PAYLOAD_SIZE);
    }

    else{
	perror("ERROR: did not recognize command type in gnerateUpdate");	
    }
    
    if (debug)
	printf("generated update type %d\n", updatePtr->type);

    return updateMessage;
}

/* sends update to spread server_group */
void sendUpdate(message * updateMessage){
    int ret;

    if (debug)
	printf("sending update\n");

    ret= SP_multicast(Mbox, AGREED_MESS,(const char *)local_state.server_group,
		      1, sizeof(message), (const char *)(updateMessage) );
    if( ret < 0 ) 
    {
	perror("ERROR: failed to send update in sendUpdate");
	SP_error( ret );
	Bye();
    }
}


void markAsRead(){
    if (debug)
	printf("Marking message as read\n");


}

void addMail(){
    if (debug)
	printf("Adding mail to state\n");


}

void deleteMail(){
    if (debug)
	printf("Deleting mail\n");


}


/* scan state for first invalid user entry and add user */
void addUser(){
    int i;
    int index;

    index = -1;

    if (debug)
	printf("Adding user\n");

    for (i = 0 ; i < MAX_USERS ; i ++){
	if (local_state.users[i].valid == 0){
	    index = i;
	}
    }

    if (index != -1){
	//sprintf(, "Server%s", argv[1] );
 	local_state.users[index].valid = 1;
	
    }

}


void applyUpdate(char * mess){
    message * messagePtr;
    update * updatePtr;

    messagePtr = (message *) mess;
    updatePtr = (update *) messagePtr->payload;
    
    if (updatePtr->type == NEWMAILMSG){
	addMail();
    }

    else if (updatePtr->type == NEWUSERMSG){
	addUser();
    }

    else if (updatePtr->type == DELETEMAILMSG){
	deleteMail();
    }

    else if (updatePtr->type == READMAILMSG){
	markAsRead();
    }
}



//processRegularMessage(sender, num_groups, target_groups, mess_type, mess);
/* should this function return anything? */
void processRegularMessage(char * sender, int num_groups, 
			   char groups[][MAX_GROUP_NAME], 
			   int16 mess_type, char * mess){

    int createUpdate = 0;
    message * updateMessage;

    message * messagePtr;
    messagePtr= (message *) mess;
    if (messagePtr->header.type == COMMAND){
	// execute client stuff
	// set createUpdate

	
	// generate update
	if (createUpdate){
	    updateMessage = generateUpdate(mess); // mallocs a message
	    sendUpdate(updateMessage);
	    free(updateMessage); // frees update message		
	}

    }

    else if (messagePtr->header.type == UPDATE){
	applyUpdate(mess);

    }


    else if (messagePtr->header.type == MATRIX){
	// execute

	// udpate local matrix

    }

    else{
	perror("ERROR: Did not recognize type in processRegularMessage");
    }
    

    /* if this is from a client */

    




    /* if this is from a server */



}


/* should this function return anything? */
void processMembershipMessage(char * mess){

    


}





static	void	readSpreadMessage()
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
	processRegularMessage(sender, num_groups, target_groups, 
			      mess_type, mess);
	//mess[ret] = 0;
        // Process the received message and send new messages accordingly.
        //deliverMessage(mess);
        //sendMessages();
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
	//if (num_groups == num_procs){
	    /* BEGIN EXECUTION */
	//  printf("Starting Transfer\n");
        //    gettimeofday(&start, NULL);
        //    sendMessages();
	//}


    } else if ( Is_reject_mess( service_type ) )
    {
	printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
	       sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
    }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
}





void initialize(int argc, char ** argv){
    debug = 0;

    if (argc < 2){
	printf("Usage: server <server_index> [debug]\n"); 
	exit(0);
    }    

    if (argc == 3){
	if (atoi(argv[2]) == 1){
	    debug = 1;
	}
    }

    local_state.proc_ID = atoi(argv[1]);
    sprintf(local_state.server_group, "%s", SERVER_GROUP_NAME);

    sprintf( User, "Server%s", argv[1] );
    sprintf( Spread_name, "4803"); // TODO: pull this out to config file

}


void init(){


}


void Bye(){
    printf("Exiting\n");
    exit(0);


}
