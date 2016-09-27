/* rcv.c
 * runs in a server capacity to receive a reliable transfer of a file sent by
 * a client process.  
 *
 * Usage: ./rcv.c <loss_rate_percent>
 *
 * Accepts unlimited simultaneous  connections, but can queue transfers
 * Sends client a message of transfer is queued
 */


#include "packet_header.h"
#include "sendto_dbg.h"
#include <math.h>

#define NAME_LENGTH 80

receiver_state_type state;
connection * currentConnection;
packet_buffer * window_buffer;
int window_min_seq;
int window_max_seq;
FILE * currentFileHandle;
char * fileName = "file00";
int fileCounter;
int connectionSocketFD;
int retryCounter;
int debug;
struct timeval timeval1;
struct timeval timeval2;
int totalBytesWritten = 0;
int intervalBytesWritten = 0;
int statSizeInterval;


void closeConnection();
void initializeWindowBuffer();
void sendResponsePacket();
void handleTimeout();
void sendAckNak();
struct timeval diffTime(struct timeval left, struct timeval right);



void checkStatistics(){

    struct timeval diff;
    double time;
    
    if (intervalBytesWritten >= statSizeInterval){
        gettimeofday(&timeval2, NULL);
	diff = diffTime(timeval2, timeval1);
	time = diff.tv_sec + (diff.tv_usec) / 1000000.0;	
	printf("Total bytes received: %d\n", totalBytesWritten);
	printf("Average transfer rate of last 100MB = %f Mbits/s\n",
	       (800/time));
	       
	intervalBytesWritten = 0;

	gettimeofday(&timeval1, NULL);
  }
}


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



int cmpfunc (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}


void initializeWindowBuffer()
{
    int i;
    /* create window buffer */
    window_buffer = malloc(WINDOW_SIZE * sizeof(packet_buffer));
        /* initialize buffer */
    for( i = 0 ; i < WINDOW_SIZE ; i ++)
    {
	window_buffer[i].received = 0;
	window_buffer[i].seq_num = i;
	/* omit initializing buffer for data */

    }  

    window_min_seq = 0;
    window_max_seq= WINDOW_SIZE - 1;
}

ack_packet * buildAckNak()
{
    int i;
    int numNak = 0;
    int nakArray [WINDOW_SIZE];
    ack_packet * ackPacket;
    //int consecutiveSeqNum = -1;
    int highestSeqNum = -1;
    int highestRecIndex = -1;

    if (debug == 1)
        printf("Building ack/nak packet\n");
    
    /* traverse, find highest seq_num and corrosponding index*/
    for (i = 0 ; i < WINDOW_SIZE ; i ++)
    {
	if (window_buffer[i].received == 1 &&
	   (window_buffer[i].seq_num > highestSeqNum) ){
	    highestRecIndex = i;
	    highestSeqNum = window_buffer[i].seq_num;
	}	
    }

    if (debug == 1)
      printf("highest received index = %d\n", highestRecIndex);

    
    /* check for anamoly */
    if (highestSeqNum == -1 && debug == 1)
    {
	perror("ATTN: buildAckNak did not find packet in winwdow\n");
    }
     
    /* traverse, determine naks */
    
    
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
	if (window_buffer[i].received == 0 &&
	    window_buffer[i].seq_num < highestSeqNum){
	    nakArray[numNak] = window_buffer[i].seq_num;
	    numNak ++;
	}
    }


    
    if (debug == 1)
        printf("loop 2 ran\n");

    
    ackPacket = malloc(sizeof(ack_packet));
    ackPacket->header.type = ACK;

    if (highestSeqNum != -1){
      ackPacket->ack = highestSeqNum;
    }
    else{
      ackPacket->ack = window_min_seq - 1;
    }
    
    ackPacket->num_nak = numNak;



    qsort(nakArray, numNak, sizeof(int), cmpfunc);
    
    memcpy((ackPacket->naks), nakArray, sizeof(int) * numNak);

    if (debug == 1)
        printf("returning from ack build\n");

    
    return ackPacket;
}


void clearWindow()
{
    int i;
    //int j;
    int consecutiveSeqNum = -1;
    //int consecutiveIndex = -1;
    //int highestIndex = -1;
    int highestSeqNum = -1;
    int numBytesWritten;
    int min_seq_index = -1;
    int max_seq_index = -1;
    
    if (debug == 1)
	printf("Clearing window:\n");

    /* traverse, find highest seq_num */
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
	if (window_buffer[i].received == 1 &&
	   (window_buffer[i].seq_num > highestSeqNum) ){
	    highestSeqNum = window_buffer[i].seq_num;
	    //highestIndex = i;
	}
    }

    if (debug == 1)
	printf("Highest sequence number in window = %d\n", highestSeqNum);


    /* traverse, find highest CONSECUTIVE seq_num */
    min_seq_index = window_min_seq % WINDOW_SIZE;
    max_seq_index = (min_seq_index + WINDOW_SIZE - 1) % WINDOW_SIZE;
    if (window_buffer[min_seq_index].received == 1)
    {
        i = min_seq_index;
	do
	{
	    if (window_buffer[i].received == 1 ){
		consecutiveSeqNum = window_buffer[i].seq_num;

		/* track index of highest consecutive packet */
		//consecutiveIndex = i;

		if (debug == 1)
		    printf("Writing packet with seq_num %d from index %d\n",
			   consecutiveSeqNum, i);
		
		/* write the packet to the file */
		numBytesWritten = fwrite(window_buffer[i].packet_bytes.data,
					 sizeof(char),
					 window_buffer[i].length,
					 currentFileHandle);

		totalBytesWritten += numBytesWritten;
		intervalBytesWritten += numBytesWritten;
		
		/* mark packet window as empty */
		window_buffer[i].received = 0;

		/* update next expected seq num at this index */
		window_buffer[i].seq_num += WINDOW_SIZE;
		
		
	    }
	    else{
		/* break loop */
		break;
	    }
	    i = (i + 1) % WINDOW_SIZE;
	} while (i != (max_seq_index + 1) % WINDOW_SIZE);
    }
    
    /* else never got first seq_num in window */


    if (debug == 1)
	printf("Highest consequitive seq number = %d\n",consecutiveSeqNum);


    /* slide window */

    if (consecutiveSeqNum != -1){
        window_min_seq = consecutiveSeqNum + 1;
	window_max_seq = window_min_seq + WINDOW_SIZE - 1;
    }

    if (debug == 1)
	printf("Sliding base of window to seq_num %d\n", 
	    window_min_seq);

    checkStatistics();

}


void sendAckNak()
{
    int i;

    ack_packet * packet = buildAckNak();
    

    if (debug == 1){
	printf("Sending ack = %d, num_nak = %d\n", packet->ack,
	       packet->num_nak);

	for (i = 0 ; i < packet->num_nak ; i ++){
	  printf("nak %d\n", (packet->naks)[i] );
	}
    }


    
    /* send packet */
    sendto_dbg( connectionSocketFD, (const char*) packet, sizeof(ack_packet),
		0, (struct sockaddr *)&(currentConnection->socket_address), 
	    sizeof(currentConnection->socket_address) );

    free(packet);
}

/* called to send either a FINACK, WAIT, or GO packet */
void sendResponsePacket(packet_type type, struct sockaddr_in sendSockAddr)
{

    packet_header * response_packet;
    int sendingSocketTemp;
    struct sockaddr_in toAddress;
    
    if (debug == 1){
      printf("sending response packet\n");
    }
    
    response_packet = NULL;
    sendingSocketTemp = socket(AF_INET, SOCK_DGRAM, 0);

    if (debug == 1)
	printf("Sending response type %d to address %d\n", type, 
	       sendSockAddr.sin_addr.s_addr);



    if (sendingSocketTemp<0)
    {
	perror("Failed in creating socket to send WAIT; continuing\n");
    }
    response_packet = malloc(sizeof(packet_header));
    response_packet->type = type;

    toAddress.sin_family = AF_INET;
    toAddress.sin_addr.s_addr = sendSockAddr.sin_addr.s_addr;
    toAddress.sin_port = htons(PORT);
    
    /* send wait, address to whoever sent original packet */
    sendto_dbg(sendingSocketTemp, (const char* )response_packet,
	       sizeof(packet_header), 0, 
	   (struct sockaddr *)&toAddress, sizeof(toAddress));
	
    /* close temporary socket */
    close(sendingSocketTemp);
    free(response_packet);
}


void handleTimeout()
{
    if (debug == 1)
	printf("In timeout, state = %d\n", state);
    switch(state)
    {
    case WAITING_DATA:
	/* free(currentConnection); */
	closeConnection();
	state = IDLE;
	break;

    case WAIT_RESPONSE:
        if (debug == 1)
  	    printf("retry counter = %d\n", retryCounter);

	sendAckNak();
	retryCounter ++;

	break;
	    
    case RECV_DATA:
        if (retryCounter == RECV_NUM_RETRY_ACK)
	{
	    closeConnection();
	    state = IDLE;
	}

	else
	{
	    sendAckNak();
     	    clearWindow();
	    state = WAIT_RESPONSE;
	}
	break;
	
    default:
	break;

    }
}

void createNewConnection(packet * sentPacket, struct sockaddr_in sendSockAddr)
{
    struct sockaddr_in toAddress;
    char * fileName;

    initializeWindowBuffer();
    
    fileCounter ++;

    if (debug ==1 )
	printf("creating new connection with %d\n",
	    sendSockAddr.sin_addr.s_addr);

	
    fileName = sentPacket->data;


    /* this had better have shown up with a null terminator */
    if (debug ==1 )
        printf("Preparing to receive %s\n",
	    fileName);


    /* check whether a connection exists */
    if (currentConnection != NULL){
	perror("ERROR: trying to create a new connection but one exists\n");
    }

    toAddress.sin_family = AF_INET;
    toAddress.sin_addr.s_addr = sendSockAddr.sin_addr.s_addr;
    toAddress.sin_port = htons(PORT);

        
    /* store connection data */
    (currentConnection) = malloc(sizeof(connection));
    (currentConnection)->socket_address = toAddress;
    (currentConnection)->status = 1;
    gettimeofday( &(currentConnection->startTime), NULL);
    totalBytesWritten = 0;

    
    
    /* create sender socket */
    connectionSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (connectionSocketFD<0) {
	perror("ERROR: failed to create socket in new connection\n");
    }   


    /* check whether a file is already open */
    if (currentFileHandle != NULL)
    {
	perror("ERROR: trying to open file, but another file is open\n");
    }
    currentFileHandle = fopen(fileName, "w");
    if (currentFileHandle == NULL){
        perror("ERROR: could not open file for writing:");
	perror(fileName);
    }
}

void closeConnection()
{
    struct timeval now;
    struct timeval diff;
    double time;
    if (debug == 1)
	printf("closing connection\n");


    gettimeofday(&now, NULL);
    diff = diffTime(now, currentConnection->startTime);

    time = diff.tv_sec + (diff.tv_usec) / 1000000.0;	

    
    
    /* finish writing file */
    clearWindow();

    printf("Transfered %f MB in %f seconds\n",
	   (totalBytesWritten * 100.0 / HUN_MB ), time);
    printf("Transfered %d MB in %f seconds\n",
	   (totalBytesWritten ), time);

    
    /* destroy connection */
    free(window_buffer);
    free(currentConnection);
    currentConnection = NULL;
    fclose(currentFileHandle);
    currentFileHandle = NULL;
    close(connectionSocketFD);    
    state = IDLE;
    totalBytesWritten = 0;
}


/* return 1 if this filled the buffer */
int processDataPacket(packet * sentPacket, int numBytes)
{
    int windowIndex;
    int bufferFilled = 0;
    
    /* check if packet is in window */
    if ( (sentPacket->header.seq_num < window_min_seq) ||
	 (sentPacket->header.seq_num > (window_max_seq)) )
    {
	/* discard */
	if (debug == 1)
	    printf("received datapacket outside window \n");
    }

    /*  packet is in window */
    else
    {
        if (state == WAITING_DATA){
    	    /* this is the first data packet */
	    gettimeofday(&timeval1, NULL);
	}

	/* update state and reset ack/nak counter if needed */

	if (state != RECV_DATA){
	    state = RECV_DATA;
	    retryCounter = 0;
	}

	/* load packet */
	windowIndex = sentPacket->header.seq_num % WINDOW_SIZE;
	/* just double check the seq_num lines up with expected */
	if (window_buffer[windowIndex].seq_num != sentPacket->header.seq_num&&
	    debug ==1)
	{
	    printf("ATTN:data packet not being loaded correctly in window\n");
	}

	if (debug == 1)
	    printf("Loading packet seq_num %d into buffer at index %d\n",
		   sentPacket->header.seq_num, windowIndex);
	
	/* copy and mark as received */
	memcpy(&(window_buffer[windowIndex].packet_bytes),
	       sentPacket, numBytes);
	window_buffer[windowIndex].received = 1;
	window_buffer[windowIndex].length = numBytes - sizeof(packet_header);



	if (sentPacket->header.seq_num == window_max_seq)
	{
	    sendAckNak();
	    clearWindow(); /* this writes data and slides window */
	}
    }
    return bufferFilled; /* this will determine which timeout value to use */
}


int processPacket(char * mess_buf, int numBytes, struct sockaddr_in sendSockAddr)
{

    packet * sentPacket = (packet *)mess_buf;
    int timer = 10; /* 10 usec default */

    if (debug == 1)
	printf("received packet type %d seq %d\n", sentPacket->header.type,
	       sentPacket->header.seq_num);

    
    /* check if FIN */
    if(sentPacket->header.type == FIN)
    {
	/* check state and if there is a connection */
	if ( (state == RECV_DATA || state == WAITING_DATA ||
	      state == WAIT_RESPONSE) &&
	     currentConnection != NULL) {
	    /* check if the FIN is coming from current connection */
	    if (currentConnection->socket_address.sin_addr.s_addr == 
		sendSockAddr.sin_addr.s_addr ) {
		closeConnection();
	    }  
	}
	sendResponsePacket(FINACK, sendSockAddr); 
    }

    /* check if SYN and there is no active connection */
    else if(state == IDLE && sentPacket->header.type == SYN)
    {
        createNewConnection(sentPacket, sendSockAddr);

	/* send GO and wait for response */
	//TODO: two sockets
	sendResponsePacket(GO, sendSockAddr); 
	state = WAITING_DATA;
	timer = recv_data_timer;
    }

    /* check if this is the active connection */
    else if ( currentConnection != NULL){
      if ( currentConnection->socket_address.sin_addr.s_addr == 
	   sendSockAddr.sin_addr.s_addr){
	  if (sentPacket->header.type == DATA)
	  {
	      processDataPacket(sentPacket, numBytes);
	      /* maybe set timer here */
	      timer = recv_window_timer;
	  }
      }
    }

    /* if this is not active connection, and packet is data type */
    else if(sentPacket->header.type == DATA)
    {
	sendResponsePacket(FINACK, sendSockAddr); 
    }

    /* send wait */
    else if (state != IDLE && sentPacket->header.type == SYN)
    {
	sendResponsePacket(WAIT, sendSockAddr);
    }

    else
    {
	perror("ERROR: reached a catch block indicating sender did not have a rule to process a packet received in this state\n");
    }

    return timer;
}


int main (int argc, char** argv)
{    
    /* socket declarations */

    struct sockaddr_in    name;    
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   numBytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    struct timeval        timeout;

    
    int lossRate = atoi(argv[1]);
    printf("Loss rate set to %d\n", lossRate);

    /* initialize */
    sendto_dbg_init(lossRate);
    fileCounter = 0;
    connectionSocketFD = -1;
    retryCounter = 0;
    initializeWindowBuffer();
    currentFileHandle = NULL;
    state = IDLE;
    debug = 0 ;
    statSizeInterval = HUN_MB;
    
    /* set up receive socket */
    sr = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (sr<0) {
	perror("Ucast: socket");
	exit(1);
    }

    name.sin_family = AF_INET; 
    name.sin_addr.s_addr = INADDR_ANY; 
    name.sin_port = htons(PORT);


    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
	perror("Ucast: bind");
	exit(1);
    }
    
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    

    /* event loop */   
    timeout.tv_sec = 1; /* timeout will never be more than a second */
    timeout.tv_usec = 0; /* initial timeout value */

    printf("Initialized: ready to receive\n");

    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) /* there is a FD that has data to read */
	{
            if ( FD_ISSET( sr, &temp_mask) ) /* data ready from socket */
	    {
                from_len = sizeof(from_addr);
                numBytes = recvfrom( sr, mess_buf, MAX_MESS_LEN, 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len );
		/* process packet, return new timeout value */	
		timeout.tv_usec = processPacket(mess_buf, numBytes, from_addr);

            }
        } 
	else 
	{ /* timer fired */
	    if (debug == 1)
		printf("timeout fired\n");
	    handleTimeout();
	    timeout.tv_sec = 1; /* timeout will never be more than a second */
	    timeout.tv_usec = 0; /* initial timeout value */
	    
        }	
    }

    return 0;
}
