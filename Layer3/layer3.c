#include "layer3.h"
#include "../net.h"
#include "../Layer2/layer2.h"

#include<arpa/inet.h>

void init_rt_table(rt_table_t **rt_table){
    *rt_table = calloc(1,sizeof(rt_table_t));
    init_glthread(&((*rt_table)->route_list));
}

void rt_table_add_direct_route(rt_table_t *rt_table,
                               char *dst, char mask){

    rt_table_add_route(rt_table,dst,mask,0,0);
}

void rt_table_add_route(rt_table_t *rt_table, 
                        char *dst, char mask,
                        char *gw, char *oif){

    assert(rt_table != NULL);

    int dest_ip = 0;
    char dest_ip_masked[16];
    apply_mask(dst,mask,dest_ip_masked);
    inet_pton(AF_INET,dest_ip_masked,&dest_ip);
    l3_route_t* l3_route =  rt_table_lookup(rt_table,dest_ip_masked,mask);

    //duplicate entry
    if(l3_route != NULL){
        printf("Route already exists. Skipping...\n");
        ///assert(!l3_route);
    }
    

    l3_route = calloc(1,sizeof(l3_route_t));
    strncpy(l3_route->dest,dest_ip_masked,sizeof(ip_add_t));
    l3_route->mask = mask;
    if(gw && oif){
        l3_route->is_direct = FALSE;
        memcpy(l3_route->gw_ip,gw,sizeof(ip_add_t));
        memcpy(l3_route->oif,oif,IF_NAME_SIZE);
        
    }
    else{
        l3_route->is_direct = TRUE;
        memset(l3_route->gw_ip,0,sizeof(ip_add_t));
        memset(l3_route->oif,0,IF_NAME_SIZE);
    }
    l3_route->oif[IF_NAME_SIZE-1] = '\0';
    l3_route->gw_ip[15] = '\0';
    init_glthread(&l3_route->rt_glue);
    glthread_add_next(&rt_table->route_list,&l3_route->rt_glue);

}

static void dump_l3_route(l3_route_t* l3_route){
    
    printf("\nDest: %s/%d",l3_route->dest,l3_route->mask);
    if(l3_route->is_direct == FALSE){
        printf("\tGateway IP:%s  OIF:%s\n",l3_route->gw_ip,l3_route->oif);
    }
}

void dump_rt_table(rt_table_t *rt_table){

    printf("\nDumping routing table\n");
    glthread_t* current;
    l3_route_t * l3_route = NULL;
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        dump_l3_route(l3_route);
    }ITERATE_GLTHREAD_END;

}

l3_route_t * rt_table_lookup(rt_table_t *rt_table,char *ip_addr,
                            char mask){

    glthread_t *current = NULL;
    l3_route_t *l3_route = NULL;

    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        if(strncmp(l3_route->dest,ip_addr,sizeof(ip_add_t)) == 0 && 
                                            l3_route->mask == mask){

            return l3_route;                                    
        }

    }ITERATE_GLTHREAD_END;

    return NULL;

}

void delete_rt_table_entry(rt_table_t *rt_table,char *ip_addr,
                           char mask){

    char dest_ip_masked[16];
    apply_mask(ip_addr,mask,dest_ip_masked);
    l3_route_t * l3_route = rt_table_lookup(rt_table,dest_ip_masked,mask);
    if(l3_route == NULL){
        return;
    }
    remove_glthread(&l3_route->rt_glue);
    free(l3_route);
}

l3_route_t * l3rib_lookup_lpm(rt_table_t *rt_table,
                              unsigned int dest_ip){
    
    char subnet[16];
    char destination_ip_str[16];
    char longest_mask = 0;
    l3_route_t *l3_route = NULL,
               *lpm_route = NULL,
               *default_route = NULL;

    dest_ip = htonl(dest_ip);
    inet_ntop(AF_INET,&dest_ip,destination_ip_str,sizeof(ip_add_t));
    //convert_ip_from_int_to_str(dest_ip,destination_ip_str);

    destination_ip_str[sizeof(ip_add_t) - 1] = '\0';
    //printf("Unmasked ip %s\n",destination_ip_str);
    glthread_t *current = NULL;
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        memset(subnet,0,16);
        apply_mask(destination_ip_str,l3_route->mask,subnet);
        //printf("MAsked ip %s  destip %s\n",subnet,destination_ip_str);
        if(strncmp("0.0.0.0",l3_route->dest,sizeof(ip_add_t)) == 0 && l3_route->mask == 0){
                default_route = l3_route;
            
        }
        else if(strncmp(subnet,l3_route->dest,sizeof(ip_add_t)) == 0){
            if(l3_route->mask > longest_mask){
                longest_mask = l3_route->mask;
                lpm_route = l3_route;
            }
        }
    }ITERATE_GLTHREAD_END;

    return lpm_route ? lpm_route:default_route;
}

static bool_t
l3_is_direct_route(l3_route_t *l3_route){
    return l3_route->is_direct;
}

bool_t
is_layer3_local_delivery(node_t *node, unsigned int dst_ip){

    char dst_ip_str[16];
    
    dst_ip = htonl(dst_ip);
    inet_ntop(AF_INET,&dst_ip,dst_ip_str,16);
    dst_ip_str[15] = '\0';

    //Destination ip address is the loopback address of the node
    if(strncmp(NODE_LOOPBACK_ADDR(node),dst_ip_str,16) == 0){
        return TRUE;
    }

    //Check for interfaces configured with an ip address
    for(int i = 0;i < MAX_INTERFACES_PER_NODE;i++){
        if(node->intf[i] == NULL){
            break;
        }
        //if ip is not configured, skip
        if(node->intf[i]->intf_nw_prop.is_ipaddr_config == FALSE){
            continue;
        }
        //Check if dst ip matches any of the interface ip's exactly
        if(strncmp(IF_IP(node->intf[i]),dst_ip_str,16) == 0){
            return TRUE;
        }
    }
    return FALSE;

}

static void
layer3_ip_pkt_recv_from_layer2(node_t* node,interface_t *interface_t,
                                ip_hdr_t* pkt,uint32_t pkt_size){

    unsigned int dest_ip = pkt->dst_ip;
    l3_route_t* l3_route = l3rib_lookup_lpm(node->node_nw_prop.rt_table,dest_ip);
    dest_ip = htonl(dest_ip);
    char dest_ip_str[16];
    inet_ntop(AF_INET,&dest_ip,dest_ip_str,16);

    //No entry in routing table. drop the packet
    if(!l3_route){
        printf("Node %s: Cannot route to ip %s %d\n",node->node_name,dest_ip_str,dest_ip);
        return;
    }

    //local route
    if(l3_is_direct_route(l3_route) == TRUE){

        //Case 1: Local delivery, pkt is destined for the same machine
        if(is_layer3_local_delivery(node,pkt->dst_ip) == TRUE){
            switch (pkt->protocol)
            {
            case ICMP_PRO:
                printf("IP address %s has been pinged successfully\n",dest_ip_str);
                break;
            
            default:
                break;
            }
        }
        //Case 2: Direct host delivery, reciever is directly connected to the router
        else{
            demote_pkt_to_layer2(node,
                                pkt->dst_ip,
                                NULL,
                                (char*)pkt,
                                pkt_size,
                                ETH_IP);
        }

    }
    //forward to gateway
    else{
            pkt->ttl--;
            //Avoids infinite loops
            if(pkt->ttl == 0){
                printf("TTL reached 0,dropping pkt\n");
                return;
            }
            unsigned int next_hop_ip = 0;
            inet_pton(AF_INET,l3_route->gw_ip,&next_hop_ip);
            next_hop_ip = htonl(next_hop_ip);
            demote_pkt_to_layer2(node,
                                next_hop_ip,
                                l3_route->oif,
                                (char*)pkt,
                                pkt_size,
                                ETH_IP);
    }

}

static void 
_l3_pkt_recv_from_layer2(node_t* node,interface_t * interface,
                        char*pkt,uint32_t pkt_size,int l3_protocol){

    switch (l3_protocol)
    {
    case ETH_IP:
        layer3_ip_pkt_recv_from_layer2(node,interface,(ip_hdr_t*)pkt,pkt_size);
        break;
    
    default:
        break;
    }

}
static void
layer3_pkt_recv_from_top(node_t *node, 
                        char *pkt, unsigned int size,
                        int protocol_number,            
                        unsigned int dest_ip_address){

    ip_hdr_t ip_hdr; 
    initialize_ip_hdr(&ip_hdr);
    ip_hdr.protocol = protocol_number; 
    unsigned int addr_int = 0;

    inet_pton(AF_INET,NODE_LOOPBACK_ADDR(node),&addr_int);
    addr_int = htonl(addr_int);

    ip_hdr.src_ip = addr_int;
    ip_hdr.dst_ip = dest_ip_address;   
    ip_hdr.total_length = (unsigned short) ip_hdr.ihl +
                          (unsigned short) (size/4)   +
                          (unsigned short) ((size % 4) ? 1 : 0);    //taking ceil value of size/4

    char dest_ip_str[16];
    //dest_ip_address = ntohl(dest_ip_address);
    convert_ip_from_int_to_str(dest_ip_address,dest_ip_str);
    l3_route_t *l3_route = l3rib_lookup_lpm(node->node_nw_prop.rt_table,dest_ip_address);
    if(!l3_route){
        printf("No route from node %s to %s %d\n",node->node_name,dest_ip_str,dest_ip_address);
        return;
    }
    char* new_pkt = NULL;
    unsigned int new_pkt_size = 0;                
    new_pkt_size = ip_hdr.total_length * 4;
    new_pkt = calloc(1,MAX_PACKET_BUFFER_SIZE);

    memcpy(new_pkt,(char*)&ip_hdr,ip_hdr.ihl*4);

    if(pkt && size){
        memcpy(new_pkt + ip_hdr.ihl * 4,pkt,size);
    }
    unsigned int nex_hop_ip;
    bool_t is_direct_route = l3_is_direct_route(l3_route); 
    if(is_direct_route == FALSE){
        //forwarding to gateway ip
        inet_pton(AF_INET,l3_route->gw_ip,&nex_hop_ip);
        nex_hop_ip = htonl(nex_hop_ip);
    }
    else{
        //Direct host delivery or self ping
        // layer 2 will differentiate b/w the 2
        nex_hop_ip = dest_ip_address;
    }
    //Create room to append ethernet header or any other headers by the lower levels
    char* shifted_pkt = pkt_buffer_shift_right(new_pkt,new_pkt_size,MAX_PACKET_BUFFER_SIZE);

    demote_pkt_to_layer2(node,
                        nex_hop_ip,
                        is_direct_route ? 0 : l3_route->oif,
                        shifted_pkt,new_pkt_size,
                        ETH_IP);
    free(new_pkt);
}

void 
promote_pkt_to_layer3(node_t *node,                
                      interface_t *interface,           
                      char *pkt, unsigned int pkt_size, 
                      int L3_protocol_number){

    _l3_pkt_recv_from_layer2(node,interface,pkt,pkt_size,L3_protocol_number);
}
void
demote_packet_to_layer3(node_t *node, 
                        char *pkt, unsigned int size,
                        int protocol_number,            /*L4 or L5 protocol type*/
                        unsigned int dest_ip_address){

    layer3_pkt_recv_from_top(node,pkt,size,protocol_number,dest_ip_address);                        
}