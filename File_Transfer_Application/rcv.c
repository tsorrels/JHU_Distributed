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



packet_buffer * initializeWindowBuffer()
{
    int i;

    /* create window buffer */
    packet_buffer * window_buffer = malloc(WINDOW_SIZE * sizeof(packet_buffer));
    /* initialize buffer */
    for( i = 0 ; i < WINDOW_SIZE ; i ++)
    {
	window_buffer[i].received = 0;
	window_buffer[i].seq_num = i;
	/* omit initializing buffer for data */
    }  
    return window_buffer;
}


void processPacket(packet_buffer ** window_buffer, char * mess_buf, int numBytes, int currentFD, connection * currentConnection, struct sockaddr_in sockAddr)
{
    int sendingSocketTemp;
    packet * wait_packet;
    /* check connection */
    if( currentConnection == NULL)
    {
	/* process possible new connection */
    }


    /* else, a connection is alreay active */
    else if ( (currentConnection->socket_address).sin_addr.s_addr == 
	      sockAddr.sin_addr.s_addr){
	

    }

    else
    {
	/* create new socket */
	sendingSocketTemp = socket(AF_INET, SOCK_DGRAM, 0);
	if (ss<0)
	{
	    perror("Failed in creating socket to send WAIT; continuing\n");
	}
	wait_packet * = malloc(sizeof(packet_header));
	wait_packet->type = WAIT;

	sendto( sendintSocketTemp, wait_packet, sizeof(packet_header), 0, 
		(struct sockaddr *)&send_addr, sizeof(send_addr) );
	


	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = host_num; 
	send_addr.sin_port = htons(PORT);



	/* send wait */

	/* destroy socket */


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

    connection * currentConnection = NULL;
    packet_buffer * window_buffer = initializeWindowBuffer();

    printf("initialized window_buffer; seq_num for packet buffer 5 is %d\n",
	   window_buffer[5].seq_num);


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

 
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);
    
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );

    /* event loop */   
    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) /* there is a FD that has data to read */
	{
            if ( FD_ISSET( sr, &temp_mask) ) /* data ready from socket */
	    {
                from_len = sizeof(from_addr);
                numBytes = recvfrom( sr, mess_buf, MAX_MESS_LEN, 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len );
		/* DO NOT NEED BULL BYTE 
		   mess_buf[bytes] = 0; */
                from_ip = from_addr.sin_addr.s_addr;

		/* process packet */	
		processPacket(&window_buffer, mess_buf, numBytes, currentFD,
				  currentConnection, from_addr);

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
        } else { /* timer fired */
            printf(".");
            fflush(0);
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
