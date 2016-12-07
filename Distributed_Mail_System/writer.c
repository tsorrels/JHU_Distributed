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

void writeUserList(state *local_state){
    user *temp;
    FILE *fd;
    char newName[30];
    temp = local_state->users.user_head;

    sprintf(newName, "userlisttemp");

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error creating userlist file\n");
        return;
    }
    while(temp){
        fprintf(fd, "%s\n", temp->name);
        temp = temp->next;
    }

    if(remove(USERLIST) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", USERLIST);

	if(rename(newName, USERLIST) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);

    fclose(fd);
}

void writeUpdateMatrix(state *local_state){
    FILE *fd;
    update_matrix *matrix;
    int i;
    char newName[30];

    matrix = &local_state->local_update_matrix;
    sprintf(newName, "updatematrixtemp");

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error creating update matrix file\n");
        return;
    }

    for(i = 0; i < NUM_SERVERS; i++){
        fprintf(fd, "%d %d %d %d %d\n", matrix->latest_update[i][0],
            matrix->latest_update[i][1], matrix->latest_update[i][2],
            matrix->latest_update[i][3], matrix->latest_update[i][4]);
    }

    if(remove(UPDATEMATRIX) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", UPDATEMATRIX);

	if(rename(newName, UPDATEMATRIX) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);

    fclose(fd);    
}

void writeUpdateBuffer(state *local_state, int index){
    FILE *fd;
    int ret;
    update *updatePtr;
    char oldName[30], newName[30];

    updatePtr = local_state->local_update_buffer.procVectors[index].updates;
    sprintf(oldName, "updatebuffer%d", index+1);
    sprintf(newName, "updatebuffertemp%d", index+1);

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error opening update buffer file while writing\n");
        return;
    }

    for(; updatePtr; updatePtr = updatePtr->next){
        if((ret = fwrite(updatePtr, sizeof(update), 1, fd)) != sizeof(update)){
            printf("Error writing update buffer file\n");
            fclose(fd);
            return;
        }
    }
    if(remove(oldName) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", oldName);

	if(rename(newName, oldName) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);

    fclose(fd);
}

void writeUser(user *userPtr){
    char newName[MAX_USER_LENGTH + 4], oldName[MAX_USER_LENGTH];
    FILE *fd;
    int ret;
    email *emailPtr;

    sprintf(newName, "%stemp", userPtr->name);
    sprintf(oldName, "%s", userPtr->name);

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error opening user %s file while writing\n", newName);
        return;
    }

    emailPtr = userPtr->emails.emails;
    for(; emailPtr; emailPtr = emailPtr->next){
        if((ret = fwrite(emailPtr, sizeof(email), 1, fd)) != sizeof(email)){
            printf("Error writing user %s file\n", newName);
            fclose(fd);
            return;
        }
    }
    if(remove(oldName) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", oldName);

	if(rename(newName, oldName) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);

    fclose(fd);
}