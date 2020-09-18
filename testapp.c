#include "graph.h"
#include "net.h"
extern graph_t *build_first_topo();

int 
main(int argc, char **argv){

    graph_t *topo = build_first_topo();
    //dump_graph(topo);
    //dump_nw_graph(topo);
    printf("Int value is %u\n",convert_ip_from_str_to_int("192.1.2.2"));
    char string_ip[32];
    convert_ip_from_int_to_str(convert_ip_from_str_to_int("192.1.4.2"),string_ip);
    printf("String value is : %s\n",string_ip);
    return 0;
}