#ifndef __ANDLINK__UTILS__H
#define __ANDLINK__UTILS__H
#include "andlink.h"
int check_andlink_config();
void listener_thread(void *paras);
int parse_net_data(char *buf, uint32_t size);
#endif
