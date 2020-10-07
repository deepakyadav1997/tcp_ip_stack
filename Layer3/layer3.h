#ifndef __L3__
#define __L3__

#include "../glthreads_lib/glthread.h"
#include "../net.h"
#include "../graph.h"

#include<stdio.h>
#include<stdlib.h>

/*The Ip hdr format as per the standard specification*/
#pragma pack (push)
typedef struct ip_hdr_{

    unsigned int version : 4 ;  /*version number, always 4 for IPv4 protocol*/    
    unsigned int ihl : 4 ;      /*length of IP hdr, in 32-bit words unit. for Ex, if this value is 5, it means length of this ip hdr is 20Bytes*/
    char tos;
    short total_length;         /*length of hdr + ip_hdr payload*/

    /* Fragmentation Related members, we shall not be using below members
     * as we will not be writing fragmentation code. if you wish, take it
     * as a extension of the project*/
    short identification;       
    unsigned int unused_flag : 1 ;
    unsigned int DF_flag : 1;   
    unsigned int MORE_flag : 1; 
    unsigned int frag_offset : 13;  


    char ttl;
    char protocol;
    short checksum;
    unsigned int src_ip;
    unsigned int dst_ip;
} ip_hdr_t;

#pragma pack(pop)

static inline void initialize_ip_hdr(ip_hdr_t *ip_hdr){
    
    ip_hdr->version = 4;
    ip_hdr->ihl = 5; /*We will not be using option field, hence hdr size shall always be 5*4 = 20B*/
    ip_hdr->tos = 0;

    ip_hdr->total_length = 0; /*To be filled by the caller*/

    /*Fragmentation related will not be used
     * int this course, initialize them all to zero*/
    ip_hdr->identification = 0; 
    ip_hdr->unused_flag = 0;
    ip_hdr->DF_flag = 1;
    ip_hdr->MORE_flag = 0;
    ip_hdr->frag_offset = 0;

    ip_hdr->ttl = 64; /*Let us use 64 avoids infinite looping of packets*/
    ip_hdr->protocol = 0; /*To be filled by the caller*/
    ip_hdr->checksum = 0; /*Not used in this course*/
    ip_hdr->src_ip = 0; /*To be filled by the caller*/ 
    ip_hdr->dst_ip = 0; /*To be filled by the caller*/
}

#define IP_HDR_LEN_IN_BYTES(ip_hdr)             (ip_hdr->ihl)*4
#define IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr)       (ip_hdr->total_length)*4
#define INCREMENT_IPHDR(ip_hdr)                 ((char*)ip_hdr + (ip_hdr->total_length)*4)
#define IP_HDR_PAYLOAD_SIZE(ip_hdr)             ((ip_hdr->total_length)*4 - (ip_hdr->ihl)*4)




typedef struct rt_table_{
    glthread_t route_list;
}rt_table_t;

typedef struct l3_route_{
    
    char dest[16];          //key
    char mask;              //key
    bool_t is_direct;       //Is the destination a local subnet,i.e directly connected
                            //If is_direct is true, the below 2 fields have no meaning
    char gw_ip[16];         // Gateway ip 
    char oif[IF_NAME_SIZE]; // outgoing interface name
    glthread_t rt_glue;
}l3_route_t;
GLTHREAD_TO_STRUCT(rt_glue_to_l3_route,l3_route_t,rt_glue,glthreadptr);

void init_rt_table(rt_table_t **rt_table);

l3_route_t * rt_table_lookup(rt_table_t *rt_table,
                            char *ip_addr,
                            char mask);

void clear_rt_table(rt_table_t *rt_table);

void delete_rt_table_entry(rt_table_t *rt_table, 
                           char *ip_addr,
                           char mask);

void rt_table_add_route(rt_table_t *rt_table, 
                        char *dst, char mask,
                        char *gw, char *oif);

void rt_table_add_direct_route(rt_table_t *rt_table,
                               char *dst, char mask);

void dump_rt_table(rt_table_t *rt_table);

l3_route_t * l3rib_lookup_lpm(rt_table_t *rt_table,
                              unsigned int dest_ip);

void promote_pkt_to_layer3(node_t *node,                /*Current node on which the pkt is received*/
                      interface_t *interface,           /*ingress interface*/
                      char *pkt, unsigned int pkt_size, /*L3 payload*/
                      int L3_protocol_number);          /*obtained from eth_hdr->type field*/
                    
void
demote_packet_to_layer3(node_t *node, 
                        char *pkt, unsigned int size,
                        int protocol_number,            /*L4 or L5 protocol type*/
                        unsigned int dest_ip_address);

#endif
