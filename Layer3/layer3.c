#include "layer3.h"
#include "../net.h"

#include<arpa/inet.h>

void init_rt_table(rt_table_t **rt_table){
    *rt_table = calloc(1,sizeof(rt_table_t));
    init_glthread(&((*rt_table)->route_list));
}

void rt_table_add_direct_route(rt_table_t *rt_table,
                               char *dst, char mask){

    rt_table_add_route(rt_table,dst,mask,0,0);
}

void rt_table_add_route(rt_table_t *rt_table, 
                        char *dst, char mask,
                        char *gw, char *oif){

    assert(rt_table != NULL);

    int dest_ip = 0;
    char dest_ip_masked[16];
    apply_mask(dst,mask,dest_ip_masked);
    inet_pton(AF_INET,dest_ip_masked,&dest_ip);
    l3_route_t* l3_route =  l3rib_lookup_lpm(rt_table,dest_ip);

    //duplicate entry
    if(l3_route != NULL){
        printf("Route already exists. Skipping...\n");
        ///assert(!l3_route);
    }
    

    l3_route = calloc(1,sizeof(l3_route_t));
    strncpy(l3_route->dest,dest_ip_masked,sizeof(ip_add_t));
    l3_route->mask = mask;
    if(gw && oif){
        l3_route->is_direct = FALSE;
        memcpy(l3_route->gw_ip,gw,sizeof(ip_add_t));
        memcpy(l3_route->oif,oif,IF_NAME_SIZE);
        
    }
    else{
        l3_route->is_direct = TRUE;
        memset(l3_route->gw_ip,0,sizeof(ip_add_t));
        memset(l3_route->oif,0,IF_NAME_SIZE);
    }
    l3_route->oif[IF_NAME_SIZE-1] = '\0';
    l3_route->gw_ip[15] = '\0';
    init_glthread(&l3_route->rt_glue);
    glthread_add_next(&rt_table->route_list,&l3_route->rt_glue);

}

static void dump_l3_route(l3_route_t* l3_route){
    
    printf("\nDest: %s/%d",l3_route->dest,l3_route->mask);
    if(l3_route->is_direct == FALSE){
        printf("\tGateway IP:%s  OIF:%s\n",l3_route->gw_ip,l3_route->oif);
    }
}

void dump_rt_table(rt_table_t *rt_table){

    printf("\nDumping routing table\n");
    glthread_t* current;
    l3_route_t * l3_route = NULL;
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        dump_l3_route(l3_route);
    }ITERATE_GLTHREAD_END;

}

l3_route_t * rt_table_lookup(rt_table_t *rt_table,char *ip_addr,
                            char mask){

    glthread_t *current = NULL;
    l3_route_t *l3_route = NULL;

    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        if(strncmp(l3_route->dest,ip_addr,sizeof(ip_add_t)) == 0 && 
                                            l3_route->mask == mask){

            return l3_route;                                    
        }

    }ITERATE_GLTHREAD_END;

    return NULL;

}

void delete_rt_table_entry(rt_table_t *rt_table,char *ip_addr,
                           char mask){

    char dest_ip_masked[16];
    apply_mask(ip_addr,mask,dest_ip_masked);
    l3_route_t * l3_route = rt_table_lookup(rt_table,dest_ip_masked,mask);
    if(l3_route == NULL){
        return;
    }
    remove_glthread(&l3_route->rt_glue);
    free(l3_route);
}

l3_route_t * l3rib_lookup_lpm(rt_table_t *rt_table,
                              unsigned int dest_ip){
    
    char subnet[16];
    char destination_ip_str[16];
    char longest_mask = 0;
    l3_route_t *l3_route = NULL,
               *lpm_route = NULL,
               *default_route = NULL;

    dest_ip = htonl(dest_ip);
    // inet_ntop(AF_INET,&dest_ip,destination_ip_str,sizeof(ip_add_t));
    convert_ip_from_int_to_str(dest_ip,destination_ip_str);

    destination_ip_str[sizeof(ip_add_t) - 1] = '\0';
    //printf("Unmasked ip %s\n",destination_ip_str);
    glthread_t *current = NULL;
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,current){
        l3_route = rt_glue_to_l3_route(current);
        memset(subnet,0,16);
        apply_mask(destination_ip_str,l3_route->mask,subnet);
        //printf("MAsked ip %s\n",subnet);
        if(strncmp("0.0.0.0",l3_route->dest,sizeof(ip_add_t)) == 0 && l3_route->mask == 0){
                default_route = l3_route;
            
        }
        else if(strncmp(destination_ip_str,l3_route->dest,sizeof(ip_add_t)) == 0){
            if(l3_route->mask > longest_mask){
                longest_mask = l3_route->mask;
                lpm_route = l3_route;
            }
        }
    }ITERATE_GLTHREAD_END;

    return lpm_route ? lpm_route:default_route;
}