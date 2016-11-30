#include "sp.h"
#include "mail.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

char userName[MAX_USER_LENGTH];
int serverNum;
int maxMailNum;
char private_group[MAX_GROUP_NAME];
char server_group[MAX_GROUP_NAME];
static  mailbox Mbox;

connectClient(){
    int ret;
    sp_time test_timeout;

    
    test_timeout.sec = TEST_TIMEOUT_SEC;
    test_timeout.usec = TEST_TIMEOUT_USEC;

    sprintf(server_group, "%s", SERVER_GROUP_NAME);

    sprintf( Spread_name, "4803");
    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, 
			      private_group, test_timeout );
    if( ret != ACCEPT_SESSION ) 
    {
	SP_error( ret );
	Bye();
    }
    
    printf("User: connected to %s with private group %s\n", Spread_name, 
	   Private_group );

    E_init();
    
    E_attach_fd( Mbox, READ_FD, readSpreadMessage, 0, NULL, HIGH_PRIORITY );

    E_handle_events();
    
}

int sendCommand(command *newCommand){
    int ret;

    ret= SP_multicast(Mbox, AGREED_MESS,(const char *)server_group,
		      1, sizeof(command), (const char *)(newCommand) );

    if( ret < 0 ) 
    {
	perror("ERROR: failed to send update in sendUpdate");
	SP_error( ret );
    }
    return ret;
}

processRegularMessage(char *mess){
    
}

static void readSpreadMessage(){

    static char		 mess[MAX_MESSLEN];
    char		 sender[MAX_GROUP_NAME];
    char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    int		 num_groups;
    int		 service_type;
    int16		 mess_type;
    int		 endian_mismatch;
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
        processRegularMessage(mess);
    }
    else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
}

void listHeaders(){
    command newCommand;
    newCommand.type = LISTMAILCMD;
    strcpy(newCommand.user_name, userName);
    sendCommand(&newCommand);
}

mailSetup(){
    
}

deleteMail(){
    
}

readMail(){
    
}

printMembership(){
    
}

void displayMenu(){
    printf("*****Use following commands*****\n");
    printf("1. Login with a username. Example: \"u Tom\"\n");
    printf("2. Connect to a specific mail server. Example: \"c 3\"\n");
    printf("3. List the headers of the received mail. Example: \"l\"\n");
    printf("4. Mail a message to a user. Example: \"m\"\n");
    printf("5. Delete a mail. Example: \"d 10\"\n");
    printf("6. Read a mail. Example: \"r 10\"\n");
    printf("7. Print the membership of the mail servers. Example: \"v\"\n");
    printf("********************************\n");
}

int parseCommand(char *command){
    int l, mailNum;
    l = strlen(command);

    if(command[0] == 'u'){
        if(l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        strncpy(userName, command+2, l-2);
    }

    else if(command[0] == 'c'){
        if(strlen(userName) == 0 || l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        serverNum = atoi(command+2);

        if(serverNum < 1 || serverNum > 5){
            printf("Incorrect use\n");
            return -1;
        }
        sprintf(server_group, "%s%d", SERVER_GROUP_NAME, serverNum);

    }

    else if(command[0] == 'l'){
        if(strlen(userName) == 0 || serverNum < 1 || serverNum > 5){
            printf("Incorrect use\n");
            return -1;
        }

        listHeaders();
    }

    else if(command[0] == 'm'){
        if(strlen(userName) == 0 || serverNum < 1 || serverNum > 5){
            printf("Incorrect use\n");
            return -1;
        }

        mailSetup();
    }

    else if(command[0] == 'd'){
        if(strlen(userName) == 0 || serverNum < 1 || serverNum > 5
           || l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        mailNum = atoi(command+2);

        if(mailNum <= 0 || mailNum > maxMailNum){
            printf("Incorrect use\n");
            return -1;
        }

        deleteMail(mailNum);
    }

    else if(command[0] == 'r'){
        if(strlen(userName) == 0 || serverNum < 1 || serverNum > 5
           || l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        mailNum = atoi(command+2);

        if(mailNum <= 0 || mailNum > maxMailNum){
            printf("Incorrect use\n");
            return -1;
        }            

        readMail(mailNum);
    }

    else if(command[0] == 'v'){
        if(strlen(userName) == 0 || serverNum < 1 || serverNum > 5
           || l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        printMembership();
    }

    else{
        printf("Incorrect use\n");
        return -1;
    }

    return 0;
    
}
int main (int argc, char ** argv)
{
    char command[MAX_COMMAND_LENGTH];
    int r = -1;

    while(1){
        if(r == -1)
            displayMenu();

        gets(command);
        r = parseCommand(command);
    }

    return 0;
}
