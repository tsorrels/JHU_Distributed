#include "sp.h"
#include "mail.h"
#include <limits.h>

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


int max(int left, int right){
    if (left > right){
	return left;
    }
    return right;
}

int main (int argc, char ** argv)
{
    int	ret;
    int	ret2, i;
    sp_time test_timeout;
    update *updatePtr;

    
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

    loadState(&local_state); // define in recover.c

    for(i = 0; i < NUM_SERVERS; i++){
        updatePtr = local_state.local_update_buffer.procVectors[i].updates;
        if(updatePtr){
            while(updatePtr->next != NULL){
                updatePtr = updatePtr->next;
            }
            applyUpdate(updatePtr, 0);
        }
    }


    printf("User: connected to %s with private group %s\n", Spread_name, 
	   Private_group );

    E_init();


    // Join group "abrahmb1tsorrel3servers"
    sprintf( group, SERVER_GROUP_NAME);
    ret = SP_join( Mbox, group );
    if( ret < 0 ) SP_error( ret );
    switch(local_state.proc_ID){
    case 1:
        ret = SP_join( Mbox, SERVER_1_GROUP );
        ret2 = SP_join( Mbox, SERVER_1_MEM );

	break;

    case 2:
        ret = SP_join( Mbox, SERVER_2_GROUP );
        ret2 = SP_join( Mbox, SERVER_2_MEM );
	break;

    case 3:
        ret = SP_join( Mbox, SERVER_3_GROUP );
        ret2 = SP_join( Mbox, SERVER_3_MEM );
	break;

    case 4:
        ret = SP_join( Mbox, SERVER_4_GROUP );
        ret2 = SP_join( Mbox, SERVER_4_MEM );
	break;

    case 5:
        ret = SP_join( Mbox, SERVER_5_GROUP );
        ret2 = SP_join( Mbox, SERVER_5_MEM );
	break;
    }
    
    if( ret < 0 || ret2 < 0) SP_error( ret );

    E_attach_fd( Mbox, READ_FD, readSpreadMessage, 0, NULL, HIGH_PRIORITY );

    E_handle_events();

    return 0;
}


/* searches update vector for given procID for update with given updateIndex
 * returns pointer to update, or NULL if it is not in the buffer */
update * findUpdate(int procID, int updateIndex){
    //int targetProcID = updateIndex->procID;
    update_vector * targetVector;
    targetVector = &local_state.local_update_buffer.procVectors[procID - 1];
    //targetVector = &local_state.local_update_buffer.procVectors[targetVector-1];
    /* return pointer to update, or NULL */
    return update_vector_get(targetVector , updateIndex);	
}

int storeUpdate(update * updatePtr){
    int returnValue;
    int procID;

    returnValue = 0;
    procID = updatePtr->mailID.procID;

    returnValue = update_vector_insert(&local_state.local_update_buffer.
				       procVectors[procID - 1], updatePtr);

    return returnValue;
}




/* generates an update message to send to other servers 
 * returns a pointer to a full message */
message * generateUpdate(char * mess){
    command * commandPtr;
    message * messagePtr;
    message * updateMessage;
    update * updatePtr;
    email * emailPtr;

    messagePtr = (message *) mess;
    commandPtr = (command *) messagePtr->payload;
    emailPtr = (email *)commandPtr->payload;
    updateMessage = malloc(sizeof(message));
    updateMessage->header.type = UPDATE;

    /* load message update with message type, procID, and user_name */
    updatePtr = (update *) updateMessage->payload;
    updatePtr->mailID.procID = local_state.proc_ID;     //modify
    updatePtr->mailID.index = local_state.updateIndex + 1;
    memcpy(updatePtr->user_name, commandPtr->user_name, MAX_USER_LENGTH);

    if (commandPtr->type == NEWUSERCMD){
	updatePtr->type = NEWUSERMSG;
    }

    else if (commandPtr->type == DELETEMAILCMD){
	updatePtr->type = DELETEMAILMSG;
	//updatePtr->mailID = emailPtr->mailID;
    memcpy(updatePtr->payload, commandPtr->payload, UPDATE_PAYLOAD_SIZE);
    }

    else if (commandPtr->type == READMAILCMD){
	updatePtr->type = READMAILMSG;
	//updatePtr->mailID = emailPtr->mailID;
	memcpy(updatePtr->payload, commandPtr->payload, UPDATE_PAYLOAD_SIZE);
	// I dont think this memcpy is necessary
    }

    else if (commandPtr->type == NEWMAILCMD){
	updatePtr->type = NEWMAILMSG;
    emailPtr->mailID = updatePtr->mailID;
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
    update * updatePtr;
    updatePtr = (update *) updateMessage->payload;
    
    if (debug)
      printf("sending update <%d, %d>\n", updatePtr->mailID.procID,
	     updatePtr->mailID.index);

    ret= SP_multicast(Mbox, AGREED_MESS,(const char *)local_state.server_group,
		      1, sizeof(message), (const char *)(updateMessage) );
    if( ret < 0 ) 
    {
	perror("ERROR: failed to send update in sendUpdate");
	SP_error( ret );
	Bye();
    }
}



void printMatrix(){
    int i;
    int j;

    //int ** matrix;
    //matrix =  &local_state.local_update_matrix.latest_update;
    
    printf("Printing local matrix____________\n");

    for (i = 0 ; i < NUM_SERVERS ; i ++){
      printf(" From proc%d\t", i+1);
	for (j = 0 ; j < NUM_SERVERS ; j ++){
	  //	    printf(" %d", matrix[i][j]);
	   printf(" %d",local_state.local_update_matrix.latest_update[i][j]);
	}
	printf("\n");     

    }  
}


/* checks update matrix for a processes in the current membership that has
 * an update index for a process procIndex + 1 that is higher that the local 
 * update index for that process AND is a lower procID */
int checkLowestProcID(int procIndex, int targetUpdate)
{
    int i;

    //TODO: Tyler fix
    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if (local_state.local_update_matrix.latest_update[i][procIndex] ==0){
	    break;
	}
    }

    return 1;
}



/* counts members of current membership that is stored in local state */
int countMembers(){
    int i;
    int numMembers;

    numMembers = 0;
    
    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if ( local_state.current_membership[i][0] != '\0'){
	    numMembers ++;
	}
    }

    return numMembers;
}

/* checks the strings of membership stored in state for a particulare serverID
 * returns 1 if serverID is in current membership in state */
int checkMembership(int serverID){
    int i;

    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if (local_state.current_membership[i][7] == serverID + 48 &&
	    local_state.current_membership[i][0] != '\0'){
	    return 1;
	}
    }
    return 0;
}

/* check if this process has the highest update originating from a server in
 * the current membership */
int checkHighestUpdate(int serverIndex, int update){
    int i;

    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if (!checkMembership(i + 1)){
	    continue;
	}
        if(i == local_state.proc_ID - 1){
	    continue;
	}

	/* check if another process has a higher update than this process, or
	   check if it is the same update, but higher proc ID*/
	if ( (local_state.local_update_matrix.latest_update[i][serverIndex] >
	      update) ||
	     (local_state.local_update_matrix.latest_update[i][serverIndex] ==
	      update && i > local_state.proc_ID - 1) ){
	    return 0;
 	}
    }    

    return 1;
}


/* searches local update matrix for lowest update received by another process
 * in the current membership for a given serverID */
int getLowestUpdate(int serverIndex){
    int lowestUpdate = INT_MAX;
    int i;

    for (i = 0 ; i < NUM_SERVERS ; i ++){
        if(i == local_state.proc_ID - 1){
	    continue;
	}
	if (!checkMembership(i + 1)){
	    continue;
	}

	if (local_state.local_update_matrix.latest_update[i][serverIndex] <
	    lowestUpdate){
	    lowestUpdate = 
		local_state.local_update_matrix.latest_update[i][serverIndex];
	}
    }    

    return lowestUpdate;
}

/* send updates from a target process from index min to index max */
void sendMissingUpdates(int procID, int min, int max){
    int i;
    update * updatePtr;

    message * messagePtr;
    messagePtr = malloc (sizeof(message));
    messagePtr->header.type = UPDATE;
    messagePtr->header.proc_num = local_state.proc_ID;
    
    for (i = min ; i <= max ; i ++){
	updatePtr = findUpdate(procID, i);
	if (updatePtr != NULL){
	    memcpy(messagePtr->payload, updatePtr, sizeof(update));
	    sendUpdate(messagePtr);	    
	}	
    }

    free(messagePtr);
}

/* checks each element in target vector to see whether an update is missing
 * that is both in the local vector and does not exist in another proc's
 * vector that is in a) in the membership and b) lower in proc number */
void checkSendUpdates(int * localVector, int * targetVector){
    int i; /* server index */
    int lowestUpdate;

    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if (targetVector[i] < localVector[i] &&
	    checkHighestUpdate(i, localVector[i]) ){
	    /* this processes must send updates */
	    lowestUpdate = getLowestUpdate(i);
	    sendMissingUpdates(i + 1, lowestUpdate + 1, localVector[i]);
	}
    } 
}


void continueReconcile(){
    int * localVector;
    int * targetVector;

    int i;
    localVector =
      local_state.local_update_matrix.latest_update[local_state.proc_ID - 1];

    
    for (i = 0 ; i < NUM_SERVERS ; i ++){
        if(i == local_state.proc_ID - 1){
	    continue;
	}
	if (!checkMembership(i + 1)){
	    continue;
	}

	targetVector = local_state.local_update_matrix.latest_update[i];
	checkSendUpdates(localVector, targetVector);
    }
}



void updateMatrix(message * messagePtr){
    int i;
    int j;
    update_matrix * sentMatrix;
    update_matrix * localMatrix;    

    if (messagePtr->header.proc_num == local_state.proc_ID){
        if(debug)
	    printf("Received own matrix; skipping updateMatrix\n");
	return;
    }
    
    if (debug){
        printf("Updating matrix\n");
        printf ("Matrix before update:\n");
	printMatrix();
    }
    sentMatrix = (update_matrix * ) messagePtr->payload;
    localMatrix = &local_state.local_update_matrix;//.latest_update;

    /* for each process in matrix */
    for (i = 0 ; i < NUM_SERVERS ; i ++){
	if (i == local_state.proc_ID - 1){
	    /* this is the this process's vector */
	    continue;
	}
	for (j = 0 ; j < NUM_SERVERS ; j ++){
	    /* compare if this is more current data */
	    if (sentMatrix->latest_update[i][j] > 
		localMatrix->latest_update[i][j]){
		/* update this vector */
		localMatrix->latest_update[i][j] = 
		    sentMatrix->latest_update[i][j];
	    }
	}
    }

    if (debug){
        printf ("Matrix after update:\n");
        printMatrix();
    }
    
}

 

/* create new message containing local update matrix and sends to group
 * returns 0 on success, -1 on malloc failuer or send failure */
int sendMatrix(){
    int ret;
    message * mess;

    if (debug){
        printf("sending matrix:\n");
	printMatrix();
    }
    
    /* craft message */
    mess = malloc(sizeof(message));
    if (mess == NULL){
	perror("Malloc failed in sendMatrix\n");
	return -1;
	//Bye();
    }
    mess->header.type = MATRIX;
    mess->header.proc_num = local_state.proc_ID;
    memcpy(mess->payload, &local_state.local_update_matrix, 
	   sizeof(update_matrix));

    if (debug)
	printf("sending matrix\n");

    ret= SP_multicast(Mbox, AGREED_MESS,(const char *)local_state.server_group,
		      1, sizeof(message), (const char *)(mess) );

    free(mess);
    if( ret < 0 ) 
    {
	perror("ERROR: failed to send update in sendUpdate");
	SP_error( ret );
	return -1;
	//Bye();
    }
    return 0;
}


/* searches state for first instance of user with name userName
 * returns pointer to that user struct, or NULL if it does not exist
 * does not check for duplicate entries for user name */
user * findUser(char * userName){
    //int i; 
    user * userPtr;

    if (debug)
        printf("searching for username %s\n", userName);
    
    userPtr = user_vector_get(&local_state.users, userName);
    
    if(debug)
        printf("Returned from user_vector_get\n");
    
    return userPtr;
}

/* searches state for email with targetID for given userPtr
 * userPtr must point to a valid user struct in state 
 * returns NULL if email is not in state for this user*/
email * findEmail(user * userPtr, mail_id targetID){
    //int j;
    email * emailPtr;
    emailPtr = email_vector_get(&userPtr->emails, targetID);  

    return emailPtr;
}

/* searches for user in updatePtr, then emailID in updatePtr
 * if search is successful, mark email->read = 1
 * else, return 1 */
int markAsRead(update * updatePtr){
    user * userPtr;
    mail_id targetID;
    email * emailPtr;
    int returnValue;
    emailPtr = (email *)updatePtr->payload;

    targetID = emailPtr->mailID;
    userPtr = NULL;
    returnValue = 1;

    if (debug)
	printf("Marking message as read\n");

    /* find user */
    userPtr = findUser(updatePtr->user_name);

    if (debug && userPtr == NULL)
	printf("Could not find user = %s in markAsRead\n",
	       updatePtr->user_name);

    /* find email */
    if (userPtr != NULL){
	emailPtr = findEmail(userPtr, targetID);
	if (emailPtr != NULL){
	    if (debug)
		printf("Marking read email user = %s procID = %d index = %d\n",
		       updatePtr->user_name, targetID.procID, 
		       targetID.index);
	    /* mark email as read */
        if(emailPtr->read == 0){
            emailPtr->read = 1;
            returnValue = 0;
            writeUser(userPtr);
        }
	}
	else{
	    if (debug)
		printf("Couldnt find email user = %s procID = %d index = %d\n",
		       updatePtr->user_name, targetID.procID, 
		       targetID.index);
	}   
    }		
    return returnValue;
}


/* inserts mail included in updatePtr 
 * first searches for user, then calls email_vector_insert which inserts into
 * user email vector, maintaining sorting by lamport time stamp
 * returns -1 if cannot find user or insertion fails because of capacity and 
 * inability to allocate more memory to add this email */
int addMail(update * updatePtr){
    user * userPtr;
    email * emailPtr;
    int checkError;
    //int returnValue;

    emailPtr = (email * ) updatePtr->payload;
    userPtr = NULL;
    checkError = -1;

    if (debug)
	printf("Adding mail to state\n");

    /* find user */
    userPtr = findUser(emailPtr->to);

    if (debug)
	printf("Returned from findUser\n");

    
    if (debug && userPtr == NULL)
	printf("Could not find user = %s in addMail\n",
	       updatePtr->user_name);    
    
    /* insert into user's email vector */
    if (userPtr != NULL){
	checkError = email_vector_insert(&userPtr->emails, emailPtr);
    if(checkError == 0)
        writeUser(userPtr);
	if (debug)
	  printf("in add mail, returned from emailvectorinsert added mail to inbox of %s\n", userPtr->name);
    }
    return checkError;
}

/* delete command will always execute if the message is in the state 
 * if email was deleted, return 0.  If not, return 1*/
int deleteMail(update * deleteUpdate){
    user * userPtr;
    mail_id targetID;
    email * emailPtr;
    int returnValue;
    emailPtr = (email *)deleteUpdate->payload;
    
    targetID = emailPtr->mailID;
    userPtr = NULL;
    returnValue = 1;

    if (debug)
	printf("Deleting mail\n");

    /* find user */
    userPtr = findUser(deleteUpdate->user_name);

    if (debug && userPtr == NULL)
	printf("Could not find user = %s in delete email\n",
	       deleteUpdate->user_name);

    /* find email */
    if (userPtr != NULL){
	emailPtr = findEmail(userPtr, targetID);
	if (emailPtr != NULL){
	    if (debug)
		printf("Deleting email user = %s procID = %d index = %d\n",
		       deleteUpdate->user_name, targetID.procID, 
		       targetID.index);
	    /* mark email as invalid */
	    //emailPtr->valid = 0;
	    returnValue = email_vector_delete(&userPtr->emails, targetID);

        if(returnValue == 0)
            writeUser(userPtr);
	    //returnValue = 0;
	}
	else{
	    if (debug)
		printf("Couldnt find email user = %s procID = %d index = %d\n",
		       deleteUpdate->user_name, targetID.procID, 
		       targetID.index);
	}   
    }
    return returnValue;
}


/* insert user into local state of user_vecotor
 * return 0 if user insertion executed
 * return 1 if user already exists
 * return -1 if vector_insert fails
 * user_name must be null terminated */
int addUser(char * user_name){
    int checkError;

    if (debug)
	printf("Adding user %s\n", user_name);

    
    checkError = user_vector_insert(&local_state.users, user_name);

    if(checkError == 0)
        writeUserList(&local_state);

    if (checkError > 0){
        if(debug)
  	    printf("User already exists in state\n");
	return checkError;
    }


    if (checkError < 0){
      	perror("ERROR: system failuer in vector_insert");
	return checkError;
    }

    
    return 0;
}




/* Updates this proc's vector in local update matrix
 * checks whether this is a more recent update before updating vector
 * return 0 on vector update, 1 if did not update vector */
int updateVector(update * updatePtr){
    int targetProcID;
    int targetUpdateIndex;
    int oldUpdateIndex;
    int returnValue;

    returnValue = 0;
    targetProcID = updatePtr->mailID.procID;
    targetUpdateIndex = updatePtr->mailID.index;
    oldUpdateIndex = local_state.local_update_matrix.latest_update
	[local_state.proc_ID - 1][targetProcID - 1];

    /* check if this is an old update */
    if (targetUpdateIndex > oldUpdateIndex){
	local_state.local_update_matrix.latest_update[local_state.proc_ID - 1]
	    [targetProcID - 1] = targetUpdateIndex;

	if (debug)
	    printf("Updated local vector w update for procID= %i, index= %i\n",
		   targetProcID, targetUpdateIndex);

    }
    else {
	returnValue = 1;
	if (debug)
	    printf("Did not update vector w update for procID= %i, index= %i\n",
		   targetProcID, targetUpdateIndex);	
    }

    return returnValue;
}



int applyUpdate(char * mess, int ignoreDup){
    message * messagePtr;
    update * updatePtr;
    int returnValue, index;


    
    returnValue = 0;
    messagePtr = (message *) mess;
    updatePtr = (update *) messagePtr->payload;


    /* check if we already received/applied this update */
    //TODO: check if this is a lower LTS than what we already have
    // modify
    if (ignoreDup && findUpdate(updatePtr->mailID.procID, updatePtr->mailID.index) != NULL){
	if (debug)
	    printf("Received duplicate update procID=%i, updateIndex = %i\n",
		   updatePtr->mailID.procID, updatePtr->mailID.index);
	return 1;
    }


    
    storeUpdate(updatePtr);
    updateVector(updatePtr);
    index = updatePtr->mailID.procID -1;

    writeUpdateBuffer(&local_state, index);
    writeUpdateMatrix(&local_state);

    
    if (updatePtr->type == NEWMAILMSG){
	returnValue = addMail(updatePtr);
    }

    else if (updatePtr->type == NEWUSERMSG){
	returnValue = addUser(updatePtr->user_name);
    }

    else if (updatePtr->type == DELETEMAILMSG){
	returnValue = deleteMail(updatePtr);
    }

    else if (updatePtr->type == READMAILMSG){
	returnValue = markAsRead(updatePtr);
    }

    /* increment update counter */
    //TODO: verify
    //if(returnValue == 0)
        local_state.updateIndex = max(local_state.updateIndex + 1,
				  updatePtr->mailID.index);
    //local_state.updateIndex ++;

    //storeUpdate(updatePtr);
    //updateVector(updatePtr);

    return returnValue;
}

void sendToClient(char groupName[SIZE_PRIVATE_GROUP], message * messagePtr)
{
    //int ret;
    SP_multicast(Mbox, AGREED_MESS,(const char *) groupName,
		      1, sizeof(message), (const char *)(messagePtr) );    
}

void generateResponse(char * mess){
    command * commandPtr;
    command * newCommandPtr;
    int i, size;

    message *messagePtr, *newMessagePtr;
    email *emailPtr, *newEmailPtr, *temp;
    mail_id targetID;
    user *userPtr;

    messagePtr = (message *) mess;
    commandPtr = (command *) messagePtr->payload;
    emailPtr = (email *)commandPtr->payload;
    newMessagePtr = malloc(sizeof(message));
    newMessagePtr->header.type = REPLY;
    newCommandPtr = (command *) newMessagePtr->payload;
    targetID = emailPtr->mailID;
    newCommandPtr->type = commandPtr->type;
    
    if(commandPtr->type == READMAILCMD){
        userPtr = findUser(commandPtr->user_name);
        if (userPtr != NULL){
            newEmailPtr = findEmail(userPtr, targetID);
            if (newEmailPtr != NULL){
                memcpy(newCommandPtr->payload, newEmailPtr, sizeof(email));
                newCommandPtr->ret = 0;
            }
            else
                newCommandPtr->ret = -1;
        }
        else
            newCommandPtr->ret = -1;
        sendToClient(commandPtr->private_group, newMessagePtr);
    }
    else if(commandPtr->type == LISTMAILCMD){
        userPtr = findUser(commandPtr->user_name);
        if(debug){
            printf("Inside generateResponse userptr = %p\n", userPtr);
            printf("Inside generateresponse name = %s\n", userPtr->name);
        }
        if (userPtr != NULL){
            newCommandPtr->ret = userPtr->emails.size;
            if(debug){
	      printf("Inside generateresponse size = %d\n", (int) userPtr->emails.size);
            }
            sendToClient(commandPtr->private_group, newMessagePtr);
            newCommandPtr->ret = -1;
            temp = userPtr->emails.emails;
            printf("Entering for loop\n");
            for(i = 0;i < userPtr->emails.size; i++){
                printf("temp = %p\n", temp);
                printf("i = %d size of email = %d size of command = %d from = %s, to = %s subject = %s\n", i,
		       (int)sizeof(email), (int)sizeof(command), temp->from, temp->to, temp->subject);
                memcpy(newCommandPtr->payload, temp, sizeof(email));
                printf("Memcpy successful\n");
                sendToClient(commandPtr->private_group, newMessagePtr);
                printf("Send to client successful\n");
                temp = temp->next;
            }
            printf("for loop successfull\n");
        }
        else{
            newCommandPtr->ret = 0;
            sendToClient(commandPtr->private_group, newMessagePtr);
        }
        
    }
    else if(commandPtr->type == SHOWMEMBERSHIPCMD){
        size = 0;
        for (i = 0 ; i < NUM_SERVERS ; i ++){
        if (local_state.current_membership[i][0] != '\0'){
            newCommandPtr->payload[size] = local_state.current_membership[i][7];
            size++;
        }
        }
        newCommandPtr->ret = size;
        sendToClient(commandPtr->private_group, newMessagePtr);
    }
    else{
       newCommandPtr->ret = 0;
       sendToClient(commandPtr->private_group, newMessagePtr);
    }
    free(newMessagePtr);
}

/* should this function return anything? */
void processRegularMessage(char * sender, int num_groups, 
			   char groups[][MAX_GROUP_NAME], 
			   int16 mess_type, char * mess){
    //char * privateGroup;
    message * updateMessage;
    command * commandPtr;
    //message * responseMessage;
    message * messagePtr;
    int createUpdate = 0;

    messagePtr= (message *) mess;

    if (debug)
        printf("Running processRegularMessage\n");

    if (messagePtr->header.type == COMMAND){
	commandPtr = (command *) messagePtr->payload;
	//privateGroup = commandPtr->private_group;

	if (commandPtr->type==NEWMAILCMD || commandPtr->type ==NEWUSERCMD ||
	    commandPtr->type==DELETEMAILCMD || commandPtr->type ==READMAILCMD){
	    createUpdate = 1;
	}

	if (createUpdate){
	    updateMessage = generateUpdate(mess); // mallocs a message
	    applyUpdate((char *)updateMessage, 1); // apply to state in this server
	    //if(ret == 0)
	        sendUpdate(updateMessage);
	    free(updateMessage); // frees update message		
	}
	if(debug)
	    printf("Generating response\n");
	generateResponse(mess);// mallocs a message
	//free(responseMessage); // frees response message
    }

    else if (messagePtr->header.type == UPDATE){
	//TODO: check reconcile
	/* check reconcile condition */

	applyUpdate(mess, 1);

    }

    else if (messagePtr->header.type == MATRIX){
        if (local_state.status != RECONCILE){
	    if (debug)
		printf("Received matrix message, but not reconciling\n");
	    /* do nothing */
	}

	/* status == RECONCILE */
	else{
	    /* update local matrix */
	    updateMatrix(messagePtr);
	    
	    local_state.local_update_matrix.num_matrix_recvd ++;
	    if (local_state.local_update_matrix.num_matrix_recvd ==
		countMembers()){
  	        if (debug){
		    printf("received %d matrices, continuing reconciliation\n",
			   local_state.local_update_matrix.num_matrix_recvd);
		}
	      
	        local_state.status = NORMAL;
		continueReconcile();
	    }
		//sendServerUpdates(messagePtr->header.proc_num);	      
	}
    }

    else{
	perror("ERROR: Did not recognize type in processRegularMessage");
    }

}



int checkReconcile(int num_in_group){
    if (num_in_group == 1){
	return 0;
    }
    return 1;
}


/* should this function return anything? */
void processMembershipMessage(char * sender, int num_groups, 
			   char groups[][MAX_GROUP_NAME], 
			   int16 mess_type, char * mess){
    int reconcile;
    int i;
    int memCtr;
    
    if (debug)
        printf("Runing processMembershipMessage\n");

    if (strcmp(sender, SERVER_GROUP_NAME) != 0) {
        if (debug)
	    printf("Membership message not for server group\n");
	return;
    }


    /* populate server group membership into state */
    for( i=0; i < num_groups; i++ ){
        printf("\t%s\n", &groups[i][0] );
	/* Copy current members into state */
	memcpy(local_state.current_membership[i], &groups[i][0],
	       MAX_GROUP_NAME);
    }
	    
    /* Mark empty slots in membership as null */	    
    for (memCtr = i; memCtr < NUM_SERVERS ; memCtr ++){
        local_state.current_membership[memCtr][0] = '\0';
    }

    reconcile = checkReconcile(num_groups);
    local_state.status = NORMAL;
    
    /* reset received matrix counter */
    local_state.local_update_matrix.num_matrix_recvd = 0;

    if (reconcile){
	/* set status to reconcile and broadcast update vectors */
        if (debug)
            printf("Reconciling\n");
      
        local_state.status = RECONCILE;
	sendMatrix();
    }
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
	  
	    printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n", sender, num_groups, mess_type );
	    for( i=0; i < num_groups; i++ ){
	        printf("\t%s\n", &target_groups[i][0] );
	    }
	    
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

	/* process this membership message */
	processMembershipMessage(sender, num_groups, target_groups, 
				 mess_type, mess);


    } else if ( Is_reject_mess( service_type ) )
    {
	printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
	       sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
    }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

}



void initialize(int argc, char ** argv){
    int i;
    int j;
    debug = 0;

    if (argc < 2){
	printf("Usage: server <server_index> [debug]\n"); 
	exit(0);
    }    

    if (argc == 3){
	if (atoi(argv[2]) == 1){
	    debug = 1;
	    printf("debug set\n");
	}
    }

    local_state.proc_ID = atoi(argv[1]);
    local_state.updateIndex = -1;
    local_state.status = NORMAL;
    local_state.users.user_head = NULL;

    /* initialize update vectors to invalid */
    for (i = 0 ; i < NUM_SERVERS ; i ++){
        local_state.local_update_buffer.procVectors[i].updates = NULL;
	for(j = 0 ; j < NUM_SERVERS ; j ++){
	    local_state.local_update_matrix.latest_update[i][j] = -1;
	}
    }
    local_state.local_update_matrix.num_matrix_recvd = 0;
    
    /* initialize not waiting on any updates */
    for (i = 0 ; i < NUM_SERVERS ; i ++){
	local_state.awaiting_updates[i] = -1;
    }


    sprintf(local_state.server_group, "%s", SERVER_GROUP_NAME);

    sprintf( User, "Server%s", argv[1] );
    //    sprintf( Spread_name, "4803"); // TODO: pull this out to config file
    sprintf( Spread_name, "10470"); // TODO: pull this out to config file
    //sprintf( "10470"); // TODO: pull this out to config file

}


void init(){


}


void Bye(){
    printf("Exiting\n");
    exit(0);


}
