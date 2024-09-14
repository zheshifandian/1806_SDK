#ifndef _SF_HNAT_TEST_TOOL_H_
#define _SF_HNAT_TEST_TOOL_H_

#include <linux/etherdevice.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/platform_device.h>

/*for gmac debug use*/
struct sf_rx_test_unit{

	unsigned short rx_pkt_len; // first tx desc address
	unsigned long long rx_pkt_count;
	unsigned long long  rx_total_data;

	unsigned long long  rx_start_time;
	unsigned long long  rx_stop_time;

	unsigned long long rx_last_pkt_count;
	unsigned long long rx_last_data_count;
	u32 time;
	u64 start_rx_jiffies;//first pkt recv
	u64 end_rx_jiffies;// last  pkt recv
	u8  should_end;
	u64 rx_target_jiffies;
	u64 rx_last_timer_jiffies;

	u32 rx_timer_index;
	struct timer_list rx_timer;

};

struct sf_frag_pkt_set{
	unsigned int frag_no;//the number of fragments
	unsigned int ip_id_no;//the number of ip_id
	unsigned char is_frag;
	unsigned char is_continus;//set 1 for multi thread test
};

struct sf_test_tool_para{
	unsigned int  time;// all work time
	unsigned short band_width; // Mbps
	unsigned int mtu;//Bytes
};

struct sf_test_tool_set{
	__be32 sip;// network order
	__be32 dip;// network order
	unsigned short sport;//host order
	unsigned short dport;//host order
	__be32 router_ip;// network order
	unsigned short router_port;//host order
	unsigned char src_mac[6];
	unsigned short src_vlan;
	unsigned short dest_vlan;
	unsigned char dest_mac[6];
	unsigned char router_src_mac[6]; //wan mac
	unsigned char router_dest_mac[6];// lan mac
	unsigned char proto; // 0 tcp or udp 1
	unsigned char valid;
	unsigned short ppp_sid;//host order
	struct sf_cap_file_data* pcap_file;
	unsigned char pkt_is_wan_lan;
};


/*for thread poll use*/
struct sf_thread_pool{
	struct workqueue_struct* pmanager_wq;

	u8 sf_test_thread_num;
	// struct sf_test_unit *p_unit_hdr;
	struct list_head test_list_hdr;
};
struct sf_tx_work{
   struct work_struct work;
   struct sf_test_unit * ptest_unit;
};

struct sf_test_unit{
	struct task_struct	*ptx_task;
	struct  sf_tx_work *ptx_work;
	struct list_head	 list_unit;
	u16 key;
	struct workqueue_struct* pmanager_wq;
	struct timer_list tx_timer;
	u32 tx_timer_index;
	unsigned short tx_pkt_len; // first tx desc address
	unsigned long long tx_pkt_count;
	unsigned long long tx_last_pkt_count;

	unsigned long long  tx_start_time;
	unsigned long long  tx_stop_time;

	// because CONFIG_HZ is 100 so one tick is 10ms
	u64 start_tx_jiffies;// first send time
	u64 end_tx_jiffies;//  last intrreupt time

	u64 tx_target_jiffies;
	u32 tx_last_timer_jiffies;

	u8 tx_thread_end;
	u8 tx_timer_release;
	struct sf_test_tool_para * ptool_para;
	struct sf_test_tool_set *ptest_set;

	struct sf_test_tool_priv * ptest_priv;

	unsigned char * dma_tx_data; // the whole list of tx desc
	dma_addr_t dma_tx_data_phy; // first tx desc address

	/*for frag test use*/
	unsigned char ip_hdr_offset;
	unsigned short ip_id;

	/*for full entry test use*/
	unsigned char is_full_test;

	u32 pkt_index;
	u32 max_pkt_index;
};

struct sf_test_tool_priv {
	atomic_t is_tx_pause;
	struct mutex tx_lock;
	struct mutex list_lock;
	unsigned char g_start_test;
	unsigned char g_start_test_tx;
	unsigned char g_start_test_rx;
	unsigned int g_test_tx_pause_count;
	// here for data print.

	struct sf_test_tool_set *ptest_sets;
// for tx
	struct sf_thread_pool *pt_pool;

	struct sf_hnat_priv *hnat_priv;
// for rx
	struct sf_rx_test_unit  rx_test_unit;

	struct sf_test_tool_para rx_tool_para;

	u8 last_use_index;

	struct sf_frag_pkt_set * frag_set;
	unsigned short rest_space;

	struct device *driver_dev;//get gmac device pointer for dma alloc
	void * driver_priv;
	char (*driver_xmit)(void* driver_priv, dma_addr_t dma_tx_data_phy,
						unsigned short pkt_len, unsigned short reset_space,
						atomic_t *pis_tx_pause, unsigned int* ptx_pause_count);
	void (*driver_tx_complete)(void* driver_priv, unsigned short rest_space, atomic_t *pis_tx_pause);

	void (*tool_init)(struct platform_device *pdev, struct device *driver_dev, void * driver_priv);
	void (*tool_deinit)(struct platform_device *pdev);
	void (*tool_set_rx)(struct platform_device *pdev, unsigned char type, unsigned int pkt_length);
};

#endif
