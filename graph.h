#ifndef __GRAPH__
#define __GRAPH__
#include "glthreads_lib/glthread.h"

#define IF_NAME_SIZE 32
#define NODE_NAME_SIZE 32
#define MAX_INTERFACES_PER_NODE 5


typedef struct link_ link_t;
typedef struct node_ node_t;

typedef struct interface_{
    char if_name[IF_NAME_SIZE];
    struct node_ *att_node;  //owning node
    struct link_ *link;
    
}interface_t;

 struct link_{
    interface_t intf1;
    interface_t intf2;
};

typedef struct graph_{
    char topology_name[32];
    glthread_t node_list;
}graph_t;

struct node_{
    char node_name[NODE_NAME_SIZE];
    interface_t* intf[MAX_INTERFACES_PER_NODE];
    glthread_t graph_glue;
};
#endif