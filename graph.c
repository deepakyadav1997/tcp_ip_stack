#include<stdio.h>
#include "graph.h"
#include "net.h"
#include<assert.h>
#include<stdlib.h>
#include<string.h>

graph_t* create_new_graph(char* topology_name){
    graph_t* graph = calloc(1,sizeof(graph_t));
    strncpy(graph->topology_name,topology_name,MAX_GRAPH_NAME);
    graph->topology_name[MAX_GRAPH_NAME-1] = '\0';
    init_glthread(&graph->node_list);
    return graph;
}
node_t* create_graph_node(graph_t* graph,char* node_name){
    node_t *node = calloc(1,sizeof(node_t));
    strncpy(node->node_name,node_name,NODE_NAME_SIZE);
    node->node_name[NODE_NAME_SIZE-1] = '\0'; // so that string ends
    init_node_nw_prop(&node->node_nw_prop);
    glthread_add_next(&graph->node_list,&node->graph_glue);
    return node;
}

void insert_link_between_two_nodes(node_t*  node1,
                                    node_t* node2,
                                    char* from_if_name,
                                    char* to_if_name,
                                    unsigned int cost){

    link_t *link = calloc(1,sizeof(link_t));
    // Set interfaces names
    strncpy(link->intf1.if_name,from_if_name,IF_NAME_SIZE);
    link->intf1.if_name[IF_NAME_SIZE-1] = '\0';
    strncpy(link->intf2.if_name,to_if_name,IF_NAME_SIZE);
    link->intf2.if_name[IF_NAME_SIZE-1] = '\0';
    // Set owning nodes on interfaces
    link->intf1.att_node = node1;
    link->intf2.att_node = node2;
    //set up back links on interfaces
    link->intf1.link = link;
    link->intf2.link = link;
    link->cost = cost;
    // connect interfaces to empty slots in the  nodes 
    int empty_slot =  get_node_intf_available_slot(node1);
    assert(empty_slot != -1);
    node1->intf[empty_slot] = &link->intf1;
    empty_slot = get_node_intf_available_slot(node2);
    assert(empty_slot != -1);
    node2->intf[empty_slot] = &link->intf2;
    //init interface properties
    init_intf_prop(&link->intf1.intf_nw_prop);
    init_intf_prop(&link->intf2.intf_nw_prop);
    //assign mac address to interface randomly
    interface_assign_mac_address(&link->intf1);
    interface_assign_mac_address(&link->intf2);
}
void dump_graph(graph_t *graph){

    glthread_t *curr;
    node_t *node;
    
    printf("Topology Name = %s\n", graph->topology_name);

    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){
    
        node = graph_glue_to_node(curr);
        dump_node(node);    
    
       
    } ITERATE_GLTHREAD_END;
}

void dump_node(node_t *node){

    unsigned int i = 0;
    interface_t *intf;

    printf("Node Name = %s : \n", node->node_name);
    for( ; i < MAX_INTERFACES_PER_NODE; i++){
        
        intf = node->intf[i];
        if(!intf) break;
        dump_interface(intf);
    }
}

void dump_interface(interface_t *interface){

   link_t *link = interface->link;
   node_t *nbr_node = get_nbr_node(interface);

   printf(" Local Node : %s, Interface Name = %s, Nbr Node %s, cost = %u\n", 
            interface->att_node->node_name, 
            interface->if_name, nbr_node->node_name, link->cost); 
}
