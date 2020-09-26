#include<stdio.h>
#include<assert.h>
#include<stdlib.h>

#include "graph.h"
#include "net.h"
#include "utils.h"

bool_t node_set_loopback_address(node_t *node, char *ip_addr){
    assert(ip_addr);
    assert(node);
    strncpy(NODE_LOOPBACK_ADDR(node),ip_addr,16);
    NODE_LOOPBACK_ADDR(node)[16] = '\0';
    node->node_nw_prop.is_lb_addr_config = TRUE;
}

bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask){
    interface_t * interface = get_node_if_by_name(node,local_if);
    if(!interface){
        assert(0);
    }
    strncpy(IF_IP(interface),ip_addr,16);
    IF_IP(interface)[16] = '\0';
    interface->intf_nw_prop.mask = mask;
    interface->intf_nw_prop.is_ipaddr_config = TRUE;
    return TRUE;

}
bool_t node_unset_intf_ip_address(node_t *node, char *local_if){
    interface_t * interface = get_node_if_by_name(node,local_if);
    if(!interface){
        assert(0);
    }
    memset(IF_IP(interface),0,16);
    IF_IP(interface)[16] = '\0';
    interface->intf_nw_prop.mask = 0;
    interface->intf_nw_prop.is_ipaddr_config = FALSE;
    return TRUE;   
}
// //Just random number generator 
// static unsigned int hash_code(void *ptr,unsigned int size){
//     unsigned int value = 0,i = 0;
//     char* str = (char*)ptr;
//     while(i<size){
//         value+=*str;
//         value*=22;
//         str++;
//         i++;
//     }
//     return value;
// }

void interface_assign_mac_address(interface_t *interface){
    node_t* node = interface->att_node;
    assert(node);
    unsigned int hash_code_val = rand();
    // hash_code_val = hash_code(node->node_name,NODE_NAME_SIZE);
    // printf("Hash code %u\n",hash_code_val);
    // hash_code_val *= hash_code(interface->if_name,IF_NAME_SIZE);
    memset(IF_MAC(interface),0,sizeof(IF_MAC(interface)));
    //memcpy(IF_MAC(interface),(char*)&hash_code_val,sizeof(unsigned int));
   
    IF_MAC(interface)[0] = hash_code_val%255;
    IF_MAC(interface)[1] = hash_code_val%254;
    IF_MAC(interface)[2] = hash_code_val%253;
    IF_MAC(interface)[3] = hash_code_val%252;
    IF_MAC(interface)[4] = hash_code_val%251;
    IF_MAC(interface)[5] = hash_code_val%250;
    printf("Mac is set to %d:%d:%d:%d:%d:%d\n",IF_MAC(interface)[0],
                                               IF_MAC(interface)[1],
                                               IF_MAC(interface)[2],
                                               IF_MAC(interface)[3],
                                               IF_MAC(interface)[4],
                                               IF_MAC(interface)[5]);
}


void dump_node_nw_props(node_t *node){

    printf("\nNode Name = %s\n", node->node_name);
   // printf("\t node flags : %u", node->node_nw_prop.flags);
    if(node->node_nw_prop.is_lb_addr_config){
        printf("\t  lo addr : %s/32\n", NODE_LOOPBACK_ADDR(node));
    }
}

void dump_intf_props(interface_t *interface){

    dump_interface(interface);

    if(interface->intf_nw_prop.is_ipaddr_config){
        printf("\t IP Addr = %s/%u", IF_IP(interface), interface->intf_nw_prop.mask);
    }
    else{
         printf("\t IP Addr = %s/%u", "Nil", 0);
    }

    printf("\t MAC : %x:%x:%x:%x:%x:%x\n", 
        IF_MAC(interface)[0], IF_MAC(interface)[1],
        IF_MAC(interface)[2], IF_MAC(interface)[3],
        IF_MAC(interface)[4], IF_MAC(interface)[5]);
    if(!IS_INTF_L3_MODE(interface))
        printf("\t L2 Mode : %s\n",intf_l2_mode_str(interface->intf_nw_prop.intf_l2_mode));
    printf("VLANs configured on the interface ----\n");
    for(int i = 0;i < MAX_VLAN_MEMBERSHIP;i++){
        if(interface->intf_nw_prop.vlans[i] != 0){
            printf("Vlan id: %d\n",interface->intf_nw_prop.vlans[i]);
        }
    }
}

void dump_nw_graph(graph_t *graph){

    node_t *node;
    glthread_t *curr;
    interface_t *interface;
    unsigned int i;
    
    printf("Topology Name = %s\n", graph->topology_name);

    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){

        node = graph_glue_to_node(curr);
        dump_node_nw_props(node);
        for( i = 0; i < MAX_INTERFACES_PER_NODE; i++){
            interface = node->intf[i];
            if(!interface) break;
            dump_intf_props(interface);
        }
    } ITERATE_GLTHREAD_END(&graph->node_list, curr);

}

static unsigned int power(unsigned int a, unsigned int b){
    double result = 1;
    while(b > 0){
        result *= a;
        b--;
    }
    return result;
}

void int_to_binary(unsigned int number,int* array,unsigned int size){

    for(int i = size-1;i>=0;i--){
        array[i] = number%2;
        number = number/2;
    }
}

unsigned int binary_to_int(int* binary,unsigned int end,int start){
    unsigned int result = 0;
    unsigned int current_power_of_two = 1;
    for (int i = end;i>=start;i--){
        result += binary[i]*current_power_of_two;
        current_power_of_two *= 2;
    }
    return result;
}

unsigned int convert_ip_from_str_to_int(char *ip_addr){
    int max_len = 16; // IPv4 string cant be longer than 15 chars + \0
    if(strlen(ip_addr) > 16){
        printf("Error! IP address string invalid.\n");
        assert(0);
    }
    int sum[32],part_sum[8];
    int temp_sum = 0;
    unsigned int final_sum = 0;
    int current_octet = 3;

    for(int i = 0;i<=strlen(ip_addr);i++){
        if(ip_addr[i] == '.' || ip_addr[i] == '\0'){
            final_sum += (temp_sum*power(256,current_octet));
            //printf("%u %u\n",temp_sum,power(256,current_octet));
            current_octet--;
            temp_sum = 0;
            if(ip_addr[i] == '\0' || current_octet == -1)
                break;
        }
        else{
            temp_sum = temp_sum*10+(ip_addr[i]-48); // converting char to digits
        }
    }
    return final_sum;
}
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){
    int binary[32];
    int_to_binary(ip_addr,binary,32);
    // for(int i = 0;i<32;i++){
    //     printf("%d",binary[i]);
    // }
    int first_octet = binary_to_int(binary,7,0);
    int second_octet = binary_to_int(binary,15,8);
    int third_octet = binary_to_int(binary,23,16);
    int fourth_octet = binary_to_int(binary,31,24);
    sprintf(output_buffer,"%d.%d.%d.%d\0",first_octet,second_octet,third_octet,fourth_octet);
}

interface_t * node_get_matching_subnet_interface(node_t *node, char *ip_addr){
    interface_t* current_interface;
    for(int i = 0;i<MAX_INTERFACES_PER_NODE;i++){
        current_interface = node->intf[i];
        if(current_interface == NULL){
            return NULL;
        }
        char mask = current_interface->intf_nw_prop.mask;
        char masked_ip[32];
        apply_mask(ip_addr,mask,masked_ip);
        char masked_interface_ip[32];
        char* interface_ip = current_interface->intf_nw_prop.ip_add.ip_addr;
        apply_mask(interface_ip,mask,masked_interface_ip);
        if(strncmp(masked_interface_ip,masked_ip,16) == 0){
            return current_interface;
        }
    }
    return NULL;
}

char* pkt_buffer_shift_right(char* pkt,unsigned int pkt_size,unsigned int total_buffer_size){
    char* temp = calloc(1,pkt_size);
    memcpy(temp,pkt,pkt_size);
    memset(pkt,0,total_buffer_size);

    pkt = pkt + total_buffer_size - pkt_size;
    memcpy(pkt,temp,pkt_size);
    return pkt;
}