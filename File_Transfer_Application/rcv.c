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
int retryCounter = 0;


void closeConnection();
void initializeWindowBuffer();
void sendResponsePacket();
void handleTimeout();
void processDataPacket();

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


void closeConnection()
{
    /* finish writig file */
    
    /* destroy connection */
    free(currentConnection);
    state = IDLE;
}


void processDataPacket()
{

}


int processPacket(char * mess_buf, int numBytes, int currentFD, struct sockaddr_in sendSockAddr)
{
    struct timeval        timeout;

    fd_set                mask;
    fd_set                dummy_mask,temp_mask;

    packet * sentPacket = (packet *)mess_buf;
    int sendingSocketTemp;
    packet * wait_packet;
    packet_type * type;
    int timer = 10; /* 10 usec default */
    
    /* check if FIN */
    if(sentPacket->header.type == FIN)
    {
	if (state == RECV_DATA || state == WAITING_DATA)
	{
	    closeConnection();
	}
	sendResponsePacket(FINACK, sendSockAddr); 
    }

    /* check if SYN and there is no active connection */
    else if(state == IDLE && sentPacket->header.type == SYN)
    {
	/* store connection data */
	(currentConnection) = malloc(sizeof(connection));
	(currentConnection)->socket_address = sendSockAddr;
	(currentConnection)->status = 1;

	/* send GO and wait for response */
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
	    processDataPacket();
	    /* maybe set timer here */
	    timer = recv_data_timer;
	}
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

    return;
}


int main (int argc, char** argv)
{    
    /* socket declarations */

    struct sockaddr_in    name;    
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   numBytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    char                  input_buf[80];
    struct timeval        timeout;

    
    int lossRate = atoi(argv[1]);
    int fileIterator = 1;
    int currentFD;

    initializeWindowBuffer();

    state = IDLE;

    return 0;

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
    
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    */

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
		timeout.tv_usec = processPacket(mess_buf, numBytes, currentFD, from_addr);

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
    ss = socket(AF_INET, SOCK_DGRAM, 0); /* socket for sending (udp) */
    if (ss<0) {
	perror("Ucast: socket");
	exit(1);
    }
    
    return 0;
}
