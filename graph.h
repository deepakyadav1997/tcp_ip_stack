#ifndef __GRAPH__
#define __GRAPH__
#include "glthreads_lib/glthread.h"
#include "communication.h"
#include<assert.h>
#include<string.h>

#define IF_NAME_SIZE 32
#define NODE_NAME_SIZE 32
#define MAX_INTERFACES_PER_NODE 5
#define MAX_GRAPH_NAME 32
#include "net.h"

typedef struct link_ link_t;
typedef struct node_ node_t;



typedef struct interface_{
    char if_name[IF_NAME_SIZE];
    struct node_ *att_node;  //owning node
    struct link_ *link;
    intf_nw_prop_t intf_nw_prop;
}interface_t;

 struct link_{
    interface_t intf1;
    interface_t intf2;
    unsigned int cost;
};

typedef struct graph_{
    char topology_name[MAX_GRAPH_NAME];
    glthread_t node_list;
}graph_t;

struct node_{
    char node_name[NODE_NAME_SIZE];
    interface_t* intf[MAX_INTERFACES_PER_NODE];
    glthread_t graph_glue;
    unsigned int udp_port_number;
    int udp_socket_fd;
    node_nw_prop_t node_nw_prop;
};
GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t, graph_glue,glthreadptr);

//Graph creation functions
graph_t* create_new_graph(char* topology_name);
node_t* create_graph_node(graph_t* graph,char* node_name);
void insert_link_between_two_nodes(node_t*  node1,
                                    node_t* node2,
                                    char* from_if_name,
                                    char* to_if_name,
                                    unsigned int cost);

//Display functions
void dump_graph(graph_t *graph);
void dump_node(node_t *node);
void dump_interface(interface_t *interface);


//Helper functions
static inline node_t * get_nbr_node(interface_t *interface){
    assert(interface->link);
    assert(interface->att_node);
    if(interface == &interface->link->intf1){
        return interface->link->intf2.att_node;
    }
    else if(interface == &interface->link->intf2){
        return interface->link->intf1.att_node;
    }
    return NULL;
}
static inline int get_node_intf_available_slot(node_t *node){
    for(int index =0;index<MAX_INTERFACES_PER_NODE;index++){
        if(node->intf[index]){
            continue;
        }
        return index;
    }    
    return -1;
}
static inline interface_t * get_node_if_by_name(node_t *node, char *if_name){
    for(int i = 0;i<MAX_INTERFACES_PER_NODE;i++){
        if(!node->intf[i])
            return NULL;
        if(strncmp(node->intf[i]->if_name,if_name,IF_NAME_SIZE) == 0){
            return node->intf[i];
        }
    }
    return NULL;
}

static inline node_t * get_node_by_node_name(graph_t *topo, char *node_name){
    assert(topo);
    glthread_t* head = topo->node_list.right;
    while(head){
        node_t* head_node = graph_glue_to_node(head);
        if(strncmp(head_node->node_name,node_name,NODE_NAME_SIZE) == 0){
            return head_node;
        }
        head = head->right;
    }
    return NULL;
}
#endif