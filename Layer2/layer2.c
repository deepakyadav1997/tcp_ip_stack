
#include<arpa/inet.h>
#include<assert.h>


#include "layer2.h"
#include "l2switch.h"
#include "../Layer3/layer3.h"

extern bool_t
is_layer3_local_delivery(node_t *node, 
                         uint32_t dst_ip);



void init_arp_table(arp_table_t ** arp_table){
    *arp_table = calloc(1,sizeof(arp_table_t));
    init_glthread(&((*arp_table)->arp_entries));
}

arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr){
    glthread_t *current = NULL;
    arp_entry_t * arp_entry = NULL;

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
bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry,glthread_t **arp_pending_list){
    arp_entry_t *arp_entry_old = arp_table_lookup(arp_table,arp_entry->ip_addr.ip_addr);

    // if(arp_entry_old && memcmp(arp_entry,arp_entry_old,sizeof(arp_entry_t)) == 0){
    //     return FALSE;
    //     // Responsibility of the caller to free the memory of duplicate arp record
    // }
    // if(arp_entry_old){
    //     delete_arp_table_entry(arp_table,arp_entry_old->ip_addr.ip_addr);
    // }
    // init_glthread(&arp_entry->arp_glue);
    // glthread_add_next(&arp_table->arp_entries,&arp_entry->arp_glue);
 
    // return TRUE;

    //complete entry already present;
    if(arp_entry_old && arp_entry_old->is_sane == FALSE && arp_entry->is_sane == FALSE){
        //Refresh arp record
        strncpy(arp_entry_old->ip_addr.ip_addr,arp_entry->ip_addr.ip_addr,sizeof(ip_add_t));
        arp_entry_old->ip_addr.ip_addr[sizeof(ip_add_t) - 1] = '\0';
        memcpy(arp_entry_old->mac_addr.mac,arp_entry->mac_addr.mac,sizeof(mac_add_t));
        strncpy(arp_entry_old->oif_name,arp_entry->oif_name,IF_NAME_SIZE);
        arp_entry_old->oif_name[IF_NAME_SIZE-1] = '\0';
        return FALSE; // caller should free the arp_entry
    }
    // no record, then add a new record
    else if(!arp_entry_old){
        init_glthread(&arp_entry->arp_glue);
        glthread_add_next(&arp_table->arp_entries,&arp_entry->arp_glue);
        return TRUE;
    }
    // incomplete entry is present. complete it
    // set arp entry list with the current list
    else if(arp_entry_old->is_sane == TRUE && arp_entry->is_sane == FALSE){

        strncpy(arp_entry_old->ip_addr.ip_addr,arp_entry->ip_addr.ip_addr,sizeof(ip_add_t));
        arp_entry_old->ip_addr.ip_addr[sizeof(ip_add_t) - 1] = '\0';
        memcpy(arp_entry_old->mac_addr.mac,arp_entry->mac_addr.mac,sizeof(mac_add_t));
        strncpy(arp_entry_old->oif_name,arp_entry->oif_name,IF_NAME_SIZE);
        arp_entry_old->oif_name[IF_NAME_SIZE-1] = '\0';
        //Set sane as true to indicate the entry is complete
        arp_entry_old->is_sane = FALSE;
        *arp_pending_list =  &arp_entry_old->arp_pending_list;
        return FALSE;
    }
    else if(arp_entry_old->is_sane == TRUE && arp_entry->is_sane == TRUE){
        
    }

    
}
static void
delete_arp_pending_entry(arp_pending_entry_t *arp_pending_entry){

    remove_glthread(&arp_pending_entry->arp_pending_entry_glue);
    free(arp_pending_entry);
}

void arp_table_update_from_arp_reply(arp_table_t *arp_table,
                                    arp_hdr_t *arp_hdr,
                                    interface_t *iif){

    printf("Updating arp table on interface %s\n",iif->if_name);
    glthread_t *arp_pending_list = NULL;
    unsigned int src_ip = 0;
    assert(arp_hdr->op_code == ARP_REPLY);
    arp_entry_t *arp_entry = calloc(1,sizeof(arp_entry_t));
    src_ip = htonl(arp_hdr->src_ip);
    inet_ntop(AF_INET,&src_ip,&arp_entry->ip_addr.ip_addr,16);
    arp_entry->ip_addr.ip_addr[15] = '\0';
    arp_entry->is_sane = FALSE;
    memcpy(arp_entry->mac_addr.mac,arp_hdr->src_mac.mac,sizeof(mac_add_t));
    strncpy(arp_entry->oif_name,iif->if_name,IF_NAME_SIZE);

    bool_t rc = arp_table_entry_add(arp_table,arp_entry,&arp_pending_list);
    if(arp_pending_list){
        glthread_t* current = NULL;
        arp_pending_entry_t* arp_pending_entry = NULL;
        ITERATE_GLTHREAD_BEGIN(arp_pending_list,current){
            arp_pending_entry = arp_pending_glue_to_arp_entry(current);
            remove_glthread(current);
            pending_arp_processing_callback_function(iif->att_node,iif,arp_entry,arp_pending_entry);
            delete_arp_pending_entry(arp_pending_entry);

        }ITERATE_GLTHREAD_END;
        arp_pending_list_to_arp_entry(arp_pending_list)->is_sane = FALSE;
    }
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

    printf("ARP broadcast from %s  %s\n",oif->if_name,ip_addr);
    unsigned int payload_size = sizeof(arp_hdr_t);
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t*)
                                    (calloc(1,ETH_HDR_SIZE_EXCL_PAYLOAD+payload_size));
    
    if(!oif){
        oif = node_get_matching_subnet_interface(node,ip_addr);
        if(!oif){
            printf("%s : No eligible interface for ARP resolution of ip address : %s\n",node->node_name,ip_addr);
            return;
        }
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
        printf("Dst ip %d\n",arp_hdr->dst_ip);
        //Temporarily set to 0. Dont access FCS directly in this manner
        ETH_FCS(ethernet_hdr,sizeof(arp_hdr_t)) = 0;

        //Step 3:Dispath the arp request
        send_packet_out((char*)ethernet_hdr,payload_size + ETH_HDR_SIZE_EXCL_PAYLOAD,oif);
        //printf("%d ip from header\n",((arp_hdr_t*)ethernet_hdr->payload)->dst_ip);
        printf("freeing ethernet header  source mac %d\n",arp_hdr->src_mac.mac);
        free(ethernet_hdr);               


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

void promote_pkt_to_layer2(node_t *node,
                                interface_t *interface,
                                ethernet_hdr_t *ethernet_hdr,
                                uint32_t pkt_size){

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
        case ETH_IP:
            {
                promote_pkt_to_layer3(node,interface,
                        GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr),pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD,
                        ethernet_hdr->type);
                break;
            }
        default:
           
            break;
        }                               


}

void
l2_forward_ip_packet(node_t* node,unsigned int next_hop_ip,
                    char* outgoing_intf,ethernet_hdr_t* pkt,
                    unsigned int pkt_size){
    
    interface_t* oif = NULL;
    char next_hop_ip_str[16];
    arp_entry_t * arp_entry = NULL;
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t*)pkt;
    unsigned int ethernet_payload_size = pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr);

    next_hop_ip = htonl(next_hop_ip);
    inet_ntop(AF_INET,&next_hop_ip,next_hop_ip_str,16);

    //reverse again back  since htonl reverses byte order
    next_hop_ip = htonl(next_hop_ip);
    
    if(outgoing_intf){
        oif = get_node_if_by_name(node,outgoing_intf);
        assert(oif);
        
        arp_entry = arp_table_lookup(node->node_nw_prop.arp_table,next_hop_ip_str);
        if(! arp_entry){
            //On demand arp resolution

            //creates a sane arp entry and adds it to the arp table
            create_arp_sane_entry(node->node_nw_prop.arp_table,
                                    next_hop_ip_str,
                                    (char*)pkt,pkt_size);
            
            send_arp_broadcast_request(node,oif,next_hop_ip_str);
            return;
        }
        else if(arp_entry->is_sane == TRUE){
            add_arp_pending_entry(arp_entry,
                                pending_arp_processing_callback_function,
                                (char*)pkt,pkt_size);
            return;
        }
        goto l2_frame_prep;
    }
    //self ping
    if(is_layer3_local_delivery(node,next_hop_ip)){
        promote_pkt_to_layer3(node,
                              0,
                              GET_ETHERNET_HDR_PAYLOAD(pkt),
                              pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr),
                              ethernet_hdr->type);
        return;
    }
    oif = node_get_matching_subnet_interface(node,next_hop_ip_str);
    if(!oif){
        printf("NO matching subnet for %s on node %s\n",next_hop_ip_str,node->node_name);
        return;
    }
    arp_entry = arp_table_lookup(node->node_nw_prop.arp_table,next_hop_ip_str); 
    if(! arp_entry){
            create_arp_sane_entry(node->node_nw_prop.arp_table,
                                    next_hop_ip_str,
                                    (char*)pkt,pkt_size);
            send_arp_broadcast_request(node,oif,next_hop_ip_str);
            return;
    }
    l2_frame_prep:
        memcpy(ethernet_hdr->dst_mac.mac,arp_entry->mac_addr.mac,sizeof(mac_add_t));
        memcpy(ethernet_hdr->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));
        send_packet_out((char*) ethernet_hdr,pkt_size,oif); 

}
void 
layer2_pkt_recieve_from_top(node_t *node,unsigned int next_hop_ip,char *outgoing_intf,      
                          char *pkt, unsigned int pkt_size,   
                          int protocol_number){

    assert(pkt_size < sizeof(((ethernet_hdr_t*)0)->payload));
    if(protocol_number == ETH_IP){
        ethernet_hdr_t * empty_ethernet_hdr = ALLOC_ETH_HDR_WITH_PAYLOAD(pkt,pkt_size);
        empty_ethernet_hdr->type = ETH_IP; 
        // ip_hdr_t *inner_ip = INCREMENT_IPHDR(((ip_hdr_t*)(empty_ethernet_hdr->payload)));
        // printf("%s: %d   %d\n",__FUNCTION__,inner_ip->dst_ip,inner_ip->src_ip);
        l2_forward_ip_packet(node,next_hop_ip,
                            outgoing_intf,empty_ethernet_hdr,
                            pkt_size + GET_ETH_HDR_SIZE_EXCL_PAYLOAD(empty_ethernet_hdr));
    }

}

void demote_pkt_to_layer2(node_t *node,unsigned int next_hop_ip,char *outgoing_intf,      
                          char *pkt, unsigned int pkt_size,   
                          int protocol_number){               

    layer2_pkt_recieve_from_top(node,next_hop_ip,outgoing_intf,
                                pkt,pkt_size,
                                protocol_number);                         


}


void layer2_frame_recv(node_t * node,interface_t *interface,char* pkt,unsigned int pkt_size){
    //Entry poin in tcp/ip stack from the bottom
    ethernet_hdr_t * ethernet_hdr = (ethernet_hdr_t*) pkt;
    
    int output_vlan_id = 0;
    if(l2_frame_recv_qualify_on_interface(interface,ethernet_hdr,&output_vlan_id) == FALSE){
        printf("L2 frame rejected on interface %s\n",interface->if_name);
        return;
    }

    //printf("L2 frame accepted \n");
    if(IS_INTF_L3_MODE(interface)){
        promote_pkt_to_layer2(node,interface,ethernet_hdr,pkt_size);
    }
    else if(interface->intf_nw_prop.intf_l2_mode == ACCESS || interface->intf_nw_prop.intf_l2_mode == TRUNK){
            unsigned int new_pkt_size = 0;
            //Frame needs to be tagged with output_vlan_id?
            ethernet_hdr = (ethernet_hdr_t*) pkt;
            //printf("%d ip from header %s  %s\n",((arp_hdr_t*)ethernet_hdr->payload)->dst_ip,node->node_name,__FUNCTION__);
            if(output_vlan_id != 0){
                pkt = (char*) tag_pkt_with_vlan_id((ethernet_hdr_t*)pkt,pkt_size,output_vlan_id,&new_pkt_size);
                assert(new_pkt_size != pkt_size);
            }

            new_pkt_size = output_vlan_id == 0 ? pkt_size:new_pkt_size;
            l2_switch_recv_frame(interface,pkt,new_pkt_size);
    }
    else{
        return;
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
        if(interface->intf_nw_prop.intf_l2_mode == TRUNK){
            printf("Overwriting trunk mode on %s interface with access mode and flushing previous vlan memberships\n",interface->if_name);
            for(int i = 0;i < MAX_VLAN_MEMBERSHIP;i++){
                interface->intf_nw_prop.vlans[i] = 0;
            }
        }
        intf_l2_mode = ACCESS;    
    }
    else if(strncmp(intf_l2_mode_option, "trunk", strlen("trunk")) ==0){
        if(interface->intf_nw_prop.intf_l2_mode == ACCESS){
            printf("Reconfiguring interface %s to trunk mode and flushing the previous vlan membership\n",interface->if_name);
            for(int i = 0;i < MAX_VLAN_MEMBERSHIP;i++){
                interface->intf_nw_prop.vlans[i] = 0;
            }
        }
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

void node_set_intf_vlan_membership(node_t *node,
                                    char *intf_name,
                                    unsigned int vlan_id){

    interface_t *interface = get_node_if_by_name(node,intf_name);
    if(interface == NULL){
        printf("No such interface %s on node %s\n",intf_name,node->node_name);
        return;
    }

    //Interface is in L3 mode
    if(IS_INTF_L3_MODE(interface)){
        printf("Error! interface is in L3 mode. Cannot set vlan membership\n");
    }

    //L2 mode is not enabled
    if(interface->intf_nw_prop.intf_l2_mode != ACCESS && interface->intf_nw_prop.intf_l2_mode != TRUNK){
        printf("Error! L2 mode is not enabled on the interface %s\n",intf_name);
    }

    //Interface is in access mode
    if(interface->intf_nw_prop.intf_l2_mode == ACCESS){
        //Set first vlan id to vlan id
        interface->intf_nw_prop.vlans[0] = vlan_id;
        for(int i = 1;i<MAX_VLAN_MEMBERSHIP;i++){
            if(interface->intf_nw_prop.vlans[i] != 0){
                printf("More than one Vlans found on interface %s in access mode.Overwriting with 0 \n",intf_name);
                interface->intf_nw_prop.vlans[i] = 0;
            }
        }
    }

    //Interface is in trunk mode
    if(interface->intf_nw_prop.intf_l2_mode == TRUNK){
        for(int i = 0;i < MAX_VLAN_MEMBERSHIP;i++){
            //Found an empty slot
            if(interface->intf_nw_prop.vlans[i] == 0){
                interface->intf_nw_prop.vlans[i] = vlan_id;
                return;
            }
        }

        // All vlan slots are configured
        printf("Max possible number of vlans already configured on %s\n",intf_name);
    }

}


ethernet_hdr_t * tag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr,
                                     unsigned int total_pkt_size,
                                     int vlan_id,
                                     unsigned int *new_pkt_size){

    vlan_8021q_hdr_t*  vlan_8021q_hdr = is_pkt_vlan_tagged(ethernet_hdr);
    int payload_size = total_pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD;
    printf("%s ip address %d %d\n",__FUNCTION__,((arp_hdr_t*)(ethernet_hdr->payload))->dst_ip,((arp_hdr_t*)(ethernet_hdr->payload))->src_ip);
    //Packet is already tagged
    //Just update the vlan id
    if(vlan_8021q_hdr){
        vlan_8021q_hdr->tci_vid =  (short)vlan_id;
        //SET_COMMON_ETH_FCS(ethernet_hdr,payload_size,0);
        *new_pkt_size = total_pkt_size;
        return ethernet_hdr;
    }

    // Packet is untagged

    //Store mac addresses and type in temp memory
    ethernet_hdr_t old_hdr;
    memcpy((char*)&old_hdr,(char*)ethernet_hdr,ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(old_hdr.FCS));

    //Expand the current header
    vlan_ethernet_hdr_t * vlan_ethernet_hdr = (vlan_ethernet_hdr_t*)((char*)ethernet_hdr - sizeof(vlan_8021q_hdr_t));

    //restore the values in the tagged ethernet header
    memset((char*)vlan_ethernet_hdr,0,VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(vlan_ethernet_hdr->FCS));
    memcpy(vlan_ethernet_hdr->dst_mac.mac,old_hdr.dst_mac.mac,sizeof(mac_add_t));
    memcpy(vlan_ethernet_hdr->src_mac.mac,old_hdr.src_mac.mac,sizeof(mac_add_t));

    // Set 8021q header values
    vlan_ethernet_hdr->vlan_8021q_hdr.tpid = VLAN_TAGGED_ETH_HDR;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_vid = vlan_id;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_pcp = 0;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_dei = 0;

    //Set type and FCS fields
    vlan_ethernet_hdr->type = old_hdr.type;
    //SET_COMMON_ETH_FCS((ethernet_hdr_t*) vlan_ethernet_hdr,payload_size,0);
    printf("%s ip address %d %d\n",__FUNCTION__,((arp_hdr_t*)(vlan_ethernet_hdr->payload))->dst_ip,((arp_hdr_t*)(ethernet_hdr->payload))->src_ip);
    *new_pkt_size = total_pkt_size + sizeof(vlan_8021q_hdr_t);
    return (ethernet_hdr_t*) vlan_ethernet_hdr;


}

ethernet_hdr_t * untag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr,
                                        unsigned int total_pkt_size,
                                        unsigned int *new_pkt_size){
    
    if(is_pkt_vlan_tagged(ethernet_hdr) == NULL){
        *new_pkt_size = total_pkt_size;
        return ethernet_hdr;
    }

    //Packet is tagged
    // Store src and dst mac addresses in temp variable
    ethernet_hdr_t ethernet_hdr_old;
    memcpy((char*)&ethernet_hdr_old,(char*) ethernet_hdr,2*(sizeof(mac_add_t)));

    // Shrink the ethernet header by sizeof 8021q header
    ethernet_hdr = (ethernet_hdr_t*)((char*)ethernet_hdr + sizeof(vlan_8021q_hdr_t));

    //Set the memory of the mac address to 0
    memset(ethernet_hdr,0,2*sizeof(mac_add_t));

    //Copy data of 2 mac addresses in the original ethernet header
    memcpy((char*)ethernet_hdr,(char*)&ethernet_hdr_old,2*sizeof(mac_add_t));
    *new_pkt_size = total_pkt_size - sizeof(vlan_8021q_hdr_t);
    return ethernet_hdr;
}

void
pending_arp_processing_callback_function(node_t *node,   /*Node on which processing is being done*/
                                        interface_t *oif,/*OIF from which the pkt present in ARP pending entry needs to be sent out*/
                                        arp_entry_t *arp_entry, /*ARP complete entry*/
                                        arp_pending_entry_t *arp_pending_entry){

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)arp_pending_entry->pkt;
    uint32_t pkt_size = arp_pending_entry->pkt_size;

    //Set source and dest mac addresses
    memcpy(ethernet_hdr->src_mac.mac,IF_MAC(oif),sizeof(mac_add_t));
    memcpy(ethernet_hdr->dst_mac.mac,arp_entry->mac_addr.mac,sizeof(mac_add_t));

    send_packet_out((char*) ethernet_hdr,pkt_size,oif);

}

void add_arp_pending_entry(arp_entry_t* arp_entry,
                            arp_processing_fn cb,
                            char* pkt,
                            unsigned int pkt_size){
    
    arp_pending_entry_t * arp_pending_entry = 
            calloc(1,sizeof(arp_pending_entry_t)+ pkt_size);
    init_glthread(&arp_pending_entry->arp_pending_entry_glue);
    arp_pending_entry->cb = cb;
    arp_pending_entry->pkt_size = pkt_size;
    memcpy(arp_pending_entry->pkt,pkt,pkt_size);

    glthread_add_next(&arp_entry->arp_pending_list,
                &arp_pending_entry->arp_pending_entry_glue);

}

void
create_arp_sane_entry(arp_table_t *arp_table, char *ip_addr, 
                       char *pkt, unsigned int pkt_size){

    arp_entry_t *arp_entry = arp_table_lookup(arp_table,ip_addr);
    if(arp_entry){
        // If a complete arp entry exists,we should not create sane arp entries
        assert(arp_entry->is_sane == FALSE);

        //If sane arp entry already exists 
        add_arp_pending_entry(arp_entry,
                            pending_arp_processing_callback_function,
                            pkt,pkt_size);
        return;
    }
    //If arp entry does not exists,create a new one
    arp_entry = calloc(1,sizeof(arp_entry_t));
    strncpy(arp_entry->ip_addr.ip_addr,ip_addr,16);
    arp_entry->ip_addr.ip_addr[16] = '\0';
    init_glthread(&arp_entry->arp_pending_list);
    arp_entry->is_sane = TRUE;
    add_arp_pending_entry(arp_entry,
                        pending_arp_processing_callback_function,
                        pkt,pkt_size);
    bool_t rc = arp_table_entry_add(arp_table,arp_entry,NULL);
    if(rc == FALSE)
        free(arp_entry);
}