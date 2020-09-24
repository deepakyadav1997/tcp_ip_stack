
#include<arpa/inet.h>
#include<assert.h>


#include "layer2.h"


void init_arp_table(arp_table_t ** arp_table){
    *arp_table = calloc(1,sizeof(arp_table_t));
    init_glthread(&((*arp_table)->arp_entries));
}

arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr){
    glthread_t *current;
    arp_entry_t * arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries,current){
        arp_entry = arp_glue_to_arp_entry(current);
        if(strncmp(arp_entry->ip_addr.ip_addr,ip_addr,16) == 0){
            return arp_entry;
        }
    }ITERATE_GLTHREAD_END

    return NULL;
}

void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr){
    arp_entry_t *arp = arp_table_lookup(arp_table,ip_addr);
    if(!arp)
        return ;
    remove_glthread(&arp->arp_glue);
    free(arp);

}
bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry){
    arp_entry_t *arp_entry_old = arp_table_lookup(arp_table,arp_entry->ip_addr.ip_addr);

    if(arp_entry_old && memcmp(arp_entry,arp_entry_old,sizeof(arp_entry_t)) == 0){
        return FALSE;
        // Responsibility of the caller to free the memory of duplicate arp record
    }
    if(arp_entry_old){
        delete_arp_table_entry(arp_table,arp_entry_old->ip_addr.ip_addr);
    }
    init_glthread(&arp_entry->arp_glue);
    glthread_add_next(&arp_table->arp_entries,&arp_entry->arp_glue);
    //printf("Ip adres in arp %s\n",arp_glue_to_arp_entry(arp_table->arp_entries.right)->oif_name);
    return TRUE;
}

void arp_table_update_from_arp_reply(arp_table_t *arp_table,
                                    arp_hdr_t *arp_hdr,
                                    interface_t *iif){

    printf("Updating arp table on interface %s\n",iif->if_name);
    unsigned int src_ip = 0;
    assert(arp_hdr->op_code == ARP_REPLY);
    arp_entry_t *arp_entry = calloc(1,sizeof(arp_entry_t));
    src_ip = htonl(arp_hdr->src_ip);
    inet_ntop(AF_INET,&src_ip,&arp_entry->ip_addr.ip_addr,16);
    arp_entry->ip_addr.ip_addr[15] = '\0';
    memcpy(arp_entry->mac_addr.mac,arp_hdr->src_mac.mac,sizeof(mac_add_t));
    strncpy(arp_entry->oif_name,iif->if_name,IF_NAME_SIZE);

    bool_t rc = arp_table_entry_add(arp_table,arp_entry);
    if(rc == FALSE){
        free(arp_entry);
    }
    // else if (rc == TRUE){
    //    dump_arp_table(arp_table);
    // }

}

void dump_arp_table(arp_table_t *arp_table){
    printf("Dumping arp records\n");
    glthread_t *curr;
    arp_entry_t *arp_entry;
    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries, curr){

        arp_entry = arp_glue_to_arp_entry(curr);
        printf("IP : %s, MAC : %u:%u:%u:%u:%u:%u, OIF = %s\n", 
            arp_entry->ip_addr.ip_addr, 
            arp_entry->mac_addr.mac[0], 
            arp_entry->mac_addr.mac[1], 
            arp_entry->mac_addr.mac[2], 
            arp_entry->mac_addr.mac[3], 
            arp_entry->mac_addr.mac[4], 
            arp_entry->mac_addr.mac[5], 
            arp_entry->oif_name);
    } ITERATE_GLTHREAD_END;
}


void send_arp_broadcast_request(node_t  *node,
                                interface_t *oif,
                                char * ip_addr){

    unsigned int payload_size = sizeof(arp_hdr_t);
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t*)
                                    (calloc(1,ETH_HDR_SIZE_EXCL_PAYLOAD+payload_size));
    
    if(!oif){
        oif = node_get_matching_subnet_interface(node,ip_addr);
        if(!oif){
            printf("%s : No eligible interface for ARP resolution of ip address : %s\n",node->node_name,ip_addr);
            return;
        }
        //Step 1: prepare the ethernet header
        layer2_fill_with_broadcast_mac(ethernet_hdr->dst_mac.mac);
        memcpy(ethernet_hdr->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));
        ethernet_hdr->type = ARP_MSG;

        //Step 2: fill broadcast arp message
        arp_hdr_t * arp_hdr = (arp_hdr_t*)ethernet_hdr->payload;
        arp_hdr->hw_type = 1;
        arp_hdr->proto_type =0x0800;
        arp_hdr->hw_addr_len = 6;
        arp_hdr->proto_addr_len = 4;

        arp_hdr->op_code = ARP_BROAD_REQ;

        memcpy(arp_hdr->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));
        
        inet_pton(AF_INET,IF_IP(oif),&arp_hdr->src_ip);
        arp_hdr->src_ip = htonl(arp_hdr->src_ip);

        memset(arp_hdr->dst_mac.mac,0,sizeof(mac_add_t));

        inet_pton(AF_INET,ip_addr,&arp_hdr->dst_ip);
        arp_hdr->dst_ip = htonl(arp_hdr->dst_ip);

        //Temporarily set to 0. Dont access FCS directly in this manner
        ETH_FCS(ethernet_hdr,sizeof(arp_hdr_t)) = 0;

        //Step 3:Dispath the arp request
        send_packet_out((char*)ethernet_hdr,payload_size + ETH_HDR_SIZE_EXCL_PAYLOAD,oif);
        printf("freeing ethernet header  source mac %d\n",arp_hdr->src_mac.mac);
        free(ethernet_hdr);
    }                


}

static void send_arp_reply_message(ethernet_hdr_t * ethernet_hdr,interface_t * oif){
    arp_hdr_t * arp_hdr_in = (arp_hdr_t *)ethernet_hdr->payload;
    ethernet_hdr_t * ethernet_hdr_reply = (ethernet_hdr_t*)calloc(1,MAX_PACKET_BUFFER_SIZE);

    memcpy(ethernet_hdr_reply->dst_mac.mac,arp_hdr_in->dst_mac.mac,sizeof(mac_add_t));
    memcpy(ethernet_hdr_reply->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));

    ethernet_hdr_reply->type = ARP_MSG;
    arp_hdr_t * arp_reply = (arp_hdr_t*)ethernet_hdr_reply->payload;

    arp_reply->hw_type = 1;
    arp_reply->proto_type = 0x0800;
    arp_reply->hw_addr_len = sizeof(mac_add_t);
    arp_reply->proto_addr_len = 4;

    arp_reply->op_code =ARP_REPLY;
    memcpy(arp_reply->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));

    inet_pton(AF_INET,IF_IP(oif),&arp_reply->src_ip);
    arp_reply->src_ip = htonl(arp_reply->src_ip);

    memcpy(arp_reply->dst_mac.mac,arp_hdr_in->src_mac.mac,sizeof(mac_add_t));
    arp_reply->dst_ip = arp_hdr_in->src_ip;

    ETH_FCS(ethernet_hdr_reply,sizeof(arp_hdr_t)) = 0;

    unsigned int total_pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD + sizeof(arp_hdr_t);
    char* shifted_buffer = pkt_buffer_shift_right((char*)ethernet_hdr_reply,total_pkt_size,MAX_PACKET_BUFFER_SIZE);

    send_packet_out(shifted_buffer,total_pkt_size,oif);
    free(ethernet_hdr_reply);

}

static void process_arp_reply_message(node_t* node,interface_t* iif,ethernet_hdr_t* ethernet_hdr){
    printf("%s :Arp reply recieved at on interface %s of node %s\n",__FUNCTION__,iif->if_name,node->node_name);
    arp_table_update_from_arp_reply(node->node_nw_prop.arp_table,(arp_hdr_t*)ethernet_hdr->payload,iif);
}

static void process_arp_broadcast_request(node_t* node,interface_t* iif,ethernet_hdr_t* eth_hdr){

    printf("%s Broadcast recieved on node %s at interface %s\n", __FUNCTION__,node->node_name,iif->if_name);
    char ip_addr[16];
    arp_hdr_t * arp_hdr = (arp_hdr_t *)eth_hdr->payload;
    unsigned int dst_ip_addr = htonl(arp_hdr->dst_ip);
    inet_ntop(AF_INET,&dst_ip_addr,ip_addr,16);
    ip_addr[15] = '\0';

    if(strncmp(IF_IP(iif),ip_addr,16) != 0){
        printf("%s : ARP message dropped. Destination ip adress  %s does not matches the interface ip :%s\n",
                node->node_name,ip_addr,
                IF_IP(iif));
        return;
    }
    send_arp_reply_message(eth_hdr,iif);
}
void layer2_frame_recv(node_t * node,interface_t *interface,char* pkt,unsigned int pkt_size){
    //Entry poin in tcp/ip stack from the bottom
    ethernet_hdr_t * ethernet_hdr = (ethernet_hdr_t*) pkt;
    if(l2_frame_recv_qualify_on_interface(interface,ethernet_hdr) == FALSE){
        printf("L2 frame rejected\n");
        return;
    }
    printf("L2 frame accepted \n");
    switch (ethernet_hdr->type)
    {
    case ARP_MSG:
        {
            arp_hdr_t * arp_hdr = (arp_hdr_t*) ethernet_hdr->payload;
            switch (arp_hdr->op_code)
            {
            case ARP_REPLY:
                process_arp_reply_message(node,interface,ethernet_hdr);
                break;
            case ARP_BROAD_REQ:
                process_arp_broadcast_request(node,interface,ethernet_hdr);
                break;
            default:
                break;
            }
        }
        break;
    
    default:
        //promote to layer 3
        break;
    }
}

void node_set_intf_l2_mode(node_t * node,
                           char* intf_name,
                           char* intf_l2_mode_option){

    interface_t * interface = get_node_if_by_name(node,intf_name);
    if(!interface){
        printf("%s : interface not found\n",intf_name);
        return;
    }
    intf_l2_mode_t intf_l2_mode;
    if(strncmp(intf_l2_mode_option, "access", strlen("access")) == 0){
        intf_l2_mode = ACCESS;    
    }
    else if(strncmp(intf_l2_mode_option, "trunk", strlen("trunk")) ==0){
        intf_l2_mode = TRUNK;
    }
    else{
        assert(0);
    }
    if(IS_INTF_L3_MODE(interface)){
        printf("Interface %s was configured in L3 mode.Overwriting previous configurations\n",intf_name);
        interface->intf_nw_prop.is_ipaddr_config == FALSE;
        memset(IF_IP(interface),0,16);
        IF_IP(interface)[15] = '\0';
    }
    interface->intf_nw_prop.intf_l2_mode = intf_l2_mode;

}