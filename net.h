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
#define IS_INTF_L3_MODE(intf_ptr) ((intfptr)->intf_nw_prop.is_ipaddr_config)
#define NODE_LOOPBACK_ADDR(nodeptr) (nodeptr)->node_nw_prop.lb_addr.ip_addr

typedef struct ip_add_{
    char ip_addr[16];
}ip_add_t;

typedef struct mac_add_{
    char mac[8];
}mac_add_t;

typedef struct node_nw_prop_{
    //Layer 3 properties
     bool_t is_lb_addr_config ;
     ip_add_t lb_addr;
}node_nw_prop_t;

static inline void init_node_nw_prop(node_nw_prop_t * node_nw_prop){
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr,0,16);
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
    memset(intf_nw_prop->mac.mac,0,8);
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

#endif