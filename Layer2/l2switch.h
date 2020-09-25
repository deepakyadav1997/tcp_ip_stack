#ifndef __L2_SWITCH__
#define __L2_SWITCH__

#include  "../net.h"
#include  "../graph.h"
#include  "layer2.h"
#include "../communication.h"

typedef struct mac_table_entry_{
    mac_add_t mac;
    char oif_name[IF_NAME_SIZE];
    glthread_t mac_entry_glue;

}mac_table_entry_t;

GLTHREAD_TO_STRUCT(mac_entry_glue_to_struct,mac_table_entry_t,mac_entry_glue,glthreadptr)

typedef struct mac_table_{
    glthread_t mac_entries;
}mac_table_t;


bool_t mac_table_entry_add(mac_table_t *mac_table,mac_table_entry_t* mac_table_entry);
mac_table_entry_t* mac_table_lookup(mac_table_t * mac_table,char* mac);
void delete_mac_table_entry(mac_table_t * mac_table,char* mac);
void dump_mac_table(mac_table_t * mac_table);

void l2_switch_recv_frame(interface_t* interface,
                         char* pkt,unsigned int pkt_size);

#endif