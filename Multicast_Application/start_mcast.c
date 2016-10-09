#include "mcast.h"
#include "net_include.h"

int main (int argc, char ** argv)
{
    struct sockaddr_in send_addr; // send address struct
    int                mcast_addr; // mcast address
    struct ip_mreq     mreq; //multicast required stuct
    unsigned char      ttl_val; // TTL to ensure packets stay in network
    int                sockSendMcast; // send MCast socket
    packet *           startPacket; // start packet send to mcast addr

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


    startPacket = malloc(sizeof(packet));
    startPacket->header.type = START;

    printf("Sending start packet\n");

    /* send start message to mcast address*/
    sendto(sockSendMcast, startPacket, sizeof(startPacket), 0, 
	   (struct sockaddr *) &send_addr, sizeof(send_addr));

    free(startPacket);


    printf("Exiting\n");


    return 0;
}
