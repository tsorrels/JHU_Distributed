#include "packet_header.h"

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

int main()
{
    struct sockaddr_in name;
    int                s;
    fd_set             mask;
    int                recv_s[10];
    int                valid[10];  
    fd_set             dummy_mask,temp_mask;
    int                i,j,num;
    int                mess_len;
    int                neto_len,flag=0,received=0;
    char               *mess_buf;
    long               on=1;
    long               total = 0,last_hun=0;
    FILE *f;
    struct timeval diff,now,end,start,hun;
    double time;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        perror("Net_server: socket");
        exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Net_server: bind");
        exit(1);
    }
 
    if (listen(s, 1) < 0) {
        perror("Net_server: listen");
        exit(1);
    }

    i = 0;
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(s,&mask);
    for(;;)
    {
        j=0;
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET(s,&temp_mask) ) {
                recv_s[j] = accept(s, 0, 0) ;
                FD_SET(recv_s[j], &mask);
                temp_mask = mask;
                valid[j] = 1;
                i=0;
            }
            for(;;)
            {
                if (valid[j]){
                if ( FD_ISSET(recv_s[j],&temp_mask) ) { 
                    if( recv(recv_s[j],&mess_len,sizeof(mess_len),0) > 0) {
                        neto_len = mess_len - sizeof(mess_len);
                        mess_buf = malloc(neto_len+1);
                        received = 0;
                        while(received<neto_len)
                            received += recv(recv_s[j], mess_buf+received, neto_len-received, 0 );
                        //printf("neto_length = %d, received = %d\n",neto_len,received);
                        if(i==0){
                            f = fopen(mess_buf,"w");
                            i++;
                            free(mess_buf);
                            gettimeofday(&start, NULL);
                            last_hun = 0;
                            gettimeofday(&hun, NULL);
                            continue;
                        }
                        fwrite(mess_buf,1,neto_len,f);
                        total += neto_len;
                        last_hun += neto_len;
                        if(last_hun>=HUN_MB){
                            printf("Total Mbytes transferred till now = %lf\n",(total*100.0)/HUN_MB);
                            gettimeofday(&now,NULL);
                            diff = diffTime(now,hun);
                            time = diff.tv_sec+ (diff.tv_usec)/1000000.0;
                            printf("Average transfer rate of last 100 Mbytes = %lfMbits/sec\n",(800/time));
                            hun = now;
                            last_hun -= HUN_MB;
                        }
                        free(mess_buf);
                    }
                    else
                    {
                        gettimeofday(&end, NULL);
                        diff = diffTime(end,start);
                        printf("Total time taken for transfer = %lf seconds\n",(diff.tv_sec+(diff.tv_usec)/1000000.0));
                        printf("Total Mbytes transferred = %lf\n",(total*100.0)/HUN_MB);
                        FD_CLR(recv_s[j], &mask);
                        close(recv_s[j]);
                        fclose(f);
                        valid[j] = 0;
                        flag=1;
                        break;
                    }
                }}
                else
                    break;
            }
        }
        if(flag==1)
            break;
    }

    return 0;

}

