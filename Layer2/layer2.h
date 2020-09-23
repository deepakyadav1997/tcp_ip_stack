#ifndef __LAYER2__
#define __LAYER2__

#include "../glthreads_lib/glthread.h"
#include "../net.h"
#include "../graph.h"
#include "../tcpconst.h"

#include<stdlib.h>
#include<memory.h>

#define ETH_HDR_SIZE_EXCL_PAYLOAD  (sizeof(ethernet_hdr_t) - sizeof(offsetof(ethernet_hdr_t,payload)))
#define ETH_FCS(eth_hdr_ptr, payload_size)    *((unsigned int*)((char*)((ethernet_hdr_t*)(eth_hdr_ptr)->payload) + payload_size))



#pragma pack (push,1)
typedef struct arp_hdr_{

    short hw_type;          /*1 for ethernet cable*/
    short proto_type;       /*0x0800 for IPV4*/
    char hw_addr_len;       /*6 for MAC*/
    char proto_addr_len;    /*4 for IPV4*/
    short op_code;          /*req or reply*/
    mac_add_t src_mac;      /*MAC of OIF interface*/
    unsigned int src_ip;        /*IP of OIF*/
    mac_add_t dst_mac;      /*?*/
    unsigned int dst_ip;        /*IP for which ARP is being resolved*/
} arp_hdr_t;


#pragma pack (push,1)
typedef struct ethernet_hdr_{
    mac_add_t dst_mac;
    mac_add_t src_mac;
    unsigned short type;
    char payload[248];  /*Max allowed 1500*/
    unsigned int FCS;
} ethernet_hdr_t;
#pragma pack(pop)

static inline ethernet_hdr_t * ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    char* temp = (char*)calloc(1,pkt_size);
    memcpy(temp,pkt,pkt_size);
    ethernet_hdr_t * header = (ethernet_hdr_t*)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset(header,0,sizeof(ETH_HDR_SIZE_EXCL_PAYLOAD));
    memcpy(header->payload,temp,pkt_size);
    ETH_FCS(header,pkt_size) = 0;
    free(temp);
    return header;
}

static inline bool_t l2_frame_recv_qualify_on_interface(interface_t *interface,ethernet_hdr_t* header){
    if(!IS_INTF_L3_MODE(interface))
        return FALSE;
    
    if(memcmp(IF_MAC(interface),header->dst_mac.mac,sizeof(mac_add_t)) == 0){
        return TRUE;
    }
    if(IS_MAC_BROADCAST_ADDR(header->dst_mac.mac)){
        return TRUE;
    }
    return TRUE;
}

//ARP API's and table

typedef struct arp_table_{
    glthread_t arp_entries;
} arp_table_t;

typedef struct arp_entry_{
    ip_add_t ip_addr;   /*key*/
    mac_add_t mac_addr;
    char oif_name[IF_NAME_SIZE];
    glthread_t arp_glue;
} arp_entry_t;

GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry, arp_entry_t, arp_glue,glthreadptr);

void init_arp_table(arp_table_t ** arp_table);

arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr);

void clear_arp_table(arp_table_t *arp_table);

void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr);

bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry);

void dump_arp_table(arp_table_t *arp_table);

void arp_table_update_from_arp_reply(arp_table_t *arp_table,
                                    arp_hdr_t *arp_hdr,
                                    interface_t *iif);

void send_arp_broadcast_request(node_t  *node,
                                interface_t *oif,
                                char * ip_addr);

#endif