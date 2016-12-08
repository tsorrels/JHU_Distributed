#include "sp.h"
#include "recover.h"
#include "mail.h"
#include <unistd.h>
//#include <ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


// state needs fd for recovery contents file


/*static void recover(int fd){
    int numBytesRead;
    char recoveryBuffer[sizeof(recovery_meta)];
    recovery_meta * recoveryStruct;

    numBytesRead = read(fd, recoveryBuffer, sizeof(recovery_meta));
    if (numBytesRead != sizeof(recovery_meta)){
	perror("In recover, failed to read entire recover_meta struct");
    }

    recoveryStruct = (recovery_meta *) recoveryBuffer;

    if (recoveryStruct->writing != 0){
	perror("In recover, loaded non-zero value for writing");
    }


}*/

static void recoverUpdateMatrix(update_matrix *matrix){
    FILE *fd;
    int temp[NUM_SERVERS], i, j, ret;
    
    if((fd = fopen(UPDATEMATRIX, "r")) == NULL){
        printf("update matrix file doesn't exist\n");
        return;
    }
    i = 0;
    //while((ret = fscanf(fd, "%d %d %d %d %d", &temp[0], &temp[1], &temp[2],
                //&temp[3], &temp[4]))
    while((ret = fread(temp, sizeof(int), NUM_SERVERS, fd)) == NUM_SERVERS)
    {
        for(j = 0; j < NUM_SERVERS; j++)
            matrix->latest_update[i][j] = temp[j];
    }
    fclose(fd);
}

static void recoverUpdateBuffer(update_buffer *buffer){
    FILE *fd;
    char name[30]; /*from[MAX_USER_LENGTH], to[MAX_USER_LENGTH];
    char subject[MAX_SUBJECT_LENGTH];
    char user_name[MAX_USER_LENGTH];
    update_type type;
    int procID, procID1, index, index1, read;*/
    int ret;
    update *updatePtr, *temp, *newUpdate;
    int i;

    for(i = 1; i < NUM_SERVERS; i++){
        sprintf(name, "updatebuffer%d", i);

        if((fd = fopen(name, "r")) == NULL){
            printf("%s file doesn't exist\n", name);
            return;
        }

        if((newUpdate = malloc(sizeof(update))) == NULL){
                printf("Error loading the update buffer\n");
                return;
        }

        updatePtr = buffer->procVectors[i-1].updates;
        //while(fscanf(fd, "%d %s %d %d %d %d %d %s %s %s", type, user_name, &procID, 
        //            &index, &read, &procID1, &index1, from, to, subject))
        while((ret = fread(newUpdate, sizeof(update), 1, fd)) == 1)
        {
            /*newUpdate->type = type;
            strcpy(newUpdate->user_name, user_name);
            newUpdate->mailID.procID = procID;
            newUpdate->mailID.index = index;
            newEmail = (email *)newUpdate->payload;
            newEmail->read = read;
            strcpy(newEmail->from, from);
            strcpy(newEmail->to, to);
            strcpy(newEmail->subject, subject);
            fscanf(fd,"%[^\n]", newEmail->message);*/
            newUpdate->next = NULL;
            if(updatePtr){
                temp->next = newUpdate;
                temp = temp->next;
            }
            else{
                updatePtr = newUpdate;
                temp = newUpdate;
            }
            if((newUpdate = malloc(sizeof(update))) == NULL){
                printf("Error loading the update buffer\n");
                return;
            }
        }
        free(newUpdate);
        fclose(fd);
    }
}

static void recoverUser(user *userPtr){
    FILE *userFD;
    //int read, procID, index; 
    int count, ret;
    //char from[MAX_USER_LENGTH], to[MAX_USER_LENGTH];
    //char subject[MAX_SUBJECT_LENGTH];
    //message[MAX_MESSAGE_SIZE];
    email *newEmail, *temp;

    if((userFD = fopen(userPtr->name, "r")) == NULL){
        printf("Userfile %s doesn't exist\n", userPtr->name);
        return;
    }

    count = 0;
    //while(fscanf(userFD, "%d %d %d %s %s %s", &read, &procID, &index,
    //            from, to, subject))
    if((newEmail = malloc(sizeof(email))) == NULL){
            printf("Error loading the email\n");
            return;
    }
    while((ret = fread(newEmail, sizeof(email), 1, userFD)) == 1)
    {
        /*newEmail->read = read;
        newEmail->mailID.procID = procID;
        newEmail->mailID.index = index;
        strcpy(newEmail->from, from);
        strcpy(newEmail->to, to);
        strcpy(newEmail->subject, subject);
        fscanf(fd,"%[^\n]", newEmail->message);*/
        //strcpy(newEmail->message, message);
        newEmail->next = NULL;

        if(userPtr->emails.emails){
            temp->next = newEmail;
            temp = temp->next;
        }
        else{
            userPtr->emails.emails = newEmail;
            temp = newEmail;
        }
        count++;
        if((newEmail = malloc(sizeof(email))) == NULL){
            printf("Error loading the email\n");
            return;
        }
    }
    free(newEmail);
    userPtr->emails.size = count;
    fclose(userFD);
}

static void recoverUserList(FILE *fd, state *local_state){
    char name[MAX_USER_LENGTH];
    user *temp, *newUser;
    int ret;
    local_state->users.user_head = NULL;
    //while(ret = fread(fd, "%s", name)){
    while((ret = fread(name, MAX_USER_LENGTH, 1, fd)) == 1){
        if((newUser = malloc(sizeof(user))) == NULL){
            printf("Error loading the usernames\n");
            return;
        }
        strcpy(newUser->name, name);
        newUser->emails.emails = NULL;
        newUser->next = NULL;
        if(local_state->users.user_head){
            temp->next = newUser;
            temp = temp->next;
        }
        else{
            local_state->users.user_head = newUser;
            temp = newUser;
        }
    }
}


void loadState(state * local_state){
    struct stat st;
    int stateExists;
    FILE *userListFD;
    user *temp;
    stateExists = 1;

    /*if (stat("./recovery", &st) == -1) {
        //mkdir("./recovery", 0700);
        return;
    }    

    chdir("./recovery");*/

    // set fd in state

    if (stat(USERLIST, &st) == -1) {
        stateExists = 0;
    }
    
    userListFD = fopen(USERLIST, "r");

    if (stateExists == 1){
	recoverUserList(userListFD, local_state);
    fclose(userListFD);
    }

    temp = local_state->users.user_head;
    while(temp != NULL){
        recoverUser(temp);
        temp = temp->next;
    }
    recoverUpdateMatrix(&local_state->local_update_matrix);
    recoverUpdateBuffer(&local_state->local_update_buffer);
}



