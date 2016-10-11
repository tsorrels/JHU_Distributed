#include "mcast.h"
//#include "net_include.h"
#include "sendto_dbg.h"


/* global variables, accessable throughout program */
connection mcastConnection;

sender_window senderWindow;
global_window globalWindow;
int debug;
process_state_type processState;


int numPacketsToSend;
int machineIndex; 
int numProcesses;
int lossRate;

void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}


void initializeGlobalWindow(){
    int i;
    globalWindow.window_start = 0;
    globalWindow.window_end = WINDOW_SIZE;
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
        globalWindow.packets[i].received = 0;
	globalWindow.packets[i].size = -1;
	globalWindow.packets[i].seq_num = i;      
    }
    globalWindow.previous_ack = -1;
    globalWindow.has_lowered = 0;
}

void initializeSenderWindow(){
  //int i;
    senderWindow.window_start = 0;
    senderWindow.window_end = 0;
    senderWindow.num_built_packets = 0;

}


/* check to see if packet is in window, return 1 for true */
int packetInWindow(packet * recvdPacket){


    return 0;
}

/* places packet into global window */ 
void receivePacket(packet * recvdPacket){
  int seq_num;

  seq_num = recvdPacket->header.seq_num;
    // check to see if in window
  if (seq_num < globalWindow.window_start ||
      seq_num > globalWindow.window_end){
    printDebug("received packet outside window");
  }

  else{
    // copy packet into global window
    memcpy(& globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes,
	   recvdPacket, sizeof(packet));
    globalWindow.packets[seq_num % WINDOW_SIZE].received = 1;

  }
}


/* returns seq_num of highest consequtive seq_num in global window; returns
 * -1 if it is empty */
int getConsecutiveAck(){

    int i;
  
    for (i = globalWindow.window_start ; i < globalWindow.window_end ; i ++){
      if (globalWindow.packets[i % WINDOW_SIZE].received != 1){
	break;
      }      
    }   
    return i - 1;
}


void slideGlobalWindow(packet * token){

  token_payload * tokenPayload = (token_payload *) token->data;

  
  
}


void clearGlobalWindow(){

    // check condition

    // deliver data

    slideGlobalWindow();    
}

void resendNaks(packet * recvdPacket){

}

/* pass in sequence number to stamp on packet, and send packet */
void sendPacket(int seq_num){

    // stamp sequence number
    packet * sendingPacket;


    if (debug)
      printf("sending packet seq_num =  %d\n", seq_num);
    
    sendingPacket = &(senderWindow.packets[senderWindow.window_start]);
    sendingPacket->header.seq_num = seq_num;
  
    //send
    sendto(mcastConnection.fd, sendingPacket, sizeof(packet), 0,
	 (struct sockaddr*) &mcastConnection.send_addr,
	   sizeof(mcastConnection.send_addr));

    // load into global window
    memcpy(&globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes,
	   sendingPacket, sizeof(packet));
    globalWindow.packets[seq_num % WINDOW_SIZE].received = 1;
    //globalWindow.window_end ++;

    
    //clear buffer?
    senderWindow.window_start = (senderWindow.window_start + 1)
      % SEND_WINDOW_SIZE ;

    

    senderWindow.num_sent_packets ++;
}

/* takes ARU/cumulative ack from token 
 * sends up to the group cumulative ack + MAX_MESSAGE */
void sendPackets(int ack, int seq_num){
    int i;
    i = seq_num + 1;
    while (
	   //i <= (ack + WINDOW_SIZE) &&
	   i <= globalWindow.window_end &&
	   i <= seq_num + MAX_MESSAGE &&
	   senderWindow.num_sent_packets <= senderWindow.num_built_packets){
        sendPacket(i);
	i ++;
    }  
}

int newRandomNumber(){


    return 1;
}


/* this method overwrites the passed address in the sender_window */
void buildPacket(packet * packetAddress){
    printDebug("building new packet");
    packetAddress->header.proc_num = machineIndex;
    packetAddress->header.seq_num = -1;
    packetAddress->header.type = DATA;
    packetAddress->header.rand_num = newRandomNumber();
}

/* loops through sender window and replenishes the window with new packets
 * up to window_start + SEND_WINDOW_SIZE; does not run if this process
 * has already built its prescribd number of packets */
void craftPackets(){

    do{
        if (senderWindow.num_built_packets == numPacketsToSend){
  	    printDebug("built max packets");
	    break;
	}
	//newPacket = buildPacket()
	buildPacket(&senderWindow.packets[senderWindow.num_built_packets %
					 SEND_WINDOW_SIZE]);
        senderWindow.window_end =
	  (senderWindow.window_end + 1) % SEND_WINDOW_SIZE;
	senderWindow.num_built_packets ++;
    }
    while (senderWindow.window_end != senderWindow.window_start +
	   SEND_WINDOW_SIZE);
  
}

void buildToken(){
    

  
}

void sendToken(){

}


/* this method does a LOT,  but the idea is to add a bunch of 
   functionality in this one method and then just be able to 
   change the entry point into the function; this minimizes
   rewriting stuff when we swtich token operations to unicast */
void processToken(packet * recvdPacket){

    resendNaks(recvdPacket);

    slideGlobalWindow(recvdPacket);

    sendPackets(0, 0);
    
    //deliver data

    craftPackets();

    buildToken();

    sendToken();

}

/* only called when state is waiting for start packet */
int processStartPacket(char * messageBuffer, int numBytes){
    packet * packetPtr;
    int returnValue;
    
    printDebug("Processing packet in processStartPacket");
  
    packetPtr = (packet*) messageBuffer;
    returnValue = 0;

    
    
    if (numBytes == sizeof(packet) && packetPtr->header.type == START){
        returnValue = 1;
    }

    return returnValue;
}


void processDataPacket(packet * recvdPacket){
    if (packetInWindow(recvdPacket)){
	receivePacket(recvdPacket);

    }
    else{
	printDebug("received data packet outside of window");
    }
}
 

/* returnes what value of timer will be in next select call */
int processPacket(char * messageBuffer, int numBytes){
    packet * recvdPacket;
    int timerValue;
    int clearWindow;

    clearWindow = 0; /* not sure how we will set this, prolly with a funccal */
    timerValue = 10; /* default of resetting timer to 10usec */
    recvdPacket = (packet*)messageBuffer;
    
    if(recvdPacket->header.type == DATA){
	printDebug("Received data packet from mcast socket");

	processDataPacket(recvdPacket);

	clearGlobalWindow();

    }

    else if(recvdPacket->header.type == TOKEN){
	printDebug("Received token packet from mcast socket");

	/* this method does a LOT,  but the idea is to add a bunch of 
	   functionality in this one method and then just be able to 
	   change the entry point into the function; this minimizes
	   rewriting stuff when we swtich token operations to unicast */
	processToken(recvdPacket);

    }


    else{
	printDebug("Received non- packet from mcast socket");

    }

    
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
    int                begin;
    
    int startMessageReceived; // indicates whether processes recvd start message

    /* INITIALIZE */
    if (argc < 5){
	printf("usage: ./mcast <num_of_packets> <machine_index> ");
	printf("<num_of_machines> <loss_rate>\n ");
	exit(0);
    }
    numPacketsToSend = atoi(argv[1]);
    machineIndex = atoi(argv[2]);
    numProcesses = atoi(argv[3]);
    lossRate = atoi(argv[4]);
    if (argc == 6){
	if (atoi(argv[5]) == 1){
	    debug = 1;
	    printDebug("dubug set");
	}
    }   
    initializeGlobalWindow();
    initializeSenderWindow();
    startMessageReceived = 0;
    begin = 0;
    if(machineIndex == 1){
        begin = 1;
    }
    
    /* hard coded multicast address */
    //mcast_addr = 225 << 24 | 1 << 16 | 2 << 8 | 120; /* (225.1.2.120) */
    mcast_addr = 225 << 24 | 0 << 16 | 1 << 8 | 1; /* (225.0.1.1) */
    
    //debug = 0;
    processState = WAITING_START;
    /* END INITIALIZE */


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

    /* Prepare for execution */
    if(machineIndex == 1){

        //craft packets
        //craft token
    }
    
    else{
        //craft packets
    }


    

    printf("Waiting to receive start message\n");
    /* WAIT FOR START MESSAGE */
    for (;;)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

	temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask
			      , &timeout
			      //, NULL
			      );

        if (numReadyFDs > 0) {

	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
		printDebug("Received data");

                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

		startMessageReceived = processStartPacket(mess_buf, 
							  numBytesRead);
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


    printDebug("Received start message, continuing to event loop");


    if(machineIndex == 1){
        
        // send token
    }

    

    /* MAIN EVENT LOOP */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    for (;;)
    {
        if(begin){
  	    //send messages
	    //send token
	    begin = 0;
	}
	temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask, &timeout);

        if (numReadyFDs > 0) {
	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

		timeout.tv_usec = processPacket(mess_buf, numBytesRead);
	    }
	}

	else{
	    handleTimeout();
	}
    }

    return 0;
}
