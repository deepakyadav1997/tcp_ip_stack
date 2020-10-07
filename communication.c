#include "communication.h"
#include "graph.h"

#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> /*for struct hostent*/


static unsigned int udp_port = 42860;

static unsigned int get_next_udp_port_number(){
    return udp_port++;
}

void init_udp_socket(node_t *node){
    node->udp_port_number = get_next_udp_port_number();
    int udp_socket_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    int opt=TRUE;
    if (setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt))<0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if(udp_socket_fd == -1){
        printf("Socket Creation Failed for node %s\n", node->node_name);
        return;   
    }

    struct sockaddr_in node_addr;
    node_addr.sin_family = AF_INET;
    node_addr.sin_port = node->udp_port_number;
    node_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(udp_socket_fd,(struct sockaddr*)&node_addr,sizeof(struct sockaddr)) == -1){
        int err = errno;
        printf("Error! Could not bind socket for node %s  Errno %d \n",node->node_name,err);
        return;
    }
    node->udp_socket_fd = udp_socket_fd;
}

static char recv_buffer[MAX_PACKET_BUFFER_SIZE];
static char send_buffer[MAX_PACKET_BUFFER_SIZE];

static void _pkt_recieve(node_t * recieving_node,
                         char*    pkt_with_aux_data,
                         unsigned int pkt_size){
    
    char *interface_recieved_name = pkt_with_aux_data;
    interface_t * recieving_interface = get_node_if_by_name(recieving_node,pkt_with_aux_data);
    if(!recieving_interface){
        printf("Error! recieved packet on an unkown interface\n");
        return;
    }
    pkt_recieve(recieving_node,recieving_interface,pkt_with_aux_data + IF_NAME_SIZE,pkt_size - IF_NAME_SIZE);
}

static int _send_packet_out(int sock_fd,
                            char* pkt_data,
                            unsigned int pkt_size,
                            unsigned int destination_udp_port_number){
    
    int rc;
    struct sockaddr_in dest_addr;
    struct hostent * host = (struct hostent*) gethostbyname("127.0.0.1");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port =    destination_udp_port_number;
    dest_addr.sin_addr = *((struct in_addr*)host->h_addr);
    rc = sendto(sock_fd,pkt_data,pkt_size,0,(struct sockaddr*)&dest_addr,sizeof(struct sockaddr));
    return rc;
}

static void* _network_start_pkt_receiver_thread(void* arg){
    node_t *node;
    glthread_t * current;
    fd_set active_sock_fd_set,
           backup_sock_fd_set;
    int sock_max_fd = 0; // maximum socket fd
    int bytes_recvd = 0;
    graph_t * topo = (void*) arg;

    int addr_len = sizeof(struct sockaddr);
    struct sockaddr_in sender_addr;

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    ITERATE_GLTHREAD_BEGIN(&topo->node_list,current){
        node = graph_glue_to_node(current);
        if(!node->udp_socket_fd)
            continue;
        if(node->udp_socket_fd > sock_max_fd){
            sock_max_fd = node->udp_socket_fd;
        }
        FD_SET(node->udp_socket_fd,&backup_sock_fd_set);
    }ITERATE_GLTHREAD_END
    while(1){
        memcpy(&active_sock_fd_set,&backup_sock_fd_set,sizeof(fd_set));
        /*
        Select is a blocking system call,i.e it waits till any socket fd recieves any data
        the code after select is only executed after any of the fd's is activated
        */
        select(sock_max_fd+1,&active_sock_fd_set,NULL,NULL,NULL);
        ITERATE_GLTHREAD_BEGIN(&topo->node_list,current){
            node = graph_glue_to_node(current);
            if(FD_ISSET(node->udp_socket_fd,&active_sock_fd_set)){
                memset(recv_buffer,0,MAX_PACKET_BUFFER_SIZE);
                bytes_recvd = recvfrom(node->udp_socket_fd,
                                       (char*)recv_buffer,
                                       MAX_PACKET_BUFFER_SIZE,
                                       0,
                                       (struct sockaddr* )&sender_addr,
                                       &addr_len);
                _pkt_recieve(node,recv_buffer,bytes_recvd);
            }
        }ITERATE_GLTHREAD_END

    }
}
void network_start_pkt_receiver_thread(graph_t * topology){
    pthread_attr_t attr;
    pthread_t recv_packet_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    pthread_create(&recv_packet_thread,&attr,_network_start_pkt_receiver_thread,(void*) topology);
}

int send_packet_out(char *pkt,unsigned int pkt_size, interface_t * outgoing_interface){

    int rc = 0;

    node_t *sending_node = outgoing_interface->att_node;
    node_t * recieving_node = get_nbr_node(outgoing_interface);
    if(!recieving_node)
        return -1;
    unsigned int destination_udp_port_number = recieving_node->udp_port_number;

    int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(sock < 0){
        printf("Error! Sending socket creation failed. Error number = %d \n",errno);
        return -1;
    }
    interface_t *recieving_interface = &outgoing_interface->link->intf1 == outgoing_interface?
                                        &outgoing_interface->link->intf2:
                                        &outgoing_interface->link->intf1;
    
    memset(send_buffer,0,MAX_PACKET_BUFFER_SIZE);
    char* pkt_with_aux_information = send_buffer;
    strncpy(pkt_with_aux_information,recieving_interface->if_name,IF_NAME_SIZE);
    pkt_with_aux_information[IF_NAME_SIZE] = '\0';
    memcpy(pkt_with_aux_information+IF_NAME_SIZE,pkt,pkt_size);
    rc = _send_packet_out(sock,pkt_with_aux_information,
                          pkt_size+IF_NAME_SIZE,
                          destination_udp_port_number);
    close(socket);
    return rc;

}

extern void layer2_frame_recv(node_t * node,interface_t *interface,char* pkt,unsigned int pkt_size);

int pkt_recieve(node_t *node,interface_t *interface,char* pkt,unsigned int pkt_size){

    //entry point of tcp-ip stack
    //printf("msg recvd :%s  on node %s interdace %s\n",pkt,node->node_name,interface->if_name);

    // Make room for  headers
    pkt = pkt_buffer_shift_right(pkt,pkt_size,MAX_PACKET_BUFFER_SIZE - IF_NAME_SIZE);

    //Further processing of the packet 
    layer2_frame_recv(node,interface,pkt,pkt_size);
    //printf("%s returned\n",__FUNCTION__);
    return 0;
}

int send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size){
    interface_t * current;
    for(int i = 0;i< MAX_INTERFACES_PER_NODE;i++){
        current = node->intf[i];
        if(!current){
            return 0;
        }
        if(current = exempted_intf)
            continue;
        send_packet_out(pkt,pkt_size,current);
    }
    return 0;
}
int send_pkt_flood_l2_intf_only(node_t *node,
                                interface_t *exempted_intf,        /*Interface on which the frame was recvd by L2 switch*/
                                char *pkt, 
                                unsigned int pkt_size){

    for(int i = 0;i<MAX_INTERFACES_PER_NODE;i++){
        if(node->intf[i] == NULL){
            return 0;
        }
        if(node->intf[i] == exempted_intf){
            continue;
        }
        if(IS_INTF_L3_MODE(node->intf[i])){
            continue;
        }
        send_packet_out(pkt,pkt_size,node->intf[i]);
    }
    return 0;
}
