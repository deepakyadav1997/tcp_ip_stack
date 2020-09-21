#ifndef __LAYER2__
#define __LAYER2__

#include "../glthreads_lib/glthread.h"
#include "../net.h"
#include "../graph.h"

#include<stdlib.h>
#include<memory.h>

#define ETH_HDR_SIZE_EXCL_PAYLOAD  (sizeof(ethernet_hdr_t) - sizeof(offsetof(ethernet_hdr_t,payload)))
#define ETH_FCS(eth_hdr_ptr, payload_size)    *((unsigned int*)((char*)((ethernet_hdr_t*)(eth_hdr_ptr)->payload) + payload_size))


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
    char* temp = (char*)calloc(1,sizeof(pkt_size));
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
    return FALSE;
}

#endif