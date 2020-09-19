#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"
#include "cmdcodes.h"
#include "graph.h"
#include<stdio.h>


extern graph_t *topo;

static int show_nw_topology_handler(param_t *param,ser_buff_t *tlv_buffer,op_mode enable_or_disable){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buffer);

    switch (CMDCODE)
    {
    case CMDCODE_SHOW_TOPOLOGY:
        dump_nw_graph(topo);
        break;
    
    default:
        break;
    }
}

static int arp_handler(param_t *param, ser_buff_t *tlv_buf,op_mode enable_or_disable){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buf);
    char *node_name = NULL;
    char *ip_address = NULL;
    tlv_struct_t * tlv;
    char *ip_addr = NULL;
    TLV_LOOP_BEGIN(tlv_buf,tlv){
        if(strncmp(tlv->leaf_id,"node_name",strlen("node_name")) == 0){
            node_name = tlv->value;
        }
        else if(strncmp(tlv->leaf_id,"ip-address",strlen("ip-address")) == 0){
            ip_address =tlv->value;
        }
    }
    TLV_LOOP_END
    switch (CMDCODE)
    {
    case RUN_NODE_RESOLVE_ARP:
        printf("Node:%s   ip address:%s",node_name,ip_address);
        break;
    
    default:
        break;
    }
}

void nw_init_cli(){
    init_libcli();
    param_t *show = libcli_get_show_hook();
    param_t* debug = libcli_get_debug_hook();
    param_t *config = libcli_get_config_hook();
    param_t* run = libcli_get_run_hook();
    param_t * debug_show = libcli_get_debug_show_hook();
    param_t * root = libcli_get_root();
    {
        //show topology
        static param_t topology;
        init_param(&topology,CMD,"topology",show_nw_topology_handler,0,INVALID,0,"Shows details about the entire network");
        libcli_register_param(show,&topology);
        set_param_cmd_code(&topology,CMDCODE_SHOW_TOPOLOGY);
    }
    {
        //run node 
        static param_t node;
        init_param(&node,CMD,"node",0,0,INVALID,0,"Run node");
        libcli_register_param(run,&node);
        {
            //run node <node-name> 
            static param_t node_name;
            init_param(&node_name,LEAF,0,0,0,STRING,"node_name","Arp handler\n");
            libcli_register_param(&node,&node_name);
            {
                //run node <node-name> resolve-arp 
                static param_t resolve_arp;
                init_param(&resolve_arp,CMD,"resolve-arp",0,0,INVALID,0,"resolve arp");
                libcli_register_param(&node_name,&resolve_arp);
                {
                    //run node <node-name> resolve-arp <ip-address>
                    static param_t ip_address;
                    init_param(&ip_address,LEAF,0,arp_handler,0,STRING,"ip-address","IP address");
                    libcli_register_param(&resolve_arp,&ip_address);
                    set_param_cmd_code(&ip_address,RUN_NODE_RESOLVE_ARP);
                }

            }
        }


    }
    support_cmd_negation(config);
}