/* rcv.c
 * runs in a server capacity to receive a reliable transfer of a file sent by
 * a client process.  
 *
 * Usage: ./rcv.c <loss_rate_percent>
 *
 * Accepts unlimited simultaneous  connections, but can queue transfers
 * Sends client a message of transfer is queued
 */


#include "net_include.h"
#include "packet_header.h"


#define NAME_LENGTH 80

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
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    char                  input_buf[80];
    struct timeval        timeout;

    int lossRate = atoi(argv[1]);

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
	
    }


    /* set up sending socket */ 
    ss = socket(AF_INET, SOCK_DGRAM, 0); /* socket for sending (udp) */
    if (ss<0) {
	perror("Ucast: socket");
	exit(1);
    }
    

    return 0;
}
