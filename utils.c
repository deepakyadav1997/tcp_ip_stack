#include<stdio.h>
#include "utils.h"
#include "net.h"

void apply_mask(char *prefix, char mask, char *str_prefix){
    unsigned int ip_int = convert_ip_from_str_to_int(prefix);
    int binary_ip[32];
    int_to_binary(ip_int,binary_ip,32);
    for(int i = mask;i<32;i++){
        binary_ip[i] = 0;
    }
    int masked_ip = binary_to_int(binary_ip,31,0);
    convert_ip_from_int_to_str(masked_ip,str_prefix);
}

void layer2_fill_with_broadcast_mac(char *mac_array){
    // Assuming mac is always 8 bytes as per the struct definition
    memset(mac_array,255,8);
}