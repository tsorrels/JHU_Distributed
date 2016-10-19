
#include "mcast.h"


/* global variables, accessable throughout program */
connection mcastConnection;

sender_window senderWindow;
global_window globalWindow;
int debug;                          // This is to switch the debug prints on and off
process_state_type processState;
struct timeval diff,end,start;

int numPacketsToSend;
int machineIndex; 
int numProcesses;
int lossRate;

void sendPacket(int seq_num);
void resendNak(int seq_num);

struct timeval diffTime(struct timeval left, struct timeval right)
{
    struct timeval diff;

    diff.tv_sec  = left.tv_sec - right.tv_sec;
    diff.tv_usec = left.tv_usec - right.tv_usec;

    if (diff.tv_usec < 0) {
        diff.tv_usec += 1000000;
        diff.tv_sec--;
    }

    if (diff.tv_sec < 0) {
        printf("WARNING: diffTime has negative result, returning 0!\n");
        diff.tv_sec = diff.tv_usec = 0;
    }

    return diff;
}

/* This closes the connection after flushing the tailing output to the file
 * and printing out the total time taken for the transfer */
void closeConnection(){
  fflush(globalWindow.fd);
  gettimeofday(&end, NULL);
  diff = diffTime(end,start);
  printf("Total time taken for transfer = %lf seconds\n",(diff.tv_sec+(diff.tv_usec)/1000000.0));
  exit(0);
}

// Custom print method for debugging
void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}

/* This initializes the global_window members,
 * opens a file for writing and stores the 
 * machine's address in the my_ip member */
void initializeGlobalWindow(){
    struct hostent h_ent;
    struct hostent *p_h_ent;
    int i, host_num, max_len = 80;
    char fileName[] = "file_00";
    char my_name[max_len];
    
    fileName[6] = (machineIndex + 48);
    globalWindow.window_start = 0;
    globalWindow.window_end = WINDOW_SIZE - 1;
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
        globalWindow.packets[i].received = 0;    
    }

    globalWindow.previous_ack = -1;
    globalWindow.has_address = 0;
    globalWindow.sendto_ip = 0;
    
    gethostname(my_name, max_len);
    p_h_ent = gethostbyname(my_name);
    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );
    
    if(debug)
        printf("Host name is %s and host_ip is %d\n",my_name,htonl(host_num));
    globalWindow.my_ip = host_num;
    globalWindow.fd = fopen(fileName, "w");
    if (globalWindow.fd == NULL){
      perror("fopen failed!");
    }

}

// This initializes the sender_window members
void initializeSenderWindow(){
    token_payload *payload;
    senderWindow.window_start = 0;
    senderWindow.window_end = 0;
    senderWindow.num_built_packets = 0;
    senderWindow.num_sent_packets = 0;
    payload = (token_payload *)senderWindow.previous_token.data;
    payload->address = -1;

}

/* places packet into global window */ 
void processDataPacket(packet * recvdPacket){
  int seq_num;

  seq_num = recvdPacket->header.seq_num;
    
  // check to see if packet is in window
  if (seq_num < globalWindow.window_start ||
      seq_num > globalWindow.window_end){
    if (debug)
      printf("received packet outside window seq=%d\n", seq_num);
  }

  else{
    // copy packet into global window
    memcpy(&(globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes),
	   recvdPacket, sizeof(packet));
    globalWindow.packets[seq_num % WINDOW_SIZE].received = 1;
    if (debug)
    printf("Received seq_num %d stored at index %d\n", 
        globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes.header.seq_num, seq_num % WINDOW_SIZE);

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

/* Writes packet to the file */
void deliverPacket(packet * deliveredPacket){
    fprintf(globalWindow.fd,"%2d, %8d, %8d\n",deliveredPacket->header.proc_num,
	    deliveredPacket->header.seq_num,
	    deliveredPacket->header.rand_num);
}

/* delivers packet to the file present in global buffer upto min of the ack received on the token
 * and the ack previously loaded on the token; returns the seq_num + 1 of
 * the packets wrote, which is the new start of the global window */
int clearGlobalWindow(int tokenAck){

  int i;
  int prevAck;

  prevAck = min(globalWindow.previous_ack, tokenAck);
 
  for (i = globalWindow.window_start ; i <= prevAck ; i ++){
    if(debug)
      printf("clearing seq_num = %d\n",
	     globalWindow.packets[i %WINDOW_SIZE].packet_bytes.header.seq_num);
    deliverPacket(&globalWindow.packets[i % WINDOW_SIZE].packet_bytes);
    printDebug("delivered packet");
    globalWindow.packets[i % WINDOW_SIZE].received = 0;

  }

  if (debug)
    printf("sliding global window, new start = %d\n", i);

  return i;  
}

/* Clears the window upto the sequence num that is received by all the processes
 * and shifts the window_start accordingly */
void slideGlobalWindow(packet * token){

  token_payload * tokenPayload;
  int clearedToSeqNum;

  tokenPayload = (token_payload *) token->data;

  printDebug("sliding window");
  /* clear window and deliver data */
  clearedToSeqNum = clearGlobalWindow(tokenPayload->ack);
  
  /* update window */
  globalWindow.window_start = clearedToSeqNum;
  globalWindow.window_end = globalWindow.window_start + WINDOW_SIZE - 1;
  /* -1 because window_end identifies the packet with the highest 
   * sequence number that will fit in the window */  
}

/* Iterates over all the nacks in the token and calls resendNak for resending each Nack */
void resendNaks(packet * recvdPacket){
  int i;
  token_payload * payload;
  int nackValue;
  
  payload = (token_payload *)recvdPacket->data;
  
  for(i = 0 ; i < payload->num_nak ; i ++){
    nackValue = payload->nak[i];
    if(globalWindow.packets[nackValue % WINDOW_SIZE].received == 1){
      if(debug)
	printf("calling resend nak %d\n",nackValue);

      resendNak(nackValue);
    }
  }    
}

// Multicasts the Nack
void resendNak(int seq_num){
    packet * sendingPacket;

    if (debug)
      printf("resending nak %d\n", seq_num);
    sendingPacket = &globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes;
    
    //multicast the nack
    sendto(mcastConnection.fd, sendingPacket, sizeof(packet), 0,
	 (struct sockaddr*) &mcastConnection.send_addr,
	   sizeof(mcastConnection.send_addr));
}

/* pass in sequence number to stamp on packet, and send packet */
void sendPacket(int seq_num){

    // stamp sequence number
    packet * sendingPacket;
    
    sendingPacket = &(senderWindow.packets[senderWindow.window_start]);
    sendingPacket->header.seq_num = seq_num;
  
    //multicast the packet
    sendto(mcastConnection.fd, sendingPacket, sizeof(packet), 0,
	 (struct sockaddr*) &mcastConnection.send_addr,
	   sizeof(mcastConnection.send_addr));

    // load into global window
    memcpy(&(globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes),
	   sendingPacket, sizeof(packet));
    if (debug)
      printf("sending packet seq_num =  %d\n", globalWindow.packets[seq_num % WINDOW_SIZE].packet_bytes.header.seq_num);
    globalWindow.packets[seq_num % WINDOW_SIZE].received = 1;

    senderWindow.window_start = (senderWindow.window_start + 1)
      % SEND_WINDOW_SIZE ;

    senderWindow.num_sent_packets ++;
}

/* takes ARU/cumulative ack from token 
 * sends until the MAX_MESSAGE limit is reached, or
 * globalWindow's window_end is reached, or
 * limit of total number of built packets is reached
 * returns sequence number of last packet sent*/
int sendPackets(int ack, int seq_num){
    int i;
    i = seq_num + 1;
    while (
	   i <= globalWindow.window_end &&
	   i <= seq_num + MAX_MESSAGE &&
	   senderWindow.num_sent_packets < senderWindow.num_built_packets){
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
 * has already built its prescribed number of packets */
void craftPackets(){

    do{
        if (senderWindow.num_built_packets == numPacketsToSend){
  	    printDebug("built max packets");
	    break;
        }
	buildPacket(&senderWindow.packets[senderWindow.num_built_packets %
					 SEND_WINDOW_SIZE]);
        senderWindow.window_end =
	  (senderWindow.window_end + 1) % SEND_WINDOW_SIZE;
	senderWindow.num_built_packets ++;
    }
    while (senderWindow.window_end != (senderWindow.window_start +
	   SEND_WINDOW_SIZE)%SEND_WINDOW_SIZE);
  
}

/* Calculates the new ack that the process will write to the token
 * based on the local consecutiveAck, current tokenAck and current token seq_num */
int getNewAck(int tokenAck, int seq_num){

  int consecutiveAck;
  int newAck;

  newAck = tokenAck;
  consecutiveAck = getConsecutiveAck();

  if (consecutiveAck < tokenAck){
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
  
  for (i = consecutiveAck + 1 ; i <= seqNum && numNacks < MAX_NAK; i ++){
    if (globalWindow.packets[i%WINDOW_SIZE].received != 1){
      if(debug)
	printf("adding nak = %d\n", i);
      tokenPayload->nak[numNacks] = i;
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
packet * buildToken(int highestSeqNumSent, int newAck, int rand_num){

    token_payload * newPayload;
    packet * newToken;
    int consecutiveAck;
    consecutiveAck = getConsecutiveAck();

    newToken = &senderWindow.previous_token;
    newPayload = (token_payload *)newToken->data;
    newPayload->seq_num = highestSeqNumSent;
    newPayload->ack = newAck;

    globalWindow.previous_ack = newPayload->ack;

    addNacks(newPayload, consecutiveAck, highestSeqNumSent);

    newToken->header.type = TOKEN;

    // rand_num field inside the token is used to discard the rebroadcasted duplicate tokens
    if(rand_num == -1)
        newToken->header.rand_num = 1;
    else
        newToken->header.rand_num = rand_num + 1;
    newPayload->address = machineIndex % numProcesses + 1;          

    if (debug)
      printf("Built token with seq = %d, ack = %d, address = %d\n",
	     newPayload->seq_num,
	     newPayload->ack,
	     newPayload->address);
    
    return newToken;
}

// Sends the token over the network
void sendToken(packet * token, int has_address){
    int i;
    struct sockaddr_in send_addr;
    if(!has_address){
        //multicast token
        printDebug("Multicasting token");
        send_addr = mcastConnection.send_addr;
    }
    else{
        //unicast token
      
        printDebug("Unicasting token");
	if (debug)
	  printf("Unicasting token to %d\n", globalWindow.sendto_ip);
        send_addr.sin_family = AF_INET;
        send_addr.sin_addr.s_addr = globalWindow.sendto_ip; 
        send_addr.sin_port = htons(PORT);
    }
    // sends the token NUM_TOKEN_RESEND times to reduce the chances of token drop
    for (i = 0 ; i < NUM_TOKEN_RESEND ; i ++){
        sendto(mcastConnection.fd, token, sizeof(packet), 0,
	       (struct sockaddr *)&send_addr, sizeof(send_addr));
    }
}


/* this method does a LOT,  but the idea is to add a bunch of 
   functionality in this one method and then just be able to 
   change the entry point into the function; this minimizes
   rewriting stuff when we swtich token operations to unicast */
void processToken(packet * recvdPacket){
    token_payload * tokenPayload;
    token_payload * newTokenPayload;
    packet * newToken;
    int highestSeqNumSent, newAck, prev_ack, k;
    
    prev_ack = globalWindow.previous_ack;

    tokenPayload = (token_payload *) recvdPacket->data;
    if (debug)
        printf("Received token addressed to %d, ack = %d, num_nacks = %d, seq_num = %d, num_shutdown = %d\n", 
        tokenPayload->address, tokenPayload->ack, tokenPayload->num_nak,
	     tokenPayload->seq_num, tokenPayload->num_shutdown);

    // Stores the address of the next machine if present in the token
    if((!globalWindow.has_address) &&
       (tokenPayload->host_num[(machineIndex%numProcesses)])){
          globalWindow.sendto_ip =
	    tokenPayload->host_num[(machineIndex%numProcesses)];
	  globalWindow.has_address = 1;
	  if(debug){
              printf("Received the address for unicast %d\n",
		     globalWindow.sendto_ip);
	  }
    }

    // discards the token if its not addressed to this machine
    if (tokenPayload->address != machineIndex ){
        printDebug("received token not addressed to this processes");
	return;
    }

    // discards the token if it receives a duplicate token
    else if (recvdPacket->header.rand_num <=
	     senderWindow.previous_token.header.rand_num &&
	     tokenPayload->address == machineIndex ){
        printDebug("received rebroadcasted duplicate token");
	return;
    }
    
    resendNaks(recvdPacket);

    slideGlobalWindow(recvdPacket);
    
    highestSeqNumSent = sendPackets(tokenPayload->ack, tokenPayload->seq_num);
    
    newAck = getNewAck(tokenPayload->ack, tokenPayload->seq_num);
    
    newToken = buildToken(highestSeqNumSent, newAck, recvdPacket->header.rand_num);

    newTokenPayload = (token_payload *)newToken->data;

    // Copies the addresses of machines that this machine has, in the token
    for(k = 0;k < numProcesses;k++){
        if(tokenPayload->host_num[k])
            newTokenPayload->host_num[k] = tokenPayload->host_num[k];
        if(debug){
            printf("%d ",newTokenPayload->host_num[k]);
        }
    }
    
    // Copies its own address in the token
    newTokenPayload->host_num[machineIndex-1] = globalWindow.my_ip;
    if(debug){
        printf("%d\n",newTokenPayload->host_num[machineIndex-1]);
    }

    if(debug){
        printf("Number of sent packets = %d, Number of packets to send = %d\n",senderWindow.num_sent_packets,numPacketsToSend);
    }

    /* if done sending messages and delivered all messages till the highest 
     * seq num, transition to FINISHED and mark token */
    if (senderWindow.num_sent_packets == numPacketsToSend &&
        processState != FINISHED &&
        globalWindow.window_start == (tokenPayload->seq_num+1)){
        processState = FINISHED;
	newTokenPayload->num_shutdown = tokenPayload->num_shutdown + 1;
        printDebug("Process state is FINISHED");
        sendToken(newToken, globalWindow.has_address);
    }

    /* if all the processes are done sending and all processes have delivered
     * packets till the highest seq num in the token then multicast FIN packet */
    else if(tokenPayload -> num_shutdown == numProcesses &&
	    prev_ack == tokenPayload->seq_num){

        /* change token to a FIN packet */
        newToken->header.type = FIN;
        printDebug("Sending FIN token");
        /* multicasts the FIN packet 50 times to reduce the chances of packet loss */
        for(k = 0;k < 50;k++)
            sendToken(newToken, 0);
    }
    else{
        sendToken(newToken, globalWindow.has_address);
    }
	
    if (processState != FINISHED){
        craftPackets();
    }
}
    
/* only called when state is waiting for start packet */
int processStartPacket(char * messageBuffer, int numBytes){
    packet *packetPtr, *token;
    int returnValue, highestSeqNumSent;
    token_payload *payload;
    
    printDebug("Processing packet in processStartPacket");
  
    packetPtr = (packet*) messageBuffer;
    returnValue = 0;
    gettimeofday(&start, NULL);
    if (numBytes == sizeof(packet) && packetPtr->header.type == START){
        recv_dbg_init( lossRate, machineIndex ); 
        returnValue = 1;
    }
    if (processState == WAITING_START &&
	packetPtr->header.type == START){
        processState = AWAITING_TOKEN;
        printDebug("received start packet");

        // If this is the first machine then it starts the communication
        if(machineIndex == 1){
          printDebug("process index = 1 building first token");
          /* takes ack and seq_num from token */
          highestSeqNumSent = sendPackets(-1, -1);
          token = buildToken(highestSeqNumSent, highestSeqNumSent, -1);
          payload = (token_payload *)token;
          payload->host_num[machineIndex-1] = globalWindow.my_ip;
          sendToken(token, globalWindow.has_address);
          printDebug("transitioning to TOKEN_SENT");
          processState = TOKEN_SENT;
          
        }
    }

    return returnValue;
}


/* Processes the packet based on the header type and 
 * returns what value of timer will be in next select call */
int processPacket(char * messageBuffer, int numBytes){
    packet * recvdPacket;
    int timerValue;
    timerValue = 1000; /* default of resetting timer to 10000usec */
    recvdPacket = (packet*)messageBuffer;
    
    if(recvdPacket->header.type == DATA){
      if(processState != FINISHED){
	processState = AWAITING_TOKEN;
      }
      processDataPacket(recvdPacket);
      timerValue = AWAITING_TOKEN_TIMER;
    }
    
    else if(recvdPacket->header.type == TOKEN){
	processToken(recvdPacket);
	if(processState != FINISHED)
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

// Resends the previous token in case when timeout fires
void handleTimeout(){

  perror("handleTimeout fired");
    token_payload *payload;
    payload = (token_payload *)senderWindow.previous_token.data;
    if(payload->address != -1)
        sendToken(&senderWindow.previous_token, globalWindow.has_address);
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

    srand (time(NULL));

    mcast_addr = MCAST_ADDR;

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
    mcastConnection.fd = sockSendMcast;

    memcpy(&mcastConnection.send_addr, &send_addr, sizeof(struct sockaddr_in));

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
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

	temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask
			      , &timeout
			      );

    if (numReadyFDs > 0) {

	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {
                numBytesRead = recv( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );

        if(numBytesRead > 0)
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


    /* MAIN EVENT LOOP */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    for (;;)
    {

        temp_mask = mask;
	numReadyFDs = select( FD_SETSIZE, &temp_mask, &dummy_mask, 
			      &dummy_mask, &timeout);

        if (numReadyFDs > 0) {
	    if ( FD_ISSET( sockRecvMcast, &temp_mask) ) {

                numBytesRead = recv_dbg( sockRecvMcast, mess_buf, 
				     sizeof(packet), 0 );
            if(numBytesRead > 0)
                timeout.tv_usec = processPacket(mess_buf, numBytesRead);
	    }
	}

	else{
	    handleTimeout();
	}
    }

    return 0;
}
