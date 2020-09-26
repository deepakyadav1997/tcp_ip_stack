#include<stdio.h>
#include<stdlib.h>
#include "l2switch.h"



void init_mac_table(mac_table_t ** mac_table){
    *mac_table = calloc(1,sizeof(mac_table_t));
    init_glthread(&((*mac_table)->mac_entries));
}

mac_table_entry_t* mac_table_lookup(mac_table_t * mac_table,char* mac){

    glthread_t *current = NULL;
    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries,current){

        mac_table_entry_t* mac_table_entry = mac_entry_glue_to_struct(current);
        if(strncmp(mac_table_entry->mac.mac,mac,sizeof(mac_add_t)) == 0){
            return mac_table_entry;
        }

    }ITERATE_GLTHREAD_END
    return NULL;
}

bool_t mac_table_entry_add(mac_table_t *mac_table,mac_table_entry_t* mac_table_entry){

    mac_table_entry_t * mac_entry_old = mac_table_lookup(mac_table,mac_table_entry->mac.mac);
    if(mac_entry_old && memcmp(mac_entry_old,mac_table_entry,sizeof(mac_table_entry_t)) == 0){
        return FALSE;
        // Responsibility of the caller to free the memory of duplicate arp record
    }

    if(mac_entry_old){
        delete_mac_table_entry(mac_table,mac_table_entry->mac.mac);
    }
    init_glthread(&mac_table_entry->mac_entry_glue);
    glthread_add_next(&mac_table->mac_entries,&mac_table_entry->mac_entry_glue);
    return TRUE;
}



void delete_mac_table_entry(mac_table_t * mac_table,char* mac){

    mac_table_entry_t *mac_entry = mac_table_lookup(mac_table,mac);
    if(!mac_entry){
        return;
    }
    remove_glthread(&mac_entry->mac_entry_glue);
    free(mac_entry);
}

void dump_mac_table(mac_table_t * mac_table){

    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){

        mac_table_entry = mac_entry_glue_to_struct(curr);
        printf("\tMAC : %u:%u:%u:%u:%u:%u   | Intf : %s\n", 
            mac_table_entry->mac.mac[0], 
            mac_table_entry->mac.mac[1],
            mac_table_entry->mac.mac[2],
            mac_table_entry->mac.mac[3], 
            mac_table_entry->mac.mac[4],
            mac_table_entry->mac.mac[5],
            mac_table_entry->oif_name);
    } ITERATE_GLTHREAD_END(&mac_table->mac_entries, curr);
}


static void l2_switch_perform_mac_learning(node_t *node, char *src_mac, char *if_name){
    
    mac_table_t * mac_table = node->node_nw_prop.mac_table;
    mac_table_entry_t *mac_table_entry = calloc(1,sizeof(mac_table_entry_t));
    strncpy(mac_table_entry->mac.mac,src_mac,sizeof(mac_add_t));
    strncpy(mac_table_entry->oif_name,if_name,IF_NAME_SIZE);

    bool_t entry_success =  mac_table_entry_add(mac_table,mac_table_entry);

    if(entry_success == FALSE){
        free(mac_table_entry);
        return;
    }

}

static void  l2_switch_forward_frame(node_t *node,interface_t * recv_interface,
                                     char* pkt,unsigned int pkt_size){

    ethernet_hdr_t * ethernet_hdr = (ethernet_hdr_t*) pkt;

    //Destination address is broadcast
    if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
        send_pkt_flood_l2_intf_only(node,recv_interface,pkt,pkt_size);
        return;
    }
    
    //Check  mac table to forward frame
    mac_table_entry_t * mac_table_entry = mac_table_lookup(node->node_nw_prop.mac_table,ethernet_hdr->dst_mac.mac);

    if(!mac_table_entry){
        send_pkt_flood_l2_intf_only(node,recv_interface,pkt,pkt_size);
        return;
    }
    char* oif_name = mac_table_entry->oif_name;
    interface_t *oif = get_node_if_by_name(node,oif_name);
    if(!oif){
        return;
    }
    send_packet_out(pkt,pkt_size,oif);

}

void l2_switch_recv_frame(interface_t* interface,
                         char* pkt,unsigned int pkt_size){
                            
    node_t *node = interface->att_node;

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t*)pkt;

    char* dest_mac = ethernet_hdr->dst_mac.mac;
    char* src_mac = ethernet_hdr->src_mac.mac;

    l2_switch_perform_mac_learning(node,src_mac,interface->if_name);
    l2_switch_forward_frame(node,interface,pkt,pkt_size);

}