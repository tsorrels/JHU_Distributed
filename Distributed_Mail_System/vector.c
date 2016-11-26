#include "sp.h"
#include "mail.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUM_EMAILS 20
#define NUM_USERS 20
#define NUM_UPDATES 20


int email_vector_init(email_vector * vector){
    vector->size = 0;
    vector->emails = malloc(sizeof(email) * NUM_EMAILS);
    if (vector->emails == NULL){
	return -1;
    }
    vector->capacity = NUM_EMAILS;
    return 0;
}


int update_vector_init(update_vector * vector){
    vector->size = 0;
    vector->updates = malloc(sizeof(update) * NUM_UPDATES);
    if (vector->updates == NULL){
	return -1;
    }
    vector->capacity = NUM_UPDATES;
    return 0;
}




/* searches vector for target time stamp and copies all 
 * indexes above down by 1 position
 * returns -1 if email does not exist in state
 * does not adjust memory allocated to vector */
int email_vector_delete(email_vector * vector, mail_id target){
    int i;

    /* search for lamport time stamp */
    for (i = 0 ; i < vector->size ; i++) {
	if (vector->emails[i].mailID.index == target.index &&
	    vector->emails[i].mailID.procID == target.procID){
	    break;
	}
    }

    if (i == vector->size){
	return -1;
    }

    /* copy all 'higher' emails 'down' by 1 */
    while (i < vector->size - 1){
	memcpy(&vector->emails[i], &vector->emails[i + 1], sizeof(email));
	i++;
    }
    vector->size --;
    return 0;
}


/* if vectory size == capacity, allocates new memory and copies all entries
 * searches for index of email that will be ordered before new mail
 * copies all entries 'above' target index up one index in array; this is O(n)
 * returns -1 if memory allocation fails and there is no room for new mail */
int email_vector_insert(email_vector * vector, email * emailPtr){
    email * newEmailList;
    //email * currentEmail;
    mail_id targetID;
    int index;
    int i;
    //targetID = (mail_id *) &emailPtr->mailID;
    targetID = emailPtr->mailID;

    if (vector->size == vector->capacity){
        newEmailList = malloc(sizeof(email) * (vector->capacity + NUM_EMAILS));
	if (newEmailList == NULL){
	    perror("Failed to allocate new memory in email_vector");
	    return -1;
	}
	memcpy(vector->emails, newEmailList, (sizeof(email) * vector->size));
	free(vector->emails);
	vector->emails = newEmailList;
    }    
 
    if (vector->size == 0){
	memcpy(&vector->emails[0], emailPtr, sizeof(email));
	vector->size ++;
    }

    else{
	/* find index of mail to insert after */
	//currentEmail = vector->emails[size - 1];
	index = vector->size - 1;	

	/* find correct update index */
	while(vector->emails[index].mailID.index >
		targetID.index){

	    index --;
	}

	/* find correct procID */
	if (vector->emails[index].mailID.index ==
		targetID.index){
    	    while(vector->emails[index].mailID.procID >
		    targetID.procID){
	        index --;
	    }	    
	}

	/* shift all 'higher' entries 'up' by 1 */
	i = index + 1;
	while (i <= vector->size){
	    memcpy(&vector->emails[i + 1], &vector->emails[i], sizeof(email));
	    i ++;	    
	}
	
	/* write in new email */
        memcpy(&vector->emails[index + 1], emailPtr, sizeof(email));
	vector->size ++;
    }
    return 0;
}



/* searches given vector for target updateIndex associated with the process
 * to which this vector is associated
 * returns a pointer to this update, or NULL */
update * update_vector_get(update_vector * vector, int updateIndex){
    update * targetUpdate;
    int i;

    targetUpdate = NULL;
    i = 0;

    while (i < vector->size){
	if (updateIndex == vector->updates[i].updateIndex){
	    targetUpdate = &vector->updates[i];
	    break;
	}
    }

    return targetUpdate;
}



int update_vector_insert(update_vector * vector, update * updatePtr){
    update * newUpdateList;
    int index;
    int i;

    if (vector->size == vector->capacity){
        newUpdateList = malloc(sizeof(update) * 
			       (vector->capacity + NUM_UPDATES));
	if (newUpdateList == NULL){
	    perror("Failed to allocate new memory in update_vector");
	    return -1;
	}
	memcpy(vector->updates, newUpdateList, (sizeof(update) * vector->size));
	free(vector->updates);
	vector->updates = newUpdateList;
    }    
 
    if (vector->size == 0){
	memcpy(&vector->updates[0], updatePtr, sizeof(update));
	vector->size ++;
    }

    else{
	/* find index of update to insert after */
	//currentEmail = vector->emails[size - 1];
	index = vector->size - 1;	

	/* find correct update index */
	while(vector->updates[index].updateIndex >
		updatePtr->updateIndex){

	    index --;
	}

	/* check if update is already in vector */
	if (vector->updates[index].updateIndex == updatePtr->updateIndex){
	    perror("ERROR: tried to insert update that already has");
	}

	/* shift all 'higher' entries 'up' by 1 */
	i = index + 1;
	while (i <= vector->size){
	    memcpy(&vector->updates[i + 1], &vector->updates[i], 
		   sizeof(update));
	    i ++;	    
	}
	
	/* write in update */
        memcpy(&vector->updates[index + 1], updatePtr, sizeof(update));
	vector->size ++;
    }
    return 0;
}



