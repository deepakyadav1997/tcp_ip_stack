#include "graph.h"
#include "net.h"
#include "utils.h"
#include "CommandParser/libcli.h"
#include "cmdcodes.h"
#include<unistd.h>

extern graph_t *build_first_topo();
graph_t *topo = NULL;

int main(int argc, char **argv){
    nw_init_cli();
    topo = build_first_topo();
    // dump_graph(topo);
    // dump_nw_graph(topo);
    // printf("Int value is %u\n",convert_ip_from_str_to_int("192.1.2.2"));
    // char string_ip[32];
    // convert_ip_from_int_to_str(convert_ip_from_str_to_int("192.1.4.2"),string_ip);
    // printf("String value is : %s\n",string_ip);
    // apply_mask("192.168.21.12",26,string_ip);
    // printf("Msked value %s\n",string_ip);
    //printf("Interface name %s\n",node_get_matching_subnet_interface(graph_glue_to_node(topo->node_list.right->right),"30.1.1.222")->if_name);
    sleep(2); //    time for reciever threead to start
    node_t *snode = get_node_by_node_name(topo,"R0_re");
    interface_t *oif = get_node_if_by_name(snode,"eth0/0");
    char *message ="Hello. This is the first message sent by our setup\n";
    send_packet_out(message,strlen(message),oif);
    start_shell();
    return 0;
}