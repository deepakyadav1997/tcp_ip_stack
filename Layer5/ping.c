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

void layer5_ero_ping_fn(node_t *node,char* dst_ip,char *ero_ip){
    printf("Trying to ping %s from node %s via %s.....\n",dst_ip,node->node_name,ero_ip);

    //Create a new ip header
    ip_hdr_t *ip_hdr = calloc(1,sizeof(ip_hdr_t));
    initialize_ip_hdr(ip_hdr);
    unsigned int dest_ip_int = convert_ip_from_str_to_int(dst_ip);
    unsigned int ero_ip_int = convert_ip_from_str_to_int(ero_ip);
    unsigned int src_ip_int = convert_ip_from_str_to_int(NODE_LOOPBACK_ADDR(node));

    ip_hdr->src_ip = src_ip_int;
    ip_hdr->dst_ip = dest_ip_int;
    ip_hdr->protocol = ICMP_PRO;

    ip_hdr->total_length = sizeof(ip_hdr_t)/4;
    demote_packet_to_layer3(node,(char*)ip_hdr,
                            ip_hdr->total_length*4,
                            IP_IN_IP,
                            ero_ip_int);
    free(ip_hdr);

}

