#include<stdio.h>

#include "../graph.h"


void layer5_ping_fn(node_t * node,char* dst_ip){
    printf("Trying to ping %s from node %s.....\n",dst_ip,node->node_name);
}

