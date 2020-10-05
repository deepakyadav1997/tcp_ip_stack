#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"
#include "cmdcodes.h"
#include "graph.h"
#include "Layer2/layer2.h"
#include "Layer3/layer3.h"

#include<stdio.h>


extern graph_t *topo;

extern void dump_mac_table(mac_table_t * mac_table);
extern void layer5_ping_fn(node_t * node,char* dst_ip);
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

static int l3_config_handler(param_t *param, ser_buff_t *tlv_buf,op_mode enable_or_disable){
    node_t *node = NULL;
    char *node_name = NULL;
    char *intf_name = NULL;
    char *gwip = NULL;
    char *mask_str = NULL;
    char *dest = NULL;
    int CMDCODE = -1;

    CMDCODE = EXTRACT_CMD_CODE(tlv_buf); 
    
    tlv_struct_t *tlv = NULL;
    
    TLV_LOOP_BEGIN(tlv_buf, tlv){

        if(strncmp(tlv->leaf_id, "node-name", strlen("node-name")) ==0)
            node_name = tlv->value;
        else if(strncmp(tlv->leaf_id, "ip-address", strlen("ip-address")) ==0)
            dest = tlv->value;
        else if(strncmp(tlv->leaf_id, "gw-ip", strlen("gw-ip")) ==0)
            gwip = tlv->value;
        else if(strncmp(tlv->leaf_id, "mask", strlen("mask")) ==0)
            mask_str = tlv->value;
        else if(strncmp(tlv->leaf_id, "oif", strlen("oif")) ==0)
            intf_name = tlv->value;
        else
            assert(0);

    }TLV_LOOP_END;

    node = get_node_by_node_name(topo, node_name);

    char mask;
    if(mask_str){
        mask = atoi(mask_str);
    }
    interface_t *intf = NULL;
    switch (CMDCODE)
    {
    case CMDCODE_CONF_NODE_L3ROUTE:
        if(intf_name){
            intf = get_node_if_by_name(node, intf_name);
            if(!intf){
                printf("Config Error : Non-Existing Interface : %s\n", intf_name);
                return -1;
            }
            if(!IS_INTF_L3_MODE(intf)){
                printf("Config Error : Not L3 Mode Interface : %s\n", intf_name);
                return -1;
            }
        }
        rt_table_add_route(node->node_nw_prop.rt_table, dest, mask, gwip, intf_name);
        break;
    
    default:
        break;
    }
}

static int cmd_handler(param_t *param, ser_buff_t *tlv_buf,op_mode enable_or_disable){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buf);
    char *node_name = NULL;
    node_t *node;
    char *ip_address = NULL;
    tlv_struct_t * tlv;
    TLV_LOOP_BEGIN(tlv_buf,tlv){
        if(strncmp(tlv->leaf_id,"node_name",strlen("node_name")) == 0){
            node_name = tlv->value;
        }
        else if(strncmp(tlv->leaf_id,"ip-address",strlen("ip-address")) == 0){
            ip_address =tlv->value;
        }
    }
    TLV_LOOP_END
    node = get_node_by_node_name(topo,node_name);
    switch (CMDCODE)
    {
    case RUN_NODE_RESOLVE_ARP:
        //printf("Node:%s   ip address:%s",node_name,ip_address);
        if(node)
            send_arp_broadcast_request(node,NULL,ip_address);
        break;
    case DUMP_ARP_TABLE:
        if(node)
            dump_arp_table(node->node_nw_prop.arp_table);
        break;
    case CMDCODE_SHOW_MAC_TABLE:
        if(node)
            dump_mac_table(node->node_nw_prop.mac_table);
        break;
    case CMD_CODE_SHOW_RT_TABLE:
        if(node)
            dump_rt_table(node->node_nw_prop.rt_table);
    case RUN_NODE_PING:
        if(node)
            layer5_ping_fn(node,ip_address);
            
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
                    init_param(&ip_address,LEAF,0,cmd_handler,0,STRING,"ip-address","IP address");
                    libcli_register_param(&resolve_arp,&ip_address);
                    set_param_cmd_code(&ip_address,RUN_NODE_RESOLVE_ARP);
                }

            }
            {
                //run node <node-name> ping <ip-address>
                static param_t ping;
                init_param(&ping,CMD,"ping",0,0,INVALID,0,"ping");
                libcli_register_param(&node_name,&ping);
                {
                    //run node <node-name> ping <ip-address>
                    static param_t ip_address;
                    init_param(&ip_address,LEAF,0,cmd_handler,0,STRING,"ip-address","IP address");
                    libcli_register_param(&ping,&ip_address);
                    set_param_cmd_code(&ip_address,RUN_NODE_PING);
                }
            }
        }


    }
    {
        static param_t node;
        init_param(&node,CMD,"node",0,0,INVALID,0,"Run node");
        libcli_register_param(show,&node);
        {   
            // show node node_name -----------displays arp table
            static param_t node_name;
            init_param(&node_name,LEAF,0,cmd_handler,0,STRING,"node_name","Dump arp table\n");
            libcli_register_param(&node,&node_name);
            set_param_cmd_code(&node_name,DUMP_ARP_TABLE);
            {
                static param_t mac;
                init_param(&mac,CMD,"mac",cmd_handler,0,INVALID,0,"Display mac table");
                libcli_register_param(&node_name,&mac);
                set_param_cmd_code(&mac,CMDCODE_SHOW_MAC_TABLE);
            }
            {
                static param_t rt;
                init_param(&rt,CMD,"rt",cmd_handler,0,INVALID,0,"Dumping routing table");
                libcli_register_param(&node_name,&rt);
                set_param_cmd_code(&rt,CMD_CODE_SHOW_RT_TABLE);
            }
        }
        
    }
    {
        static param_t node;
        init_param(&node,CMD,"node",0,0,INVALID,0,"Config node");
        libcli_register_param(config,&node);
        {
            /*config node <node-name>*/
            static param_t node_name;
            init_param(&node_name, LEAF, 0, 0, 0, STRING, "node-name", "Node Name");
            libcli_register_param(&node, &node_name);
            {
                        {
                /*config node <node-name> route*/
                static param_t route;
                init_param(&route, CMD, "route", 0, 0, INVALID, 0, "L3 route");
                libcli_register_param(&node_name, &route);
                {
                    /*config node <node-name> route <ip-address>*/    
                    static param_t ip_addr;
                    init_param(&ip_addr, LEAF, 0, 0, 0, IPV4, "ip-address", "IPv4 Address");
                    libcli_register_param(&route, &ip_addr);
                        {
                            /*config node <node-name> route <ip-address> <mask>*/
                            static param_t mask;
                            init_param(&mask, LEAF, 0, l3_config_handler,0, INT, "mask", "mask(0-32");
                            libcli_register_param(&ip_addr, &mask);
                            set_param_cmd_code(&mask, CMDCODE_CONF_NODE_L3ROUTE);
                                {
                                    /*config node <node-name> route <ip-address> <mask> <gw-ip>*/
                                    static param_t gwip;
                                    init_param(&gwip, LEAF, 0, 0, 0, IPV4, "gw-ip", "IPv4 Address");
                                    libcli_register_param(&mask, &gwip);
                                    {
                                        /*config node <node-name> route <ip-address> <mask> <gw-ip> <oif>*/
                                        static param_t oif;
                                        init_param(&oif, LEAF, 0, l3_config_handler, 0, STRING, "oif", "Out-going intf Name");
                                        libcli_register_param(&gwip, &oif);
                                        set_param_cmd_code(&oif, CMDCODE_CONF_NODE_L3ROUTE);
                                    }
                                }
                            }
                        }    
                    }    
            }

        }
    }
    support_cmd_negation(config);
}