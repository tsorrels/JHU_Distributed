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

void writeUserList(state *local_state){
    user *temp;
    FILE *fd;
    char newName[30];
    int ret;
    temp = local_state->users.user_head;

    sprintf(newName, "userlisttemp");

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error creating userlist file\n");
        return;
    }
    while(temp){
        ret = fwrite(temp->name, MAX_USER_LENGTH, 1, fd);
        if(ret != 1)
            printf("Error writing userlist to file\n");
        temp = temp->next;
    }

    remove(USERLIST);
    rename(newName, USERLIST);

    fclose(fd);
}

void writeUpdateMatrix(state *local_state){
    FILE *fd;
    update_matrix *matrix;
    int i, ret, j;
    char newName[30];

    matrix = &local_state->local_update_matrix;
    sprintf(newName, "updatematrixtemp");

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error creating update matrix file\n");
        return;
    }

    for(i = 0; i < NUM_SERVERS; i++){
        for(j = 0; j < NUM_SERVERS; j++){
            ret = fwrite(&matrix->latest_update[i][j], sizeof(int), 1, fd);
            if(ret != 1)
                printf("Error writing matrix to the file\n");
        }
    }

    
    remove(UPDATEMATRIX);
    rename(newName, UPDATEMATRIX);

    fclose(fd);    
}

void writeUpdateBuffer(state *local_state, int index){
    FILE *fd;
    int ret;
    update *updatePtr;
    char oldName[30], newName[30];
    int i=0;

    updatePtr = local_state->local_update_buffer.procVectors[index].updates;
    sprintf(oldName, "updatebuffer%d", index+1);
    sprintf(newName, "updatebuffertemp%d", index+1);

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error opening update buffer file while writing\n");
        return;
    }

    for(; updatePtr; updatePtr = updatePtr->next){
        if((ret = fwrite(updatePtr, sizeof(update), 1, fd)) != 1){
            fclose(fd);
            return;
        }
        i++;
    }
    
    remove(oldName);
    rename(newName, oldName);

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
        if((ret = fwrite(emailPtr, sizeof(email), 1, fd)) != 1){
            printf("Error writing user %s file\n", newName);
            fclose(fd);
            return;
        }
    }

    remove(oldName);
    rename(newName, oldName);
    

    fclose(fd);
}
