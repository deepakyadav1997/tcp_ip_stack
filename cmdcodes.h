#ifndef __CMDCODES__
#define __CMDCODES__

#define CMDCODE_SHOW_TOPOLOGY        11
#define RUN_NODE_RESOLVE_ARP         12
#define DUMP_ARP_TABLE               13
#define CMDCODE_SHOW_MAC_TABLE       14
#define CMD_CODE_SHOW_RT_TABLE       15   
#define CMDCODE_CONF_NODE_L3ROUTE    16
#define RUN_NODE_PING                17
#define RUN_NODE_PING_ERO            18

void nw_init_cli();
#endif