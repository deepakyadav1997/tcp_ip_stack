#ifndef __COMM__
#define __COMM__

#define MAX_PACKET_BUFFER_SIZE 2048

typedef struct node_ node_t;
typedef struct graph_ graph_t;
typedef struct interface_ interface_t;

void init_udp_socket(node_t *node);
void network_start_pkt_receiver_thread(graph_t * topology);

int send_packet_out(char *pkt,unsigned int pkt_size, interface_t * outgoing_interface);
int pkt_recieve(node_t *node,interface_t *interface,char* pkt,unsigned int pkt_size);

int send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size);
int send_pkt_flood_l2_intf_only(node_t *node,
                                interface_t *exempted_intf,        /*Interface on which the frame was recvd by L2 switch*/
                                char *pkt, 
                                unsigned int pkt_size);


#endif