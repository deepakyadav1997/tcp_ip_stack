#include<stdio.h>
#include<stdlib.h>
#include "l2switch.h"
#include "layer2.h"



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

static bool_t l2_switch_send_pkt_out(char *pkt, unsigned int pkt_size,
                                    interface_t *oif){

    assert(!IS_INTF_L3_MODE(oif)); 
    vlan_8021q_hdr_t *vlan_8021q_hdr = is_pkt_vlan_tagged((ethernet_hdr_t*) pkt);
    intf_l2_mode_t oif_mode = oif->intf_nw_prop.intf_l2_mode;

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    switch(oif_mode){
        case ACCESS:{
            if(vlan_8021q_hdr == NULL){ 
                //case 1
                if(get_access_intf_operating_vlan_id(oif) == -1){
                    printf("Error! interface in access mode must have vlan configured.\n");
                    assert(0);
                }
                else{ //case 2
                    return FALSE;
                }
                
            }
            else{ // pkt is vlan tagged
                if(get_access_intf_operating_vlan_id(oif) == -1){ //case 4
                    return FALSE;
                }
                if(get_access_intf_operating_vlan_id(oif) == GET_802_1Q_VLAN_ID(vlan_8021q_hdr)){ //case 3
                    unsigned int new_pkt_size = 0;
                    char *pkt = (char*) untag_pkt_with_vlan_id(ethernet_hdr,pkt_size,&new_pkt_size);
                    send_packet_out(pkt,new_pkt_size,oif);
                    return TRUE;
                }
                else{
                    return FALSE;
                }
            }
            break;
        }
        case TRUNK:{ //case 5
            int pkt_vlan_id = 0;
            if(vlan_8021q_hdr != NULL){
                pkt_vlan_id = GET_802_1Q_VLAN_ID(vlan_8021q_hdr);
                if(pkt_vlan_id > 0 && is_trunk_interface_vlan_enabled(oif,pkt_vlan_id) == TRUE){
                    send_packet_out(pkt,pkt_size,oif);
                    return TRUE;
                }
                else{
                    return FALSE;
                }
                
            }
            break;
        }
        default:return FALSE;
    }

}

static bool_t l2_switch_flood_pkt_out(node_t *node, interface_t *exempted_intf,
                                      char *pkt, unsigned int pkt_size){

    interface_t *oif = NULL;
    char* pkt_copy = NULL;
    char* temp_pkt = calloc(1,MAX_PACKET_BUFFER_SIZE);
    //temp memory for packet with space on the right 
    pkt_copy = temp_pkt + MAX_PACKET_BUFFER_SIZE - pkt_size;
    for(int i = 0;i < MAX_INTERFACES_PER_NODE; i++){
        oif = node->intf[i];
        if(oif == NULL){
            break;
        }
        if(oif == exempted_intf || IS_INTF_L3_MODE(oif)){
            continue;
        }
        memcpy(pkt_copy,pkt,pkt_size);
        l2_switch_send_pkt_out(pkt_copy,pkt_size,oif);
        
    }
    free(temp_pkt);
}

static void  l2_switch_forward_frame(node_t *node,interface_t * recv_interface,
                                     char* pkt,unsigned int pkt_size){

    ethernet_hdr_t * ethernet_hdr = (ethernet_hdr_t*) pkt;

    //Destination address is broadcast
    if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
        l2_switch_flood_pkt_out(node,recv_interface,pkt,pkt_size);
        return;
    }
    
    //Check  mac table to forward frame
    mac_table_entry_t * mac_table_entry = mac_table_lookup(node->node_nw_prop.mac_table,ethernet_hdr->dst_mac.mac);

    if(!mac_table_entry){
        l2_switch_flood_pkt_out(node,recv_interface,pkt,pkt_size);
        return;
    }
    char* oif_name = mac_table_entry->oif_name;
    interface_t *oif = get_node_if_by_name(node,oif_name);
    if(!oif){
        return;
    }
    l2_switch_send_pkt_out(pkt,pkt_size,oif);

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