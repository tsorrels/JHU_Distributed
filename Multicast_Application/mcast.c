
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




void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}


void initializeGlobalWindow(){
    int i;
    globalWindow.window_start = 0;
    globalWindow.window_end = WINDOW_SIZE - 1;
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
        globalWindow.packets[i].received = 0;
	//globalWindow.packets[i].size = -1;
	//globalWindow.packets[i].seq_num = i;      
    }
    globalWindow.previous_ack = -1;
    globalWindow.has_token = 0;
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
    printf("%2d, %8d, %8d\n", deliveredPacket->header.proc_num,
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

  prevAck = min(globalWindow.previous_ack, tokenAck);

  for (i = globalWindow.window_start ; i <= prevAck ; i ++){
    deliverPacket(&globalWindow.packets[i].packet_bytes);
    globalWindow.packets[i].received = 0;

  }

  return i;  
}


void slideGlobalWindow(packet * token){

  int consecutiveAck;
  token_payload * tokenPayload;
  int clearedToSeqNum;

  tokenPayload = (token_payload *) token->data;

  /* clear window and deliver data */
  clearedToSeqNum = clearGlobalWindow(tokenPayload->ack);

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
      tokenPayload->nak[numNacks] = globalWindow.packets[i].packet_bytes.header.seq_num;
      numNacks ++;
    }  
  }
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
    packet * newToken;
    int highestSeqNumSent, newAck;

    tokenPayload = (token_payload *) recvdPacket->data;

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

    sendToken(newToken);

    /* we no longer call malloc in buildToken, nor free here */
    //free(newToken);
    
    craftPackets();

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


/*void processDataPacket(packet * recvdPacket){
    if (packetInWindow(recvdPacket)){
	receivePacket(recvdPacket);

    }
    else{
	printDebug("received data packet outside of window");
    }
}*/
 

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

	//clearGlobalWindow();

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
    if(globalWindow.has_token){
        sendToken(&senderWindow.previous_token);
    }
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
        int highestSeqNumSent;
        highestSeqNumSent = sendPackets(-1, -1); /* takes ack and seq_num from token */
        // send token
        sendToken(buildToken(highestSeqNumSent, highestSeqNumSent));
    }   

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

		timeout.tv_usec = processPacket(mess_buf, numBytesRead);
	    }
	}

	else{
	    handleTimeout();
	}
    }

    return 0;
}
