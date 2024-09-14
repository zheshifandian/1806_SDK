#ifndef _SF_GSW_INTEL_OPS_H_
#define _SF_GSW_INTEL_OPS_H_

#include <linux/list.h>
#include <linux/vmalloc.h>

#define BIT_ZERO(X)      ((X) = 0)
#define BIT_SET(X,n)     ((X) |= 1 << (n))
#define BIT_CLR(X,n)     ((X) &= ~(1 << (n)))
#define BIT_TST(X,n)     ((X) & 1 << (n))

struct sf_intelsw_mc_list {
	struct list_head list;
	u32 mcast_addr;
	u32	port_list;
};

struct sf_intelsw_mc_list * mc_list = NULL;
struct sf_intelsw_mc_list * mc_list_tail = NULL;
#endif
