#include "sp.h"


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int debug = 0;

/* Custom print method for debugging */
void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}



int main(int argc, char ** argv){

    if (argc < 4){
	printf("Usage: mcast <num_of_messages> <process_index> "); 
	printf("<num_of_processes> [debug]\n");
	exit(0);
    }

    if (argc == 5){
	if (atoi(argv[4]) == 1){
	    debug = 1;
	    printDebug("dubug set");
	}
    }

    /* initialize */



    return 0;
}
