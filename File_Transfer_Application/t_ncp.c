#include "packet_header.h"
#define NAME_LENGTH 80

int main(int argc,char *argv[])
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    int                s;
    int                ret;
    int                mess_len;
    char               *mess_buf;
    char               *neto_mess_ptr; 
    char * s_filename;
    char * host_name;
    char * d_filename;
    FILE *f;
    int i=0,bytes;
    //int lossRate;
    
    s_filename = malloc(NAME_LENGTH);

    d_filename = malloc(NAME_LENGTH);

    host_name = malloc(NAME_LENGTH);

    //lossRate = atoi(argv[1]);

    s_filename = argv[1];

    d_filename = strtok(argv[2],"@");
    host_name = strtok(NULL,"@");

    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    p_h_ent = gethostbyname(host_name);
    if ( p_h_ent == NULL ) {
        printf("net_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
    memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );

    ret = connect(s, (struct sockaddr *)&host, sizeof(host) ); /* Connect! */
    if( ret < 0)
    {
        perror( "Net_client: could not connect to server"); 
        exit(1);
    }
    f = fopen(s_filename,"r");
    for(;;)
    {
        mess_buf = malloc(MAX_MESS_LEN);
        neto_mess_ptr = mess_buf+sizeof(mess_len);
        if(i==0){
            neto_mess_ptr = d_filename;
            mess_len = strlen(d_filename) + sizeof(mess_len);
            i++;
        }
        else{
        bytes = fread(neto_mess_ptr,1,PAYLOAD_SIZE,f);
        if(bytes==0){
            free(mess_buf);
            break;
        }
        mess_len = bytes + sizeof(mess_len);
        }
        memcpy( mess_buf, &mess_len, sizeof(mess_len) );

        ret = send( s, mess_buf, mess_len, 0);
        if(ret != mess_len) 
        {
            free(mess_buf);
            perror( "Net_client: error in writing");
            exit(1);
        }
        free(mess_buf);
    }

    return 0;

}

