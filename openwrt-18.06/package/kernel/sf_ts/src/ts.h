#ifndef _TS_H_
#define _TS_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <linux/jhash.h>
#include <linux/random.h>
#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <asm/unaligned.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include <linux/net.h>
#include <net/dsfield.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/netfilter.h>
#include <linux/netfilter_bridge.h>
#include <net/netfilter/nf_conntrack.h>

#if IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER)
#include "sf_hnat.h"
#endif

#define DMARK				0x800
#define NF_UPLOAD_PRI_SF_TS				-500
#define NF_DOWNLOAD_PRI_SF_TS			-500

#define MAC_HASH_SIZE			128
#define MAC_HASH_NUM				(MAC_HASH_SIZE - 1)


#define sf_alloc_percpu_stats(type)               \
({                              \
	typeof(type) __percpu *pcpu_stats = alloc_percpu(type); \
	if (pcpu_stats) {                   \
		int i;                      \
		for_each_possible_cpu(i) {          \
			typeof(type) *per_c;         \
			per_c = per_cpu_ptr(pcpu_stats, i);  \
			per_c->up_c = 0; \
			per_c->down_c = 0; \
			u64_stats_init(&per_c->lock);       \
		}                       \
	}                           \
	pcpu_stats;                     \
})

/*
 * Import form sf_gmac.h
 * should always the same
 * */
enum drop_level {
	SF_UNRELATE_DROP,
	SF_HASH_DROP,
	SF_RANDOM_DROP,
	SF_ALL_DROP,
};

/*
 * add a mac entry to traffic list
 * del a mac entry from traffic list
 * add a mac to hnat hw flowoffload blacklist
 * del a mac from hnat hw flowoffload blacklist
 * reset a mac tx/rx
 * reset all mac tx/rx
 */
enum ts_cmd{
	CMD_ADD_STS = 0,
	CMD_DEL_STS,
	CMD_ADD_BLK,
	CMD_DEL_BLK,
	CMD_RESET_TXRX,
	CMD_RESET_ALL_TXRX,
	CMD_LOG_ENABLE,
};

struct ts_info{
	u64 up_c;
	u64 down_c;
	struct u64_stats_sync lock;
};

struct conn_info{
	struct list_head list;
	struct nf_conn *ct;
	unsigned long update_time;
};

/*
 * snode: hash list node
 * mac: device mac address
 * total: total traffic for all cpu
 * c: traffic statistic for per cpu
 * upload_s: upload speed
 * download_s: download speed
 * m_upload_s: max upload speed
 * m_download_s: max download speed
 * alive: mark device alive when traffic has passed in 1s
 * clear: use for reset total traffic statistic
 */

struct dev_info{
	struct hlist_node snode;
	u8 mac[ETH_ALEN];
	struct ts_info total;
	struct ts_info __percpu *c;
	struct list_head ct_list;
	spinlock_t ct_lock;
	u32 ct_count;
	u32 upload_s;
	u32 download_s;
	u32 m_upload_s;
	u32 m_download_s;
	u8 alive;
	u8 clear;
};


struct sf_ts_priv {
	struct hlist_head devlist[MAC_HASH_SIZE];
	unsigned int random_seed;
	struct timer_list ts_timer;
	struct proc_dir_entry *file;
#if IS_ENABLED(CONFIG_SFAX8_HNAT_DRIVER)
	struct platform_device *hnat_pdev;
	struct sf_hnat_priv *phnat_priv;
#endif
};

#endif /* ifndef  _TS_H_ */
