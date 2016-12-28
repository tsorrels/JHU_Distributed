#include "sp.h"
#include "recover.h"
#include "mail.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



static void recoverUpdateMatrix(update_matrix *matrix){
    FILE *fd;
    int temp, ret, i ,j;
    
    if((fd = fopen(UPDATEMATRIX, "r")) == NULL){
        return;
    }
    i = 0;
    j = 0;
    while((ret = fread(&temp, sizeof(int), 1, fd)) == 1)
    {
        matrix->latest_update[i][j] = temp;
        if(j == 4){
            i++;
            j = 0;
        }
        else{
            j++;
        }
    }
    fclose(fd);
}

static void recoverUpdateBuffer(update_buffer *buffer){
    FILE *fd;
    char name[30];
    int ret;
    update *temp, *newUpdate;
    int i,j;

    for(i = 1; i <= NUM_SERVERS; i++){
        sprintf(name, "updatebuffer%d", i);

        if((fd = fopen(name, "r")) == NULL){
            continue;
        }

        if((newUpdate = malloc(sizeof(update))) == NULL){
	    printf("Error loading the update buffer\n");
	    return;
        }
        j = 0;

        while((ret = fread(newUpdate, sizeof(update), 1, fd)) == 1)
        {
            newUpdate->next = NULL;
            if(buffer->procVectors[i-1].updates){
                temp->next = newUpdate;
                temp = temp->next;
            }
            else{
                buffer->procVectors[i-1].updates = newUpdate;
                temp = newUpdate;
            }
            j++;
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
    int count, ret;
    email *newEmail, *temp;

    if((userFD = fopen(userPtr->name, "r")) == NULL){
        return;
    }

    count = 0;
    if((newEmail = malloc(sizeof(email))) == NULL){
            printf("Error loading the email\n");
            return;
    }
    while((ret = fread(newEmail, sizeof(email), 1, userFD)) == 1)
    {
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



