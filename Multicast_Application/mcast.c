
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





void sendPacket(int seq_num);

void closeConnection(){

}



void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}


void initializeGlobalWindow(){
    int i;
    char fileName[] = "file_00";
    fileName[6] = (machineIndex + 65);
    globalWindow.window_start = 0;
    globalWindow.window_end = WINDOW_SIZE - 1;
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
        globalWindow.packets[i].received = 0;
	//globalWindow.packets[i].size = -1;
	//globalWindow.packets[i].seq_num = i;      
    }
    globalWindow.previous_ack = -1;
    globalWindow.has_token = 0;
    globalWindow.fd = fopen(machineIndex + 65, "w");
    if (globalWindow.fd < 0){
      perror("fopen failed!");
    }

}

void initializeSenderWindow(){
  //int i;
    senderWindow.window_start = 0;
    senderWindow.window_end = 0;
    senderWindow.num_built_packets = 0;
    senderWindow.num_sent_packets = 0;

}


/* check to see if packet is in window, return 1 for true */
/*int packetInWindow(packet * recvdPacket){


    return 0;
}*/

/* places packet into global window */ 
void processDataPacket(packet * recvdPacket){
  int seq_num;

  seq_num = recvdPacket->header.seq_num;
    // check to see if in window

  if (debug)
    printf("Received seq_num %d\n", seq_num);


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

int min (int left, int right){
  if (left <= right){
    return left;
  }

  else{
    return right;
  }
}



void deliverPacket(packet * deliveredPacket){
  
  if(debug){
  //fprintf(globalWindow.fd,"%2d, %8d, %8d\n",deliveredPacket->header.proc_num,
    printf("%2d, %8d, %8d\n",deliveredPacket->header.proc_num,
	   deliveredPacket->header.seq_num, 
	   deliveredPacket->header.rand_num);

  }
}

/* delivers packet in global buffer up min of the ack received on the token
 * and the ack previously loaded on the token; returns the seq_num + 1 of
 * the packets wrote, which is the new start of the global window */
int clearGlobalWindow(int tokenAck){

  int i;
  int prevAck;

  printDebug("clearing global window");
  prevAck = min(globalWindow.previous_ack, tokenAck);
  printDebug("min returned");

  
  for (i = globalWindow.window_start ; i <= prevAck ; i ++){
    if(debug)
      printf("clearing seq_num = %d\n",
	     globalWindow.packets[i %WINDOW_SIZE].packet_bytes.header.seq_num);
    deliverPacket(&globalWindow.packets[i % WINDOW_SIZE].packet_bytes);
    printDebug("delivered packet");
    globalWindow.packets[i % WINDOW_SIZE].received = 0;

  }

  printDebug("returning from clearGlobalWindow");

  return i;  
}


void slideGlobalWindow(packet * token){

  int consecutiveAck;
  token_payload * tokenPayload;
  int clearedToSeqNum;

  tokenPayload = (token_payload *) token->data;

  printDebug("sliding window");
  /* clear window and deliver data */
  clearedToSeqNum = clearGlobalWindow(tokenPayload->ack);

  printDebug("cleared global window");

  
  /* update window */
  globalWindow.window_start = clearedToSeqNum;
  globalWindow.window_end = globalWindow.window_start + WINDOW_SIZE - 1;
  /* -1 because window_end identifies the packet with the highest 
   * sequence number that will fit in the window */

  //consecutiveAck = getConsecutiveAck();  
}


void resendNaks(packet * recvdPacket){
  int i;
  token_payload * payload;
  int nackValue;
  
  payload = recvdPacket->data;
  
  for(i = 0 ; i < payload->num_nak ; i ++){
    nackValue = payload->nak[i];
    if(globalWindow.packets[nackValue % WINDOW_SIZE].received == 1){
      if(debug)
	printf("calling resend nak %d\n",nackValue);

      resendNak(nackValue);
    }
  }    
}

void resendNak(int seq_num){
    packet * sendingPacket;

    if (debug)
      printf("resending nak %d\n", seq_num);
    sendingPacket = &globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes;
    
    //send
    sendto(mcastConnection.fd, sendingPacket, sizeof(packet), 0,
	 (struct sockaddr*) &mcastConnection.send_addr,
	   sizeof(mcastConnection.send_addr));
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
 * sends up to the group cumulative ack + MAX_MESSAGE 
 * returns sequence number of last packet sent*/
int sendPackets(int ack, int seq_num){
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

    
    return i -1;
}

int newRandomNumber(){


  return rand() % MAX_RAND + 1;
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
    while (senderWindow.window_end != (senderWindow.window_start +
	   SEND_WINDOW_SIZE)%SEND_WINDOW_SIZE);
  
}

int getNewAck(int tokenAck, int seq_num){

  int consecutiveAck;
  int newAck;

  newAck = tokenAck;
  consecutiveAck = getConsecutiveAck();

  if (consecutiveAck < tokenAck){
    //globalWindow.has_lowered = 1;
    newAck = consecutiveAck;
  }

  else if (tokenAck == seq_num){
    newAck = consecutiveAck;
  }

  else if (tokenAck == globalWindow.previous_ack){
    newAck = consecutiveAck;    
  }

  else{
    newAck = tokenAck;
  }
  /* else return tokenAck; do not update it */  
  if (debug)
    printf("getNewAck returning %d\n", newAck);
  return newAck;
}


/* this method loads the token payload with nacks; it does not re-hang the 
 * nacks that it received on the token */
void addNacks(token_payload * tokenPayload, int consecutiveAck, int seqNum)
{
  int i;
  int numNacks;

  numNacks = 0;
  
  for (i = consecutiveAck + 1 ; i <= seqNum ; i ++){
    if (globalWindow.packets[i].received != 1){
      if(debug)
	printf("adding nak = %d\n",
	       globalWindow.packets[i].packet_bytes.header.seq_num);
      tokenPayload->nak[numNacks] =
	globalWindow.packets[i].packet_bytes.header.seq_num;
      numNacks ++;
    }  
  }
  if (debug)
    printf("added %d naks\n", numNacks);
  tokenPayload->num_nak = numNacks;
}


/* builds token and overwrites previous_token in senderWindow 
 * returns reference to senderWindow.previous_token, which this method
 * overwrites with the new token it is about to send */
packet * buildToken(int highestSeqNumSent, int newAck){

    /*token_payload * tokenPayload;
    
    tokenPayload = (token_payload *) token->data;*/
    token_payload * newPayload;
    packet * newToken;
    int consecutiveAck;
    consecutiveAck = getConsecutiveAck();
    
    //newToken = malloc(sizeof(packet));
    newToken = &senderWindow.previous_token;
    newPayload = newToken->data;
    newPayload->seq_num = highestSeqNumSent;
    //newPayload->ack = getNewAck(tokenPayload->ack, tokenPayload->seq_num);
    newPayload->ack = newAck;

    globalWindow.previous_ack = newPayload->ack;

    addNacks(newPayload, consecutiveAck, highestSeqNumSent);

    newToken->header.type = TOKEN;
    newToken->header.rand_num = newRandomNumber();
    newPayload->address = machineIndex % numProcesses + 1;
    globalWindow.has_token = 1;           

    if (debug)
      printf("Built token with seq = %d, ack = %d, address = %d\n",
	     newPayload->seq_num,
	     newPayload->ack,
	     newPayload->address);
    
    return newToken;
}

void sendToken(packet * token){
    sendto(mcastConnection.fd, token, sizeof(packet), 0,
	 (struct sockaddr*) &mcastConnection.send_addr,
	   sizeof(mcastConnection.send_addr)); 
}


/* this method does a LOT,  but the idea is to add a bunch of 
   functionality in this one method and then just be able to 
   change the entry point into the function; this minimizes
   rewriting stuff when we swtich token operations to unicast */
void processToken(packet * recvdPacket){
    token_payload * tokenPayload;
    token_payload * newTokenPayload;
    packet * newToken;
    int highestSeqNumSent, newAck;

    tokenPayload = (token_payload *) recvdPacket->data;
    if (debug)
      printf("Received token addressed to %d\n", tokenPayload->address);
    
    if (tokenPayload->address != machineIndex ){
        printDebug("received token not addressed to this processes");
	return;
    }

    else if (recvdPacket->header.rand_num ==
	     senderWindow.previous_token.header.rand_num &&
	     tokenPayload->address == machineIndex ){
        printDebug("received rebroadcasted duplicate token");
	return;
    }
    
    newAck = getNewAck(tokenPayload->ack, tokenPayload->seq_num);
    
    resendNaks(recvdPacket);

    slideGlobalWindow(recvdPacket);
    
    highestSeqNumSent = sendPackets(tokenPayload->ack, tokenPayload->seq_num);
    
    newToken = buildToken(highestSeqNumSent, newAck);

    newTokenPayload = newToken->data;

    /* if done sending messages, transition to FINISHED and mark token */
    if (senderWindow.num_sent_packets == numPacketsToSend &&
        processState != FINISHED){
        processState = FINISHED;
	newTokenPayload->num_shutdown = tokenPayload->num_shutdown + 1;
        //craftPackets();


    }
    
    else if(tokenPayload -> num_shutdown == numProcesses &&
	    globalWindow.previous_ack == tokenPayload->seq_num){

        /* change token to a FIN packet */
        newToken->header.type = FIN;    
    }

    
    /* we no longer call malloc in buildToken, nor free here */
    //free(newToken);
    sendToken(newToken);
	
    if (processState != FINISHED){
        craftPackets();
    }
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


/* returnes what value of timer will be in next select call */
int processPacket(char * messageBuffer, int numBytes){
    packet * recvdPacket;
    int timerValue;
    int clearWindow;
    int highestSeqNumSent;
    
    clearWindow = 0; /* not sure how we will set this, prolly with a funccal */
    timerValue = 1000000; /* default of resetting timer to 10000usec */
    recvdPacket = (packet*)messageBuffer;


    if (processState == WAITING_START &&
	recvdPacket->header.type == START){
        processState = AWAITING_TOKEN;
	printDebug("received start packet");
	if(machineIndex == 1){
	  printDebug("process index = 1 building first token");
	  /* takes ack and seq_num from token */
	  highestSeqNumSent = sendPackets(-1, -1); 
	  sendToken(buildToken(highestSeqNumSent, highestSeqNumSent));
	  printDebug("transitioning to TOKEN_SENT");
	  processState = TOKEN_SENT;
	  
	}   
    }

    
    else if(recvdPacket->header.type == DATA){
      //printDebug("Received data packet from mcast socket");
	processState = AWAITING_TOKEN;
	processDataPacket(recvdPacket);
	timerValue = AWAITING_TOKEN_TIMER;
	//clearGlobalWindow();

    }
    
    else if(recvdPacket->header.type == TOKEN){
	printDebug("Received token packet from mcast socket");

	/* this method does a LOT,  but the idea is to add a bunch of 
	   functionality in this one method and then just be able to 
	   change the entry point into the function; this minimizes
	   rewriting stuff when we swtich token operations to unicast */
	processToken(recvdPacket);
	processState = TOKEN_SENT;
	timerValue = TOKEN_RESEND_TIMER;
    }

    else if(recvdPacket->header.type == FIN){
      printDebug("received FIN");
         closeConnection();
    }

    else{
	printDebug("Received non- packet from mcast socket");

    }

    
    return timerValue;
}

void handleTimeout(){

  //printDebug("handleTimeout fired");
  if (processState == TOKEN_SENT){
    // resend token
        printDebug("timeout fired, resending token");
	sendToken(&senderWindow.previous_token);
    }


    else if (processState == AWAITING_TOKEN){
    // resend token
        printDebug("timeout fired, AWAITING_TOKEN");
    }
    
    //if(globalWindow.has_token){
    //sendToken(&senderWindow.previous_token);
	//}
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
    
    int startMessageReceived;// indicates whether processes recvd start message
    int highestSeqNumSent; // used for first process and input into first tok

    
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
    srand (time(NULL));
    
    /* hard coded multicast address */
    mcast_addr = 225 << 24 | 1 << 16 | 2 << 8 | 120; /* (225.1.2.120) */
    //mcast_addr = 225 << 24 | 0 << 16 | 1 << 8 | 1; /* (225.0.1.1) */
    
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
    //mcastConnection.fd=socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
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
    mcastConnection.fd = sockSendMcast;
    memcpy(&mcastConnection.send_addr, &send_addr, sizeof(struct sockaddr_in));
    //mcastConnection.send_addr = (sock_addr) send_addr;
    /******* END SET UP MCAST SEND SOCKET ********/


    if (debug)
      printf("mcastConnection.fd = %d\n", mcastConnection.fd);
    if (debug)
      printf("sockSendMcast = %d\n", sockSendMcast);

    /******* SET UP FDs FOR SELECT ********/
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sockRecvMcast, &mask );

    /* Prepare for execution */
    craftPackets();  

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
	      //printDebug("Received data");

                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

		//startMessageReceived = processStartPacket(mess_buf, 
		//					  numBytesRead);
		timeout.tv_usec = processPacket(mess_buf, numBytesRead);

		//if (startMessageReceived == 1){
		//    break;
		//}
	    }
	}

	else{
	  //printf(".");
	  // fflush(0);
	  handleTimeout();
	}
    }

    printDebug("Received start message, continuing to event loop");


    /* MAIN EVENT LOOP */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    for (;;)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask, &timeout);

        if (numReadyFDs > 0) {
	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
  	        printDebug("from event loop: socket received data ");
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
