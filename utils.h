#ifndef __UTIL__
#define __UTIL__

typedef enum {
    FALSE,
    TRUE
}bool_t;

void apply_mask(char *prefix, char mask, char *str_prefix);
void layer2_fill_with_broadcast_mac(char *mac_array);

#endif