#include "packet_header.h"
#define NAME_LENGTH 80

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
    long total = 0,last_hun=0;
    struct timeval diff,now,end,start,hun;
    double time;
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
            for(int k=0;k<strlen(d_filename);k++){
            *(neto_mess_ptr+k) = *(d_filename+k);
            }
            mess_len = strlen(d_filename) + sizeof(mess_len);
        }
        else{
            bytes = fread(neto_mess_ptr,1,PAYLOAD_SIZE,f);
            if(bytes==0){
                free(mess_buf);
                gettimeofday(&end, NULL);
                diff = diffTime(end,start);
                printf("Total time taken for transfer = %lf seconds\n",(diff.tv_sec+(diff.tv_usec)/1000000.0));
                printf("Total Mbytes transferred = %lf\n",(total*100.0)/HUN_MB);
                break;
            }
            mess_len = bytes + sizeof(mess_len);
        }
        memcpy( mess_buf, &mess_len, sizeof(mess_len) );
        ret = 0;
        while(ret < mess_len)
            ret += send( s, mess_buf+ret, mess_len-ret, 0);
        //printf("mess_len = %d, ret = %d\n",mess_len,ret);
        if(i==0){
            gettimeofday(&start, NULL);
            last_hun = 0;
            gettimeofday(&hun, NULL);
            i++;
        }
        else{
            total += bytes;
            last_hun += bytes;
            if(last_hun>=HUN_MB){
                printf("Total Mbytes transferred till now = %lf\n",(total*100.0)/HUN_MB);
                gettimeofday(&now,NULL);
                diff = diffTime(now,hun);
                time = diff.tv_sec+ (diff.tv_usec)/1000000.0;
                printf("Average transfer rate of last 100 Mbytes = %lfMbits/sec\n",(800/time));
                hun = now;
                last_hun -= HUN_MB;
            }
        }
        /*if(ret != mess_len) 
        {
            free(mess_buf);
            perror( "Net_client: error in writing");
            exit(1);
        }*/
        free(mess_buf);
    }

    return 0;

}

