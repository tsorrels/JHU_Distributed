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




packet * check(uint timer);
void sender(int lossRate, char * s_filename, char * d_filename);
void establish_conn(char *filename);


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
    int                   i,j,k,h,read,resend,ack;
    int                   last_seq;
    int start_seq;
    int prev_seq;
    int hasnacks;
    int sent_fin;
    int                   size[WINDOW_SIZE];
    char                  **buffer;
    packet                *packets[WINDOW_SIZE];
    packet                *rec;
    ack_payload           *payload;
    uint                  timer;
    FILE *f;    
    int total;


    timer=sender_data_timer;    

    last_seq=0;
    start_seq=0;
    prev_seq=0;
    hasnacks=0;
    sent_fin=0;

    resend=0;
    ack=WINDOW_SIZE-1;


    if(debug==1)
        printf("Entered sender\n");
    //gethostname(my_name, NAME_LENGTH);
    
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
    while(1){
        if((resend == 0)&&((hasnacks==0)||((payload->naks[0])>prev_seq))){
            if(hasnacks==1)
                total=(payload->naks[0])-prev_seq;
            else
                total = ack+1;
            if(debug==1)
                    printf("Reading %d packets\n",total);
            for(j=0;j<total;j++){
                read = fread(buffer[(start_seq+j)%WINDOW_SIZE],1,PAYLOAD_SIZE,f);
                size[(start_seq+j)%WINDOW_SIZE]=read;
                if(read==0)
                    break;
            }
        }
        if(j>0){
            if(debug==1)
                    printf("Sending %d packets\n",j);
            for(i=0;i<j;i++){
                for(h=0;h<size[(start_seq+i)%WINDOW_SIZE];h++)
                    packets[(start_seq+i)%WINDOW_SIZE]->data[h] = buffer[(start_seq+i)%WINDOW_SIZE][h];
                packets[(start_seq+i)%WINDOW_SIZE]->header.seq_num=i+last_seq;
                packets[(start_seq+i)%WINDOW_SIZE]->header.type=DATA;
                sendto_dbg( ss, (char *)packets[(start_seq+i)%WINDOW_SIZE], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
            }
            timer = sender_data_timer;
        }
        else if((read<=0)&&(hasnacks==0)&&(ack==(last_seq-1))){
            if(debug==1)
                    printf("Sending FIN packet\n");
            packets[0]->header.type=FIN;
            sendto_dbg( ss,(char *)packets[0], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
            sent_fin =1;
            timer = sender_fin_timeout;
        }
        if((hasnacks==1)||(ack<(last_seq-1))){
            if(hasnacks==1){
                if(debug==1)
                    printf("Resending NAKS\n");
                for(k=0;k<payload->num_nak;k++){
                    sendto_dbg( ss, (char *)packets[(payload->naks[k])%WINDOW_SIZE], sizeof(packet), 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                }
            }
            if(ack<(last_seq-1)){
                if(debug==1)
                    printf("Resending packets after ACK\n");
                for(k=ack+1;k<last_seq;k++){
                    sendto_dbg( ss, (char *)packets[(k)%WINDOW_SIZE], sizeof(packet), 0, 
                      (struct sockaddr *)&send_addr, sizeof(send_addr) );
                }
            }
            timer = sender_data_timer;
        }
        rec = check(timer);
        if(rec == NULL){
            resend = 1;
            if(debug==1)
                printf("Resending the last window\n");
            continue;
        }
        if(rec->header.type == ACK){
            payload = (ack_payload *)rec->data;
            resend=0;
            prev_seq = start_seq;
            start_seq = last_seq;
            last_seq += i;
            ack = payload->ack;
            if(payload->num_nak>0)
                hasnacks = 1;
            else
                hasnacks = 0;
            if(debug==1)
                printf("Received ACK/NACK packet with ACK = %d and num_nak = %d\n",ack,payload->num_nak);
        }
        else if(rec->header.type == FINACK){
            if(debug==1)
                printf("Received FINACK, FIN status = %d\n",sent_fin);
            fclose(f);
	    for(i=0;i<WINDOW_SIZE;i++){
                free(packets[i]);
            }
            free(buffer);
            close(sr);
            if(sent_fin==1){
                break;
            }
            else{
                sender(lossRate,s_filename,d_filename);
            }
        }
    }
}
void establish_conn(char *filename){
    while(1){
        packet *start;
	packet * rec;
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
            continue;
        }
        else if(rec->header.type == WAIT){
            printf("Receiver is busy, will retry after some time.\n");
            sleep(sender_wait_timer);
        }
        else if(rec->header.type == GO)
            if(debug == 1)
                printf("Received Go!\n");
            return;
    }    
}

packet * check(uint timer){
    struct timeval timeout;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    packet *mess;
    char mess_buf[MAX_MESS_LEN];
    fd_set temp_mask;
    int num;
    while(1){
        temp_mask = mask;
        timeout.tv_sec = 0;
        //TODO
        timeout.tv_usec = timer;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
                if ( FD_ISSET( sr, &temp_mask) ) {
                    from_len = sizeof(from_addr);
                    recvfrom( sr, mess_buf, sizeof(mess_buf), 0,  
                              (struct sockaddr *)&from_addr, 
                              &from_len );
                    //mess_buf[bytes] = 0;
                    if(debug==1)
                        printf("Received packet\n");
                    from_ip = from_addr.sin_addr.s_addr;
                    if(host_num == from_ip){
                        mess = (packet *)mess_buf;
                        return mess;
                    }
                }
        } else {
            return NULL;   
        }
    }
    return NULL;
}
