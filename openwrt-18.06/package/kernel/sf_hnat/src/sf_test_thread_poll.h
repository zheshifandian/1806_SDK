#ifndef _SF_TEST_THREAD_POOL_H_
#define _SF_TEST_THREAD_POOL_H_

#include "sf_hnat_debug.h"
void sf_thread_pool_init(struct sf_test_tool_priv *ptest_priv);

void sf_thread_pool_deinit(struct sf_test_tool_priv *ptest_priv);
void sf_thread_pool_start_test(struct sf_test_tool_priv *ptest_priv, u8 test_index, u8 is_wan_lan, u8 is_udp, struct sf_test_tool_para * ptool_para);
void sf_thread_pool_end_test(struct sf_test_tool_priv *ptest_priv, u16 key);
int sf_thread_test_send_full(struct sf_test_tool_priv *ptest_priv, struct sf_test_tool_para * ptool_para);
int sf_thread_test_select_pkt(struct sf_test_tool_priv *ptest_priv,struct sf_test_tool_para * ptool_para, u8 select_pkt_index);
#endif // _SF_TEST_THREAD_POOL_H_
