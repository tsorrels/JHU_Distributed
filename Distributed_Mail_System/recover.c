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


void loadState(state * local_state){
    struct stat st;

    if (stat("./recovery", &st) == -1) {
        mkdir("./recovery", 0700);
    }    

    chdir("./recovery");

    // set fd in state

    local_state->recoveryFD = open(CONTENTS, O_CREAT);
    

}



