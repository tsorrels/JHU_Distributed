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


#define NAME_LENGTH 80

receiver_state_type state;
connection * currentConnection;
packet_buffer * window_buffer;
FILE * currentFileHandle;
char * fileName = "file00";
int fileCounter;
int connectionSocketFD;
int retryCounter;


void closeConnection();
void initializeWindowBuffer();
void sendResponsePacket();
void handleTimeout();
void sendAckNak();



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
}

packet * buildAckNak()
{
    int i;
    int numNak = 0;
    ack_payload * payload;
    int nakArray [WINDOW_SIZE];
    packet * ackPacket;
    int consecutiveSeqNum = -1;
    int highestSeqNum = -1;
    int highestRecIndex = -1;

    /* traverse, find highest seq_num and corrosponding index*/
    for (i = 0 ; i < WINDOW_SIZE ; i ++)
    {
	if (window_buffer[i].received == 1 &&
	   (window_buffer[i].seq_num > highestSeqNum) ){
	    highestRecIndex = i;
	    highestSeqNum = window_buffer[i].seq_num;
	}	
    }

    /* check for anamoly */
    if (highestSeqNum == -1)
    {
	printf("ERROR: buildAckNak did not find packet in winwdow\n");
    }
     
    /* traverse, determine naks */
    for (i = 0 ; i < highestRecIndex ; i ++){
	if (window_buffer[i].received == 0){
	    nakArray[numNak] = window_buffer[i].seq_num;
	    numNak ++;
	}
    }

    ackPacket = malloc(sizeof(packet));
    ackPacket->header.type = ACK;
    payload = (ack_payload *) ackPacket->data;
    payload->ack = highestSeqNum;
    payload->num_nak = numNak;    

    memcpy(payload->naks, nakArray, sizeof(int) * numNak);
 
    return ackPacket;
}


void clearWindow()
{
    int i;
    int consecutiveSeqNum = -1;
    int consecutiveIndex = -1;
    int highestSeqNum = -1;

    /* traverse, find highest seq_num */
    for (i = 0 ; i < WINDOW_SIZE ; i ++){
	if (window_buffer[i].received == 1 &&
	   (window_buffer[i].seq_num > highestSeqNum) ){
	    highestSeqNum = window_buffer[i].seq_num;
	}	
    }

    /* traverse, find highest CONSECUTIVE seq_num */
    if (window_buffer[0].received == 1)
    {
	for (i = 0 ; i < highestSeqNum ; i ++)
	{
	    if (window_buffer[i].received == 1 ){
		consecutiveSeqNum = window_buffer[i].seq_num;

		/* track index of highest consecutive packet */
		consecutiveIndex = i;

		/* write the packet to the file */
		fwrite(window_buffer[i].packet_bytes.data, sizeof(char), 
		       PAYLOAD_SIZE, currentFileHandle);

		/* mark packet window as empty */
		window_buffer[i].received = 0;
	    }
	    else{
		/* break loop */
		break;
	    }
	}
    }
    
    /* else never got first seq_num in window */
	

    /* slide window */
    for (i = 0 ; i < WINDOW_SIZE ; i ++)
    {
	window_buffer[i].seq_num += consecutiveIndex;
	window_buffer[i].seq_num += 1;
    } 
}


void sendAckNak()
{
    int sizePacket;
    int sizePayload;
    ack_payload * payloadPointer;
    packet * ackPacket = buildAckNak();

    /* determine size of payload */
    payloadPointer = ackPacket->data;
    sizePayload = payloadPointer->num_nak * sizeof(int) + sizeof(int);
    
    sizePacket = sizePayload + sizeof(packet_header);

    /* send packet */
    sendto( connectionSocketFD, ackPacket, sizePacket, 0, 
	    (struct sockaddr *)&(currentConnection->socket_address), 
	    sizeof(currentConnection->socket_address) );

    free(ackPacket);
}

/* called to send either a FINACK, WAIT, or GO packet */
void sendResponsePacket(packet_type type, struct sockaddr_in sendSockAddr)
{
    packet_header * response_packet = NULL;
    int sendingSocketTemp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendingSocketTemp<0)
    {
	perror("Failed in creating socket to send WAIT; continuing\n");
    }
    response_packet = malloc(sizeof(packet_header));
    response_packet->type = type;
 
    /* send wait, address to whoever sent original packet */
    sendto(sendingSocketTemp, response_packet, sizeof(packet_header), 0, 
	   (struct sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
	
    /* close temporary socket */
    close(sendingSocketTemp);
    free(response_packet);
}


void handleTimeout()
{
    switch(state)
    {
    case WAITING_DATA:
	free(currentConnection);
	state = IDLE;
	break;

    case RECV_DATA:
	if (retryCounter == RECV_NUM_RETRY_ACK)
	{
	    closeConnection();
	}

	else
	{
	    /* resend ack/nak */
	    retryCounter ++;
	}
	break;
	
    default:
	break;

    }
}

void createNewConnection(struct sockaddr_in sendSockAddr)
{
    fileCounter ++;

    if (fileCounter > 99){
	printf("ERROR: cannot write to more than 99 files; require restart\n");
    }

    /* check whether a connection exists */
    if (currentConnection != NULL){
	printf("ERROR: trying to create a new connection but one exists\n");
    }

    /* store connection data */
    (currentConnection) = malloc(sizeof(connection));
    (currentConnection)->socket_address = sendSockAddr;
    (currentConnection)->status = 1;

    /* create sender socket */
    connectionSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (connectionSocketFD<0) {
	perror("ERROR: failed to create socket in new connection\n");
    }   

    /* create new string! */
    if(fileCounter % 10 == 0){
	fileName[5] = fileName[5] + 1;
	fileName[4] = '0';
    }

    else{
	fileName[5] = fileName[5] + 1;
    }

    /* check whether a file is already open */
    if (currentFileHandle != NULL)
    {
	printf("ERROR: trying to open file, but another file is open\n");
    }
    currentFileHandle = fopen(fileName, "w");   
}

void closeConnection()
{
    /* finish writing file */
    clearWindow();

    /* destroy connection */
    free(currentConnection);
    fclose(currentFileHandle);
    close(connectionSocketFD);
    state = IDLE;
}


/* return 1 if this filled the buffer */
int processDataPacket(packet * sentPacket, int numBytes)
{
    /* does what happen in processing the data packet dictate what happens with timer? */
    int startOfWindow = window_buffer[0].seq_num;    
    int windowIndex;
    int bufferFilled = 0;

    /* check if packet is in window */
    if (sentPacket->header.seq_num < startOfWindow ||
	sentPacket->header.seq_num > (startOfWindow + WINDOW_SIZE) )
    {
	/* discard */
	printf("received datapacket outside window \n");
    }

    /*  packet is in window */
    else
    {
	/* update state if needed */
	if (state != RECV_DATA){
	    state = RECV_DATA;
	}

	/* load packet */
	windowIndex = sentPacket->header.seq_num - startOfWindow;
	/* just double check the seq_num lines up with expected */
	if (window_buffer[windowIndex].seq_num != sentPacket->header.seq_num)
	{
	    printf("ERROR: data packet not being loaded correctly in window\n");
	}

	/* copy and mark as received */
	memcpy(&window_buffer[windowIndex], sentPacket, numBytes);
	window_buffer[windowIndex].received = 1;

	/* ack nak */
	if (windowIndex % (WINDOW_SIZE / NUM_INTERMITENT_ACK) == 0)
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
	createNewConnection(sendSockAddr);

	/* send GO and wait for response */
	//TODO: two sockets
	sendResponsePacket(GO, sendSockAddr); 
	state = WAITING_DATA;
	timer = recv_go_timer;
    }

    /* check if this is the active connection */
    else if ( currentConnection->socket_address.sin_addr.s_addr == 
	      sendSockAddr.sin_addr.s_addr){
	if (sentPacket->header.type == SYN && state == WAITING_DATA)
	{
	    /* something is a little out of synch, just resend GO */
	    sendResponsePacket(GO, sendSockAddr); 
	    timer = recv_go_timer;
	}
	else if (sentPacket->header.type == DATA)
	{
	    bufferFull = processDataPacket(sentPacket, numBytes);
	    /* maybe set timer here */
	    timer = recv_data_timer;
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
	printf("ERROR: reached a catch block indicating sender did not have a rule to process a packet received in this state\n");
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
    fileIterator = 1;
    fileCounter = 0;
    connectionSocketFD = -1;
    retryCounter = 0;
    initializeWindowBuffer();
    currentFileHandle = NULL;
    state = IDLE;

    return 0;

    /* set up receive socket */
    sr = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (sr<0) {
	perror("Ucast: socket");
	exit(1);
    }

    //name.sin_family = AF_INET; 
    //name.sin_addr.s_addr = INADDR_ANY; 
    //name.sin_port = htons(PORT);


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
    timeout.tv_sec = 0; /* timeout will never be more than a second */
    timeout.tv_usec = 10; /* initial timeout value */
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
	    handleTimeout();
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
