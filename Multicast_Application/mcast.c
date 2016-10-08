#include "mcast.h"
#include "net_include.h"
#include "sendto_dbg.h"


/* global variables, accessable throughout program */
sender_window senderWindow;
global_window globalWindow;
int debug;
process_state_type processState;


void initializeGlobalWindow(){

}

void initializeSenderWindow(){

}

/* only called when state is waiting for start packet */
int processStartPacket(char * messageBuffer){

    
    return 0;
}

/* returnes what value of timer will be in next select call */
int processPacket(char * messageBuffer){

    
    return 0;
}

void handleTimeout(){

}


int main (int argc, char ** argv)
{
    struct sockaddr_in name; // receive address struct
    struct sockaddr_in send_addr; // send address struct

    int                mcast_addr; // mcast address

    struct ip_mreq     mreq; //multicast required stuct
    unsigned char      ttl_val; // TTL to ensure packets stay in network

    int                sockRecvMcast; // receive MCast socket
    int                sockSendMcast; // send MCast socket
    fd_set             mask; // FD setup for select
    fd_set             dummy_mask; // FD setup for select
    fd_set             temp_mask; // FD setup for select
    int                numReadyFDs; // num FDs ready to read from select
    int                numBytesRead; // used in calls to recv
    char               mess_buf[sizeof(packet)]; // buffer to receive packets
    struct timeval     timeout; // for even loop timeout

    int startMessageReceived; // indicates whether processes recvd start message

    /* initialize */
    initializeGlobalWindow();
    initializeSenderWindow();
    startMessageReceived = 0;
    mcast_addr = 225 << 24 | 1 << 16 | 2 << 8 | 120; /* (225.1.2.120) */
    debug = 0;
    processState = WAITING_START;
    if (argc > 5){
	if (atoi(argv[5]) == 1){
	    //set debug
	    debug = 1;
	}
    }

    /******* BEGIN SET UP MCAST RECEIVE SOCKET ********/
    sockRecvMcast = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockRecvMcast < 0) {
        perror("Mcast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sockRecvMcast, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Mcast: bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = htonl( mcast_addr );
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sockRecvMcast, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) 
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }
    /******* END SET UP MCAST RECEIVE SOCKET ********/

    /******* BEGIN SET UP MCAST SEND SOCKET ********/
    sockSendMcast = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (sockSendMcast < 0) {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    if (setsockopt(sockSendMcast, IPPROTO_IP, IP_MULTICAST_TTL, 
		   (void *)&ttl_val, sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d",ttl_val );  
	printf("- ignore in WinNT or Win95\n");
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(mcast_addr);  /* mcast address */
    send_addr.sin_port = htons(PORT);
    /******* END SET UP MCAST SEND SOCKET ********/



    /******* SET UP FDs FOR SELECT ********/
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sockRecvMcast, &mask );


    if (debug)
	printf("starting loop to wait for start message\n");
    /* WAIT FOR START MESSAGE */
    for (;;)
    {
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

	temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask, &timeout);

        if (numReadyFDs > 0) {
	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

		startMessageReceived = processStartPacket(mess_buf);
		if (startMessageReceived == 1){
		    break;
		}
	    }
	}

	else{
            printf(".");
            fflush(0);
	}
    }

    if (debug)
	printf("Received start message, continuing to event loop");


    /* MAIN EVENT LOOP */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    for (;;)
    {
	temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask, &timeout);

        if (numReadyFDs > 0) {
	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

		timeout.tv_usec = processPacket(mess_buf);
	    }
	}

	else{
	    handleTimeout();
	}
    }

    return 0;
}
