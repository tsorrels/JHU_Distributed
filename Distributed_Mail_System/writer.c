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
    int ret;
    temp = local_state->users.user_head;

    sprintf(newName, "userlisttemp");

    if((fd = fopen(newName, "w")) == NULL){
        printf("Error creating userlist file\n");
        return;
    }
    while(temp){
        //fprintf(fd, "%s\n", temp->name);
        ret = fwrite(temp->name, MAX_USER_LENGTH, 1, fd);
        if(ret != 1)
            printf("Error writing userlist to file\n");
        temp = temp->next;
    }

    /*if(remove(USERLIST) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", USERLIST);

	if(rename(newName, USERLIST) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);*/
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

    printf("writing update matrix to file:\n");
    for(i = 0; i < NUM_SERVERS; i++){
        for(j = 0; j < NUM_SERVERS; j++){
        /*fprintf(fd, "%d %d %d %d %d\n", matrix->latest_update[i][0],
            matrix->latest_update[i][1], matrix->latest_update[i][2],
            matrix->latest_update[i][3], matrix->latest_update[i][4]);*/
            ret = fwrite(matrix->latest_update[i][j], sizeof(int), 1, fd);
            if(ret != 1)
                printf("Error writing matrix to the file\n");
            else{
                printf("%d ", matrix->latest_update[i][j]);
            }
        }
        printf("\n");
    }

    /*if(remove(UPDATEMATRIX) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", UPDATEMATRIX);

	if(rename(newName, UPDATEMATRIX) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);*/
    
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
            printf("Error writing update buffer file ret = %d, sizeof update = %d\n", ret, sizeof(update));
            fclose(fd);
            return;
        }
        i++;
    }
    printf("%d updates written\n", i);
    /*if(remove(oldName) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", oldName);

	if(rename(newName, oldName) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);*/
    
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
    /*if(remove(oldName) != 0)
        fprintf(stderr, "Error deleting the file %s.\n", oldName);

	if(rename(newName, oldName) == 0)
        fprintf(stderr, "Error renaming %s.\n", newName);*/

    remove(oldName);
    rename(newName, oldName);
    

    fclose(fd);
}