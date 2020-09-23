#ifndef __NET__
#define __NET__
#include "utils.h"
#include<memory.h>

//declarations so that we can use the pointers of these type
typedef struct graph_ graph_t;
typedef struct interface_ interface_t;
typedef struct node_ node_t;

//shorthand macros
#define IF_MAC(intfptr) ((intfptr)->intf_nw_prop.mac.mac)
#define IF_IP(intfptr)  ((intfptr)->intf_nw_prop.ip_add.ip_addr)
#define NODE_LOOPBACK_ADDR(nodeptr) (nodeptr)->node_nw_prop.lb_addr.ip_addr
#define IS_INTF_L3_MODE(intf_ptr)    (intf_ptr->intf_nw_prop.is_ipaddr_config == TRUE)

typedef struct ip_add_{
    char ip_addr[16];
}ip_add_t;

typedef struct mac_add_{
    unsigned char mac[6];
}mac_add_t;

//Forward declaration
typedef struct arp_table_ arp_table_t;

typedef struct node_nw_prop_{
    // Layer 2 properties
    arp_table_t * arp_table;


    //Layer 3 properties
     bool_t is_lb_addr_config ;
     ip_add_t lb_addr;
}node_nw_prop_t;

extern void init_arp_table(arp_table_t ** arp_table);

static inline void init_node_nw_prop(node_nw_prop_t * node_nw_prop){
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr,0,16);
    init_arp_table(&(node_nw_prop->arp_table));
}

typedef struct intf_nw_prop_{
    //L2 properties
    mac_add_t mac;

    //L3 properties
    bool_t is_ipaddr_config;
    ip_add_t ip_add;
    char mask;
}intf_nw_prop_t;

static inline void init_intf_prop(intf_nw_prop_t* intf_nw_prop){
    intf_nw_prop->is_ipaddr_config = FALSE;
    intf_nw_prop->mask = 32;
    memset(intf_nw_prop->mac.mac,0,6);
    memset(intf_nw_prop->ip_add.ip_addr,0,16);

}

//configuration api's
bool_t node_set_loopback_address(node_t *node, char *ip_addr);
bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask);
bool_t node_unset_intf_ip_address(node_t *node, char *local_if);
void interface_assign_mac_address(interface_t *interface);

/*Dumping Functions to dump network information
 * on nodes and interfaces*/
void dump_nw_graph(graph_t *graph);
void dump_node_nw_props(node_t *node);
void dump_intf_props(interface_t *interface);

//IP address manipulation
unsigned int convert_ip_from_str_to_int(char *ip_addr);
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer);
unsigned int binary_to_int(int* binary,unsigned int end,int start);
void int_to_binary(unsigned int number,int* array,unsigned int size);

interface_t * node_get_matching_subnet_interface(node_t *node, char *ip_addr);

//total_buffer_size =  MAX_PACKET_BUFFER_SIZE - IF_NAME_SIZE
char* pkt_buffer_shift_right(char* pkt,unsigned int pkt_size,unsigned int total_buffer_size);

#endif