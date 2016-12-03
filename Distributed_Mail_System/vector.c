#include "sp.h"
#include "mail.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUM_EMAILS 20
#define NUM_USERS 20
#define NUM_UPDATES 20

int email_vector_delete(email_vector * vector, mail_id target){
    email **pp, *temp;

    for(pp=&vector->emails; *pp && ((*pp)->mailID.index !=
		target.index || (*pp)->mailID.procID != target.procID);
        pp=&(*pp)->next);

    if(*pp){
        temp = *pp;
        *pp = (*pp)->next;
        free(temp);
        return 0;
    }
    return -1;
}

int update_vector_delete(update_vector * vector, mail_id target){
    update **pp, *temp;

    for(pp=&vector->updates; *pp && ((*pp)->mailID.index !=
		target.index || (*pp)->mailID.procID != target.procID);
        pp=&(*pp)->next);

    if(*pp){
        temp = *pp;
        *pp = (*pp)->next;
        free(temp);
        return 0;
    }
    return -1;
}

int user_vector_delete(user_vector * vector, char * name){
    user **pp, *temp;

    for(pp=&vector->user_head; *pp && strcmp((*pp)->name, name); pp=&(*pp)->next);

    if(*pp && strcmp((*pp)->name, name) == 0){
        temp = *pp;
        *pp = (*pp)->next;
        free(temp);
        return 0;
    }

    return -1;
}

int email_vector_insert(email_vector * vector, email * emailPtr){
    mail_id targetID;
    email *email_head, *newEmail, **pp, *temp;

    targetID = emailPtr->mailID;
    email_head = vector->emails;
    newEmail = malloc(sizeof(email));

    if(newEmail == NULL){
        perror("Failed to allocate new memory in email_vector");
	    return -1;
    }
    memcpy(newEmail, emailPtr, sizeof(email));
    
    /* find correct update index */
    for(pp=&email_head; *pp && (*pp)->mailID.index <
		targetID.index; pp=&(*pp)->next);

    if(*pp && (*pp)->mailID.index == targetID.index){
        for(; *pp && (*pp)->mailID.procID <
		targetID.index; pp=&(*pp)->next);
        temp = (*pp);
        *pp = newEmail;
        newEmail = temp;
    }
    else{
        temp = (*pp);
        *pp = newEmail;
    }
    (*pp)->next = temp;

    vector->size ++;
    return 0;

}

email * email_vector_get(email_vector * vector, mail_id target){
    email **pp;

    for(pp=&vector->emails; *pp && ((*pp)->mailID.index !=
        target.index || (*pp)->mailID.procID != target.procID);
        pp=&(*pp)->next);

    return (*pp);

}


update * update_vector_get(update_vector * vector, int updateIndex){
    update **pp;

    for(pp=&vector->updates; *pp && (*pp)->mailID.index !=
        updateIndex; pp=&(*pp)->next);

    return (*pp);

}

user * user_vector_get(user_vector * vector, char * name){
    user **pp;

    for(pp=&vector->user_head; *pp && strcmp((*pp)->name, name); pp=&(*pp)->next);

    return (*pp);

}

int update_vector_insert(update_vector * vector, update * updatePtr){
    update **pp, *newUpdate, *temp;
    
    if((newUpdate = malloc(sizeof(update))) == NULL){
        perror("Failed to allocate new memory in update_vector");
	    return -1;
    }

    memcpy(newUpdate, updatePtr, sizeof(update));

    for(pp=&vector->updates; *pp && (*pp)->mailID.index <
        updatePtr->mailID.index; pp=&(*pp)->next);
    
    temp = *pp;
    *pp = newUpdate;
    (*pp)->next = temp;

    return 0;
}

int user_vector_insert(user_vector * vector, char * name){
    user **pp, *newUser;
    
    if((newUser = malloc(sizeof(user))) == NULL){
        perror("Failed to allocate new memory in update_vector");
	    return -1;
    }

    strcpy(newUser->name, name);
    newUser->emails.emails = NULL;
    newUser->emails.size = 0;

    for(pp=&vector->user_head; *pp && strcmp((*pp)->name, name); pp=&(*pp)->next);

    if(*pp && strcmp((*pp)->name, name) == 0)
        return 1;

    *pp = newUser;
    (*pp)->next = NULL;

    return 0;
}


