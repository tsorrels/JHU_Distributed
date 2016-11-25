#include "sp.h"
#include "mail.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUM_EMAILS 20
#define NUM_USERS 20
/*
typedef struct email_vector_type{
    int size;
    int capacity;
    email * emails;

} email_vector;
*/

/*
int user_vector_init(user_vector * vector){
    vector->size = 0;
    vector->emails = malloc(sizeof(email) * NUM_EMAILS);
    if (vector->emails == NULL){
	return -1;
    }
    vector->capacity = NUM_EMAILS;
    return 0;
    }*/



int email_vector_init(email_vector * vector){
    vector->size = 0;
    vector->emails = malloc(sizeof(email) * NUM_EMAILS);
    if (vector->emails == NULL){
	return -1;
    }
    vector->capacity = NUM_EMAILS;
    return 0;
}



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
