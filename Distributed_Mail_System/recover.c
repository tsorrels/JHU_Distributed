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


// state needs fd for recovery contents file


static void recover(int fd){
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


}


void loadState(state * local_state){
    struct stat st;
    int stateExists;
    stateExists = 1;


    if (stat("./recovery", &st) == -1) {
        mkdir("./recovery", 0700);
    }    

    chdir("./recovery");

    // set fd in state

    if (stat(CONTENTS, &st) == -1) {
        stateExists = 0;
    }    
    
    local_state->recoveryFD = open(CONTENTS, O_CREAT);

    if (stateExists == 1){
	recover(local_state->recoveryFD);
    }

}



