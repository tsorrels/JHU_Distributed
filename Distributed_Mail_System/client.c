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
static  char    Spread_name[80];
static	char	User[80];
static  int     To_exit = 0;
static  int     curr_count = 0;
email *emailHead;

int sendCommand(command *newCommand){
    int ret;
    message mess;
    mess.header.type = COMMAND;
    memcpy(mess.payload, newCommand, sizeof(command));
    ret= SP_multicast(Mbox, AGREED_MESS,(const char *)server_group, 
                        1, sizeof(message), (const char *)(&mess) );

    if( ret < 0 ) 
    {
	perror("ERROR: failed to send update in sendUpdate");
	SP_error( ret );
    }
    return ret;
}

void displayList(){
    email *mail;
    int c = 0;
    printf("Username: %s\n",userName);
    printf("Server Index: %d\n", serverNum);
    printf("Read Messages:\n");
    for(mail = emailHead; mail; mail = mail->next){
        if(mail->read){
            c++;
            printf("%d Sender: %s, Subject: %s\n", c, mail->from, mail->subject);
        }
    }
    printf("Unread Messages:\n");
    for(mail = emailHead; mail; mail = mail->next){
        if(!mail->read){
            c++;
            printf("%d Sender: %s, Subject: %s\n", c, mail->from, mail->subject);
        }
    }
    
    curr_count = 0;
}

void processRegularMessage(message *mess){
    command_type commandType;
    email **pp, *newEmail, *mail;
    command *com;
    com = (command *)mess->payload;
    commandType = com->type;
    
    if(commandType == LISTMAILCMD){
        if(com->ret != -1){
            maxMailNum = com->ret;
            curr_count = 0;
        }
        else{            
            for(pp=&emailHead; *pp; pp=&(*pp)->next);

            if((newEmail = malloc(sizeof(email))) == NULL)
                printf("No space to store a mail\n");
            else{
                memcpy(newEmail, com->payload, sizeof(email));
                *pp = newEmail;
                (*pp)->next = NULL;
            }
            curr_count++;
            if(curr_count == maxMailNum)
                displayList();
        }
    }
    else if(commandType == NEWUSERCMD){
        if(com->ret == -1)
            printf("Could not login with the username\n");
    }
    else if(commandType == NEWMAILCMD){
        if(com->ret == -1)
            printf("Could not send the mail\n");
    }
    else if(commandType == DELETEMAILCMD){
        if(com->ret == -1)
            printf("Could not delete the mail\n");
    }
    else if(commandType == READMAILCMD){
        if(com->ret == -1)
            printf("Could not read the mail\n");
        else{
            mail = (email *)com->payload;
            printf("%s\n", mail->message);
        }
    }
    else if(commandType == SHOWMEMBERSHIPCMD){
        if(com->ret == -1)
            printf("Cannot not showmembership the mail\n");
        else{
            mail = (email *)com->payload;
            printf("%s\n", mail->message);
        }
    }
    else{
        printf("Undefined command type\n");
    }
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
        processRegularMessage((message *)mess);
    }
    else if( Is_membership_mess( service_type ) )
        printf("Membership changed\n");
    else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
}

void Bye(){
    printf("Exiting\n");
    exit(0);
}

void loginUser(){
    command newCommand;
    email mail;
    newCommand.type = NEWUSERCMD;
    strcpy(newCommand.user_name, userName);
    strcpy(newCommand.private_group, private_group);
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
    
}

void listHeaders(){
    command newCommand;
    email mail;
    newCommand.type = LISTMAILCMD;
    strcpy(newCommand.user_name, userName);
    strcpy(newCommand.private_group, private_group);
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
}

void mailSetup(){
    command newCommand;
    email mail;
    char sendTo[MAX_USER_LENGTH];
    char subject[MAX_SUBJECT_LENGTH];
    char message[MAX_MESSAGE_SIZE];

    printf("to: ");
    gets(sendTo);
    printf("subject: ");
    gets(subject);
    printf("message: ");
    gets(message);

    newCommand.type = NEWMAILCMD;
    strcpy(mail.from, userName);
    strcpy(newCommand.user_name, userName);
    strcpy(newCommand.private_group, private_group);
    strcpy(mail.to, sendTo);
    strcpy(mail.subject, subject);
    strcpy(mail.message, message);
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
}

void deleteMail(int mailNum){
    command newCommand;
    email mail;
    email *head;
    int i;
    newCommand.type = DELETEMAILCMD;
    strcpy(newCommand.private_group, private_group);
    strcpy(newCommand.user_name, userName);
    
    for(head = emailHead, i = 1; head && i != mailNum; head = head->next, i++);
    memcpy(&mail, head, sizeof(email));
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
}

void readMail(int mailNum){
    command newCommand;
    email *head, mail;
    int i;
    newCommand.type = READMAILCMD;
    strcpy(newCommand.private_group, private_group);
    strcpy(newCommand.user_name, userName);

    for(head = emailHead, i = 1; head && i != mailNum; head = head->next, i++);
    memcpy(&mail, head, sizeof(email));
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
}

void printMembership(){
    command newCommand;
    email mail;
    newCommand.type = SHOWMEMBERSHIPCMD;
    strcpy(newCommand.user_name, userName);
    strcpy(newCommand.private_group, private_group);
    memcpy(newCommand.payload, &mail, sizeof(email));
    sendCommand(&newCommand);
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
        if(serverNum == 0 || l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        strncpy(userName, command+2, l-2);
        printf("Username = %s\n", userName);
        loginUser();
    }

    else if(command[0] == 'c'){
        if(l <= 2){
            printf("Incorrect use\n");
            return -1;
        }

        serverNum = atoi(command+2);

        if(serverNum < 1 || serverNum > 5){
            printf("Incorrect use\n");
            return -1;
        }
        switch(serverNum){
            case 1:
                sprintf(server_group, "%s", SERVER_1_GROUP );
            break;

            case 2:
                sprintf(server_group, "%s", SERVER_2_GROUP );
            break;

            case 3:
                sprintf(server_group, "%s", SERVER_3_GROUP );
            break;

            case 4:
                sprintf(server_group, "%s", SERVER_4_GROUP );
            break;

            case 5:
                sprintf(server_group, "%s", SERVER_5_GROUP );
            break;
        }            
            printf("server_group = %s\n", server_group);
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

void userCommand(){
    char command[MAX_COMMAND_LENGTH];
    if(!curr_count){
        //if(r == -1)
            //displayMenu();

        gets(command);
        parseCommand(command);
        printf("\nUser> ");
        fflush(stdout);
    }

}

int main (int argc, char ** argv)
{

    int ret;
    sp_time test_timeout;

    
    test_timeout.sec = TEST_TIMEOUT_SEC;
    test_timeout.usec = TEST_TIMEOUT_USEC;

    sprintf( Spread_name, "10470");
    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, 
			      private_group, test_timeout );
    if( ret != ACCEPT_SESSION ) 
    {
	SP_error( ret );
	Bye();
    }
    
    printf("User: connected to %s with private group %s\n", Spread_name, 
	   private_group );

    E_init();
    
    E_attach_fd( 0, READ_FD, userCommand, 0, NULL, LOW_PRIORITY );
    
    E_attach_fd( Mbox, READ_FD, readSpreadMessage, 0, NULL, HIGH_PRIORITY );
    
    displayMenu();
    printf("\nUser> ");
    fflush(stdout);

    E_handle_events();

    return 0;

}
