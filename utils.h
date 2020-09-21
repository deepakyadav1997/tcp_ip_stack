#ifndef __UTIL__
#define __UTIL__

typedef enum {
    FALSE,
    TRUE
}bool_t;

#define IS_MAC_BROADCAST_ADDR(mac)  \
(mac[0] == 255 && mac[1] == 255 &&  \
 mac[2] == 255 && mac[3] == 255 &&  \
 mac[4] == 255 && mac[5] == 255)


void apply_mask(char *prefix, char mask, char *str_prefix);
void layer2_fill_with_broadcast_mac(char *mac_array);

#endif