#ifndef __L3__
#define __L3__

#include "../glthreads_lib/glthread.h"
#include "../net.h"
#include "../graph.h"

#include<stdio.h>
#include<stdlib.h>

typedef struct rt_table_{
    glthread_t route_list;
}rt_table_t;

typedef struct l3_route_{
    
    char dest[16];          //key
    char mask;              //key
    bool_t is_direct;       //Is the destination a local subnet,i.e directly connected
                            //If is_direct is true, the below 2 fields have no meaning
    char gw_ip[16];         // Gateway ip 
    char oif[IF_NAME_SIZE]; // outgoing interface name
    glthread_t rt_glue;
}l3_route_t;
GLTHREAD_TO_STRUCT(rt_glue_to_l3_route,l3_route_t,rt_glue,glthreadptr);

void init_rt_table(rt_table_t **rt_table);

l3_route_t * rt_table_lookup(rt_table_t *rt_table,
                            char *ip_addr,
                            char mask);

void clear_rt_table(rt_table_t *rt_table);

void delete_rt_table_entry(rt_table_t *rt_table, 
                           char *ip_addr,
                           char mask);

void rt_table_add_route(rt_table_t *rt_table, 
                        char *dst, char mask,
                        char *gw, char *oif);

void rt_table_add_direct_route(rt_table_t *rt_table,
                               char *dst, char mask);

void dump_rt_table(rt_table_t *rt_table);

l3_route_t * l3rib_lookup_lpm(rt_table_t *rt_table,
                              unsigned int dest_ip);

#endif
