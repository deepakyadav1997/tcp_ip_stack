#include<stdio.h>
#include<arpa/inet.h>

#include "../graph.h"
#include "../Layer3/layer3.h"


void layer5_ping_fn(node_t * node,char* dst_ip){
    printf("Trying to ping %s from node %s.....\n",dst_ip,node->node_name);
    unsigned int addr_int;

    inet_pton(AF_INET, dst_ip, &addr_int);
    addr_int = htonl(addr_int);

    /* We dont have any application or transport layer paylod, so, directly prepare
     * L3 hdr*/
    demote_packet_to_layer3(node, NULL, 0, ICMP_PRO, addr_int);
}

