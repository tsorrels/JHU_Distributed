#include <stdio.h>

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#define PORT	     10470

#define MAX_MESS_LEN 1400

