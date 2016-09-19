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
#define PAYLOAD_SIZE (1400-sizeof(packet_header))

int gethostname(char*,size_t);

void PromptForHostName( char *my_name, char *host_name, size_t max_len ); 
packet * check(void);
void establish_conn(void);
fd_set mask,dummy_mask;
int host_num;
int ss,sr;
struct sockaddr_in    send_addr;

int main(int argc, char** argv)
{
    struct sockaddr_in    name;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    //char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    char                  *s_filename,*host_name;
    char                  *d_filename;
    int                   lossRate,i,j,read,resend=0,ack;
    //fd_set                mask;
    fd_set                temp_mask;
    int                   num,last_seq=0,start_seq=0,hasnacks=0;
    int                   size[WINDOW_SIZE];
    char                  *input_buf;
    struct timeval        timeout;
    char                  **buffer;
    packet                *packets[WINDOW_SIZE];
    packet                *rec;
    ack_payload           *payload;
    
    lossRate = atoi(argv[1]);
    s_filename = argv[2];
    d_filename = strtok(argv[3],"@");
    host_name = strtok(NULL,"@");
    gethostname(my_name, NAME_LENGTH);
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
    
    //PromptForHostName(my_name,host_name,NAME_LENGTH);
    
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
    //FD_SET( (long)0, &mask ); /* stdin */
    sendto_dbg_init(lossRate);
    FILE *f = fopen(s_filename,'r');
    buffer = (char **)malloc(sizeof(char *)*WINDOW_SIZE);
    buffer[0] = (char *)malloc(sizeof(char)*WINDOW_SIZE*PAYLOAD_SIZE);
    for(i=0;i<WINDOW_SIZE;i++){
        buffer[i] = (*buffer + PAYLOAD_SIZE*i);
        packets[i] = malloc(sizeof(packet));
    }
    //temp_mask = mask;
    //timeout.tv_sec = 10;
    //timeout.tv_usec = 0;
    // Sending syn packet
    establish_conn();
    int total=WINDOW_SIZE;
    while(1){
        if((resend == 0)&&((hasnacks==0)||((payload->num_nak)>start_seq))){
            if(hasnacks==1)
                total=(payload->num_nak)-start_seq;
            else
                total = WINDOW_SIZE;
            for(j=0;j<total;j++){
                read = fread(buffer[(start_seq+j)%WINDOW_SIZE],1,PAYLOAD_SIZE,f);
                size[(start_seq+j)%WINDOW_SIZE]=read;
                if(read==0)
                    break;
            }
        }
        if(j>0){
            for(i=0;i<j;i++){
                for(int h=0;h<size[(start_seq+i)%WINDOW_SIZE];h++)
                    packets[(start_seq+i)%WINDOW_SIZE]->data[h] = buffer[(start_seq+i)%WINDOW_SIZE][h];
                packets[(start_seq+i)%WINDOW_SIZE]->header.seq_num=i+last_seq;
                packets[(start_seq+i)%WINDOW_SIZE]->header.type=DATA;
                sendto_dbg( ss, packets[(start_seq+i)%WINDOW_SIZE], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
            }
            
            /*if(i<WINDOW_SIZE){
                packets[i]->data = buffer[i];
                packets[i]->header = {.seq_num=i;.type=DATA};
                sendto_dbg( ss, packets[i], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
                last_seq++;
            }*/
        }
        else if((read<=0)&&(hasnacks==0)){
            packets[0]->header.type=FIN;
            sendto_dbg( ss, packets[0], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
        }
        if(hasnacks==1){
            for(int k=0;k<payload->num_nak;k++){
                sendto_dbg( ss, packets[(payload->naks[k])%WINDOW_SIZE], sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
            }
        }
        packet * rec = check();
        if(rec == NULL){
            resend = 1;
            continue;
        }
        if(rec->header.type == ACK){
            payload = (ack_payload *)rec->data;
            resend=0;
            start_seq = last_seq;
            last_seq += i;
            ack = payload->ack;
            if(payload->num_nak>0)
                hasnacks = 1;
            else
                hasnacks = 0;
        }
        else if(rec->header.type == FINACK){
            fclose(f);
            for(i=0;i<WINDOW_SIZE;i++){
                free(buffer[i]);
                free(packets[i]);
            }
            free(buffer);
            break;
        }
    }
    return 0;

}
void establish_conn(void){
    //TODO
    while(1){
        packet *start;
        start->header.type=SYN;
        sendto_dbg( ss, start, sizeof(packet), 0, 
                  (struct sockaddr *)&send_addr, sizeof(send_addr) );
        packet * rec = check();
        if(rec == NULL)
            continue;
        else if(rec->header.type == WAIT){
            //TODO
            sleep(10);
        }
        else if(rec->header.type == GO)
            return;
    }    
}

packet * check(void){
    struct timeval timeout;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    int bytes;
    packet *mess;
    char mess_buf[MAX_MESS_LEN];
    fd_set temp_mask;
    while(1){
        temp_mask = mask;
        timeout.tv_sec = 0;
        //TODO
        timeout.tv_usec = sender_fin_timeout;
        int num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
                if ( FD_ISSET( sr, &temp_mask) ) {
                    from_len = sizeof(from_addr);
                    bytes = recvfrom( sr, mess_buf, sizeof(mess_buf), 0,  
                              (struct sockaddr *)&from_addr, 
                              &from_len );
                    //mess_buf[bytes] = 0;
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
