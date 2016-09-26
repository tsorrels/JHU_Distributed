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


void closeConnection();
void initializeWindowBuffer();
void sendResponsePacket();
void handleTimeout();
void sendAckNak();




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
    int consecutiveSeqNum = -1;
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
      printf("loop 1 ran, highest received index = %d\n", highestRecIndex);

    
    /* check for anamoly */
    if (highestSeqNum == -1)
    {
	perror("ERROR: buildAckNak did not find packet in winwdow\n");
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
    /*
    payload = (ack_payload *) (ackPacket->data);
    payload->ack = highestSeqNum;
    payload->num_nak = numNak;    
    */

    ackPacket->ack = highestSeqNum;
    ackPacket->num_nak = numNak;
        
    if (debug == 1)
      printf("about to run memcpy, numNak = %d\n", numNak);

    /*
    printf("nakArray is at %p\n", nakArray);
    printf("nakArray is at %d\n", nakArray[0]);
    printf("payload pointer is poiting to %p\n", payload);
    printf("packet pointer is pointing to %p\n", ackPacket);
    printf("payload naks is pointing to %p\n", payload->naks);
    */    


    qsort(nakArray, numNak, sizeof(int), cmpfunc);
    
    memcpy((ackPacket->naks), nakArray, sizeof(int) * numNak);

    if (debug == 1)
        printf("returning from ack build\n");

    
    return ackPacket;
}


void clearWindow()
{
    int i;
    int j;
    int consecutiveSeqNum = -1;
    int consecutiveIndex = -1;
    int highestIndex = -1;
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
	    highestIndex = i;
	}
    }

    /* traverse, find highest seq_num */
    /*
    i = window_min_seq;
    j = WINDOW_SIZE - 1;
    if (i == 0){
        j = i - 1;
    }
    while (i != (j -1) ){
	if (window_buffer[i].received == 1 &&
	   (window_buffer[i].seq_num > highestSeqNum) ){
	    highestSeqNum = window_buffer[i].seq_num;
	    highestIndex = i;
	}
	i = (i + 1) % WINDOW_SIZE;
    }
    */
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
		consecutiveIndex = i;

		if (debug == 1)
		    printf("Writing packet with seq_num %d from index %d\n",
			   consecutiveSeqNum, i);
		
		/* write the packet to the file */
		numBytesWritten = fwrite(window_buffer[i].packet_bytes.data,
					 sizeof(char),
					 window_buffer[i].length,
					 currentFileHandle);

		/*
		if (debug == 1)
		printf("Wrote %d bytes\n", numBytesWritten); */
		
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
    /*
    min_seq_index = window_min_seq % WINDOW_SIZE;
    max_seq_index = (min_seq_index + WINDOW_SIZE - 1) % WINDOW_SIZE;
    */
   
    for (i = window_min_seq ; i <= consecutiveIndex ; i ++)
    {
        /*
	window_buffer[i].seq_num += WINDOW_SIZE;
	window_buffer[i].seq_num += 1;*/
    } 

    if (debug == 1)
	printf("Sliding base of window to seq_num %d\n", 
	    window_min_seq);
}


void sendAckNak()
{
    int sizePacket;
    int sizePayload;
    int i;
/*
    ack_payload * payloadPointer;
    
    packet * ackPacket = buildAckNak(); 
    */

    ack_packet * packet = buildAckNak();
    
    /* determine size of payload 
    payloadPointer = (ack_payload *)  (ackPacket->data);
    sizePayload = payloadPointer->num_nak * sizeof(int) + sizeof(int)
      + sizeof(int);
    
    sizePacket = sizePayload + sizeof(packet_header);

    */

    if (debug == 1){
	printf("Sending ack = %d, num_nak = %d\n", packet->ack,
	       packet->num_nak);

	for (i = 0 ; i < packet->num_nak ; i ++){
	  printf("nak %d\n", (packet->naks)[i] );
	}
    }


    
    /* send packet */
    sendto_dbg( connectionSocketFD, packet, sizeof(ack_packet), 0, 
	    (struct sockaddr *)&(currentConnection->socket_address), 
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
    sendto_dbg(sendingSocketTemp, response_packet, sizeof(packet_header), 0, 
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

    fileCounter ++;

    if (debug ==1 )
	printf("creating new connection with %d\n",
	    sendSockAddr.sin_addr.s_addr);

	
    fileName = sentPacket->data;


    /* this had better have shown up with a null terminator */
    if (debug ==1 )
        printf("Preparing to receive %s\n",
	    fileName);

    /*
    if (fileCounter > 99){
	perror("ERROR: cannot write to more than 99 files; require restart\n");
	} */

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
    if (debug == 1)
	printf("closing connection\n");


    /* finish writing file */
    clearWindow();

    /* destroy connection */
    free(currentConnection);
    fclose(currentFileHandle);
    currentFileHandle = NULL;
    close(connectionSocketFD);    
    state = IDLE;
}


/* return 1 if this filled the buffer */
int processDataPacket(packet * sentPacket, int numBytes)
{
    /* does what happen in processing the data packet dictate what happens with timer? */
    int startOfWindow;     
    int windowIndex;
    int bufferFilled = 0;

    startOfWindow = window_min_seq;    

    /*
    if (debug == 1)
      printf("Window starts at seq_num = %d, size = %d\n",
	     startOfWindow, numBytes);
    */
    
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
	/* update state and retry ack/nak counter if needed */
	if (state != RECV_DATA){
	    state = RECV_DATA;
	    retryCounter = 0;
	}

	/* load packet */
	windowIndex = sentPacket->header.seq_num % WINDOW_SIZE;
	/* just double check the seq_num lines up with expected */
	if (window_buffer[windowIndex].seq_num != sentPacket->header.seq_num)
	{
	    perror("ERROR:data packet not being loaded correctly in window\n");
	}

	if (debug == 1)
	    printf("Loading packet seq_num %d into buffer at index %d\n",
		   sentPacket->header.seq_num, windowIndex);
	
	/* copy and mark as received */
	memcpy(&(window_buffer[windowIndex].packet_bytes),
	       sentPacket, numBytes);
	window_buffer[windowIndex].received = 1;
	window_buffer[windowIndex].length = numBytes - sizeof(packet_header);


	/* ack nak */
	/*
	if (( windowIndex % (WINDOW_SIZE / NUM_INTERMITENT_ACK) == 0) &&
	windowIndex != 0 ) */

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
    //struct timeval        timeout;

    //fd_set                mask;
    //fd_set                dummy_mask,temp_mask;

    packet * sentPacket = (packet *)mess_buf;
    //int sendingSocketTemp;
    //packet * wait_packet;
    //packet_type * type;
    int timer = 10; /* 10 usec default */
    int bufferFull = 0;

    if (debug == 1)
	printf("received packet type %d seq %d\n", sentPacket->header.type,
	       sentPacket->header.seq_num);

    
    /* check if FIN */
    if(sentPacket->header.type == FIN)
    {
	/* check state and if there is a connection */
	if ( (state == RECV_DATA || state == WAITING_DATA) &&
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
    else if ( currentConnection->socket_address.sin_addr.s_addr == 
	      sendSockAddr.sin_addr.s_addr){
	if (sentPacket->header.type == SYN && state == WAITING_DATA)
	{
	    /* something is a little out of synch, just resend GO */
	    sendResponsePacket(GO, sendSockAddr); 
	    timer = recv_data_timer;
	}
	else if (sentPacket->header.type == DATA)
	{
	    bufferFull = processDataPacket(sentPacket, numBytes);
	    /* maybe set timer here */
	    timer = recv_window_timer;
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
    //struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    //struct hostent        h_ent;
    //struct hostent        *p_h_ent;
    //char                  host_name[NAME_LENGTH] = {'\0'};
    //char                  my_name[NAME_LENGTH] = {'\0'};
    //int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   numBytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    //char                  input_buf[80];
    struct timeval        timeout;

    
    int lossRate = atoi(argv[1]);
    int fileIterator;
    int currentFD;

    printf("Loss rate set to %d\n", lossRate);

    /* initialize */
    sendto_dbg_init(lossRate);
    fileIterator = 1;
    fileCounter = 0;
    connectionSocketFD = -1;
    retryCounter = 0;
    initializeWindowBuffer();
    currentFileHandle = NULL;
    state = IDLE;
    debug = 1;

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

 
    /* build sender socket */
    /*
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);
    */
    
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
		/* DO NOT NEED NULL BYTE 
		   mess_buf[bytes] = 0; */
                from_ip = from_addr.sin_addr.s_addr;

		/* process packet, return new timeout value */	
		timeout.tv_usec = processPacket(mess_buf, numBytes, from_addr);

/*
                printf( "Received from (%d.%d.%d.%d): %s\n", 
                                (htonl(from_ip) & 0xff000000)>>24,
                                (htonl(from_ip) & 0x00ff0000)>>16,
                                (htonl(from_ip) & 0x0000ff00)>>8,
                                (htonl(from_ip) & 0x000000ff),
                                mess_buf );

*/
            }

	    /* REMOVE STDIN FROM EVENT FD IN EVENT LOOP 
	    else if( FD_ISSET(0, &temp_mask) ) {
                bytes = read( 0, input_buf, sizeof(input_buf) );
                input_buf[bytes] = 0;
                printf( "There is an input: %s\n", input_buf );
                sendto( ss, input_buf, strlen(input_buf), 0, 
                    (struct sockaddr *)&send_addr, sizeof(send_addr) );
 		    }*/
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


    /* set up sending socket */ 
/*
    ss = socket(AF_INET, SOCK_DGRAM, 0);
    if (ss<0) {
	perror("Ucast: socket");
	exit(1);
    }
  */  
    return 0;
}
