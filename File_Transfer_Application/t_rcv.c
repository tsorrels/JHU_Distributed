#include "net_include.h"

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
    int                neto_len,flag=0;
    char               *mess_buf;
    long               on=1;
    FILE *f;

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
                valid[j] = 1;
                i=0;
            }
            for(;;)
            {   
                if (valid[j])    
                if ( FD_ISSET(recv_s[j],&temp_mask) ) {
                    if( recv(recv_s[j],&mess_len,sizeof(mess_len),0) > 0) {
                        neto_len = mess_len - sizeof(mess_len);
                        mess_buf = malloc(neto_len+1);
                        recv(recv_s[j], mess_buf, neto_len, 0 );
                        if(i==0){
                            mess_buf[neto_len] = '\0';
                            f = fopen(mess_buf,"w");
                            i++;
                            free(mess_buf);
                            continue;
                        }
                        fwrite(mess_buf,1,neto_len,f);
                        free(mess_buf);
                    }
                    else
                    {
                        printf("closing %d \n",j);
                        FD_CLR(recv_s[j], &mask);
                        close(recv_s[j]);
                        fclose(f);
                        valid[j] = 0;
                        flag=1;
                        break;
                    }
                }
            }
        }
        if(flag==1)
            break;
    }

    return 0;

}

