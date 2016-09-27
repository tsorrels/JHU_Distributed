/* ncp.c
 * runs as a client to send a reliable transfer of a file to a host running
 * rcv.c
 *
 * Usage: ./rcv.c <loss_rate_percent> <source_file_name> <dest_file_name> @
 *                   <compe_name>
 *
 * 
 */
#include "packet_header.h"
#include "sendto_dbg.h"
#define NAME_LENGTH 80



fd_set mask;
fd_set dummy_mask;
int host_num;
int ss;
int sr;
int debug;
struct sockaddr_in    send_addr;




ack_packet * check(uint timer);
void sender(int lossRate, char * s_filename, char * d_filename);
void establish_conn(char *filename);

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

int main(int argc, char ** argv)
{

    char * s_filename;
    char * host_name;
    char * d_filename;
    int lossRate;
    struct sockaddr_in    name;
    struct hostent        *p_h_ent;
    struct hostent        h_ent;
    
    debug = 1;
    if(debug==1)
        printf("Starting sender\n");


    s_filename = malloc(NAME_LENGTH);

    d_filename = malloc(NAME_LENGTH);

    host_name = malloc(NAME_LENGTH);

    lossRate = atoi(argv[1]);

    s_filename = argv[2];

    d_filename = strtok(argv[3],"@");
    host_name = strtok(NULL,"@");

    if(debug ==1)
        printf("LossRate = %d, source_filename = %s, destination_filename = %s, hostname = %s\n",lossRate, s_filename, d_filename,host_name);
    
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
 
    ss = socket(AF_INET, SOCK_DGRAM, 0); /* socket for sending (udp) */
    if (ss<0) {
        perror("Ucast: socket");
        exit(1);
    }
    
    p_h_ent = gethostbyname(host_name);
    if ( p_h_ent == NULL ) {
        printf("Ucast: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    sendto_dbg_init(lossRate);
    sender(lossRate,s_filename,d_filename);
    return 0;
}
void sender(int lossRate, char *s_filename, char *d_filename)
{
    int                   i,j,h,read,ack;
    int                   last_seq;
    int start_seq;
    int hasnacks,hasfreed = 0;
    int sent_fin,max_value=0;
    int                   size[WINDOW_SIZE],recvd[WINDOW_SIZE];
    char                  **buffer;
    packet                *packets[WINDOW_SIZE];
    ack_packet           *payload;
    uint                  timer;
    FILE *f;    
    int total,begin,new_start;
    long total_bytes = 0,last_hun=0;
    struct timeval diff,now,end,start,hun;
    double time;


    timer=sender_data_timer;    

    last_seq=0;
    start_seq=0;
    new_start = WINDOW_SIZE;
    hasnacks=0;
    sent_fin=0;
    begin = 0;
    ack=WINDOW_SIZE-1;


    if(debug==1)
        printf("Entered sender\n");
    
    f = fopen(s_filename,"r");
    buffer = (char **)malloc(sizeof(char *)*WINDOW_SIZE);
    buffer[0] = (char *)malloc(sizeof(char)*WINDOW_SIZE*PAYLOAD_SIZE);
    if(debug==1)
        printf("Entered sender\n");
    for(i=0;i<WINDOW_SIZE;i++){
        buffer[i] = (*buffer + PAYLOAD_SIZE*i);
        packets[i] = malloc(sizeof(packet));
    }
    if(debug==1)
        printf("Establishing connection\n");
    establish_conn(d_filename);
    total=WINDOW_SIZE;
    gettimeofday(&start, NULL);
    last_hun = 0;
    gettimeofday(&hun, NULL);
    while(1){
        if((sent_fin!=1)&&(begin!=0)&&(new_start>0)){
            total_bytes = (new_start-1)*PAYLOAD_SIZE;
            if((total_bytes-last_hun)>=HUN_MB){
                printf("Total Mbytes transferred till now = %lf\n",(total_bytes*100.0)/HUN_MB);
                gettimeofday(&now,NULL);
                diff = diffTime(now,hun);
                time = diff.tv_sec+ (diff.tv_usec)/1000000.0;
                printf("Average transfer rate of last 100 Mbytes = %lfMbits/sec\n",(800/time));
                hun = now;
                last_hun = total_bytes;
            }
        }
        if(hasnacks==1){
            for(int k=start_seq;k<(max_value);k++){
                if(recvd[k%WINDOW_SIZE]==0){
                    sendto_dbg( ss, (char *)packets[k%WINDOW_SIZE], sizeof(packet_header)+size[k%WINDOW_SIZE], 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                    if(debug==1)
                    printf("Resending packet =%d at index=%d\n",k,k%WINDOW_SIZE);
                }
            }
            timer = sender_data_timer;
        }
        if((sent_fin!=1)&&(start_seq<new_start)){
            total = new_start-start_seq;
            if(begin==0)
                last_seq = 0;
            else
                last_seq = max_value;
            if(debug==1)
                    printf("Reading %d packets, starting from = %d\n",(new_start-start_seq),last_seq);
            for(j=0;j<total;j++){
                read = fread(buffer[(last_seq+j)%WINDOW_SIZE],1,PAYLOAD_SIZE,f);
                size[(last_seq+j)%WINDOW_SIZE]=read;
                recvd[(last_seq+j)%WINDOW_SIZE] = 1;
                if(read==0)
                    break;
            }
            if(j>0){
                if(debug==1)
                        printf("Sending %d packets\n",j);
                for(i=0;i<j;i++){
                    for(h=0;h<size[(last_seq+i)%WINDOW_SIZE];h++)
                        packets[(last_seq+i)%WINDOW_SIZE]->data[h] = buffer[(last_seq+i)%WINDOW_SIZE][h];
                    if(debug==1)
                        printf("Sending sequence num=%d stored at index=%d\n",(i+last_seq),(last_seq+i)%WINDOW_SIZE);
                    packets[(last_seq+i)%WINDOW_SIZE]->header.seq_num=i+last_seq;
                    packets[(last_seq+i)%WINDOW_SIZE]->header.type=DATA;
                    sendto_dbg( ss, (char *)packets[(last_seq+i)%WINDOW_SIZE], sizeof(packet_header)+size[(last_seq+i)%WINDOW_SIZE], 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                }
                max_value = last_seq + j;
                timer = sender_data_timer;
                if(begin > 0)
                start_seq = new_start;
                begin++;
                if(debug==1){
                    printf("Start_seq = %d\n",start_seq);
                }
            }
            else if((read<=0)&&(hasnacks==0)){
                if(debug==1)
                        printf("Sending FIN packet\n");
                packets[0]->header.type=FIN;
                sendto_dbg( ss,(char *)packets[0], sizeof(packet), 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                sent_fin =1;
                timer = sender_fin_timeout;
            }
        }
        payload = check(timer);
        if(payload == NULL){
            if(sent_fin==1){
                hasnacks=0;
                if(debug==1)
                        printf("Sending FIN packet\n");
                packets[0]->header.type=FIN;
                sendto_dbg( ss,(char *)packets[0], sizeof(packet), 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                timer = sender_fin_timeout;
                continue;
            }
            for(int k=start_seq;k<(max_value);k++){
                recvd[k%WINDOW_SIZE] = 0;
                hasnacks = 1;
            }
            new_start = start_seq;
            if(debug==1)
                printf("Resending the last window, new_start = %d\n",new_start);
            continue;
        }
        else if(payload->header.type == ACK){
            ack = payload->ack;
            for(int k=0;k<payload->num_nak;k++){
                recvd[((payload->naks)[k])%WINDOW_SIZE]=0;
                hasnacks=1;
                if(k==0)
                    new_start = (payload->naks)[k];
            }
            for(int k=ack+1;k<(max_value);k++){
                recvd[k%WINDOW_SIZE]=0;
                hasnacks=1;
                if(payload->num_nak==0){
                    if(k==(ack+1))
                        new_start = k;
                }                
            }
            if((payload->num_nak==0)&&(ack==(max_value-1))){
                hasnacks=0;
                new_start = max_value;
            }
            if(debug==1)
                printf("Received ACK/NACK packet with ACK = %d and num_nak = %d and new_start = %d\n",ack,(payload->num_nak),new_start);
        }
        else if(payload->header.type == FINACK){
            if(debug==1)
                printf("Received FINACK, FIN status = %d\n",sent_fin);
            gettimeofday(&end, NULL);
            diff = diffTime(end,start);
            printf("Total time taken for transfer = %lf seconds\n",(diff.tv_sec+(diff.tv_usec)/1000000.0));
            printf("Total Mbytes transferred = %lf\n",(total_bytes*100.0)/HUN_MB);
            if(hasfreed==0){
                printf("Entering to free the buffers\n");
                fclose(f);
                for(i=0;i<WINDOW_SIZE;i++){
                    free(packets[i]);
                }
                free(buffer);
                hasfreed = 1;
            }
            if(sent_fin==1){
                break;
            }
            else{
                sender(lossRate,s_filename,d_filename);
            }
        }
        else{
            printf("Something wrong with the receiver!!!!!!");
        }
    }
}
void establish_conn(char *filename){
    for(;;){
        printf("Pinging receiver\n");
        packet *start;
        ack_packet * rec;
        start =malloc(sizeof(packet));
        start->header.type=SYN;
        strcpy(start->data, filename);
        sendto_dbg( ss, (char *)start, sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
        if(debug==1)
            printf("Sent syn packet\n");
        rec = check(sender_syn_timer);
        if(rec == NULL){
            if(debug==1)
                printf("Sender timed out.\n");
            free(start);
            continue;
        }
        else if(rec->header.type == WAIT){
            printf("Receiver is busy, will retry after some time.\n");
            free(start);
            sleep(sender_wait_timer);
            continue;
        }
        else if(rec->header.type == GO){
            if(debug == 1)
                printf("Received Go!\n");
            free(start);
            return;
        }
    }  
}

ack_packet * check(uint timer){
    struct timeval timeout;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    ack_packet *mess;
    char *mess_buf;
    fd_set temp_mask;
    int num;
    mess_buf = malloc(MAX_MESS_LEN);
    FD_ZERO( &dummy_mask );
    while(1){
        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = timer;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
                if ( FD_ISSET( sr, &temp_mask) ) {
                    from_len = sizeof(from_addr);
                    recvfrom( sr, mess_buf, MAX_MESS_LEN, 0,  
                              (struct sockaddr *)&from_addr, 
                              &from_len );
                    from_ip = from_addr.sin_addr.s_addr;
                    if(host_num == from_ip){
                        if(debug==1)
                        printf("Received packet from correct receiver\n");
                        mess = (ack_packet *)mess_buf;
                        return mess;
                    }
                }
        } else {
            return NULL;   
        }
    }
    return NULL;
}
