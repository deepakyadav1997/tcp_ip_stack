#include<stdio.h>

#include "tcpconst.h"
#include "Layer2/layer2.h"

void dump_arp_hdr(arp_hdr_t *arp_hdr){

    printf("---ARP header----\n");

    printf("Src Mac %x:%x:%x:%x:%x:%x\n",arp_hdr->src_mac.mac[0],
                                         arp_hdr->src_mac.mac[1],
                                         arp_hdr->src_mac.mac[2],
                                         arp_hdr->src_mac.mac[3],
                                         arp_hdr->src_mac.mac[4],
                                         arp_hdr->src_mac.mac[5]);

    printf("Dst Mac %x:%x:%x:%x:%x:%x\n",arp_hdr->dst_mac.mac[0],
                                         arp_hdr->dst_mac.mac[1],
                                         arp_hdr->dst_mac.mac[2],
                                         arp_hdr->dst_mac.mac[3],
                                         arp_hdr->dst_mac.mac[4],
                                         arp_hdr->dst_mac.mac[5]);

    char ip[16];
    convert_ip_from_int_to_str(arp_hdr->src_ip,ip); 
    printf("Source IP : %s\n",ip);
    convert_ip_from_int_to_str(arp_hdr->dst_ip,ip);
    printf("Destinaton IP: %s\n",ip);

}

void pkt_dump(ethernet_hdr_t *ethernet_hdr,unsigned int pkt_size){

    printf("Src Mac %x:%x:%x:%x:%x:%x\n",ethernet_hdr->src_mac.mac[0],
                                         ethernet_hdr->src_mac.mac[1],
                                         ethernet_hdr->src_mac.mac[2],
                                         ethernet_hdr->src_mac.mac[3],
                                         ethernet_hdr->src_mac.mac[4],
                                         ethernet_hdr->src_mac.mac[5]);

    printf("Dst Mac %x:%x:%x:%x:%x:%x\n",ethernet_hdr->dst_mac.mac[0],
                                         ethernet_hdr->dst_mac.mac[1],
                                         ethernet_hdr->dst_mac.mac[2],
                                         ethernet_hdr->dst_mac.mac[3],
                                         ethernet_hdr->dst_mac.mac[4],
                                         ethernet_hdr->dst_mac.mac[5]);


    unsigned short message_type = ethernet_hdr->type;
    if(message_type == VLAN_TAGGED_ETH_HDR){
        vlan_ethernet_hdr_t * vlan_ethernet_hdr = (vlan_ethernet_hdr_t*) ethernet_hdr;
        printf("Vlan ID: %d\n",vlan_ethernet_hdr->vlan_8021q_hdr.tci_vid);
        message_type = vlan_ethernet_hdr->type;
    }

    switch (message_type)
    {
    case ARP_MSG:
        dump_arp_hdr((arp_hdr_t*)ethernet_hdr->payload);
        break;
    
    default:
        printf(" Type is %x Offset of payload %d Size of Payload %d\n",
                                                        message_type,
                                                        offsetof(ethernet_hdr_t,payload),
                                                        pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD);
        break;
    } 

}

