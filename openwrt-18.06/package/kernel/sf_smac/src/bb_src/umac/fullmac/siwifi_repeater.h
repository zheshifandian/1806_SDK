#ifndef __REPEATER_H__
#define __REPEATER_H__
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "siwifi_defs.h"
#ifdef CONFIG_SIWIFI_REPEATER_IPV6
#define IP_ADDR_LEN   16
// ipv6 address and ipv6 link local address(fe80::)
#define IPTYPE 3
#else
#define IP_ADDR_LEN   4
#define IPTYPE 1
#endif

struct repeater_info {
    struct repeater_sta **ip_head[IPTYPE];
    struct repeater_sta **mac_head;
    unsigned char if_mac[ETH_ALEN]; /* replace mac */
    unsigned short arr_len; /* Hash table arr LEN */
    unsigned short max_sta;
    unsigned short sta_num; /* current sta num. */
    struct net_device *vif_dev;
    struct list_head list;
    struct timer_list expires;
};

enum {
    IPVER_4 = 0,
#ifdef CONFIG_SIWIFI_REPEATER_IPV6
    IPVER_6,
    // ipv6 link local address
    IPVER_6_LLA
#endif
};

struct repeater_sta {
    unsigned char mac[ETH_ALEN];
    u8 ip[IPTYPE][IP_ADDR_LEN];
    struct repeater_sta *ip_link[IPTYPE];
    struct repeater_sta *mac_link;
    u16 dhcp_flag;
    unsigned long last_use;
};

static int repeater_ip_proc(struct repeater_info *rp_info, unsigned char *eh, unsigned char *pkt, int pkt_len, int send);
static int repeater_arp_proc(struct repeater_info *rp_info, unsigned char *eh, unsigned char *pkt, int pkt_len, int send);
typedef int (*prot_proc_t)(struct repeater_info *rp_info, unsigned char *eh, unsigned char *pkt, int pkt_len, int send);


typedef int (*ipv4_proc_t)(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *pkt, int pkt_len, int send);

typedef int (*udp_proc_t)(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *udph, u8 *pkt, int pkt_len, int send);
static int repeater_udp_proc(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *pkt, int pkt_len, int send);

#ifdef CONFIG_SIWIFI_REPEATER_IPV6
typedef int (*ipv6_proc_t)(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *pkt, int pkt_len, int send);
static int repeater_ipv6_proc(struct repeater_info *rp_info, unsigned char *eh, unsigned char *pkt, int pkt_len, int send);
static int repeater_ipv6_icmp_proc(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *data, int data_len, int send);
static int repeater_ipv6_udp_proc(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *pkt, int pkt_len, int send);
#endif

static int repeater_dhcpc_proc(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *udph, u8 *pkt, int pkt_len, int send);
static int repeater_dhcps_proc(struct repeater_info *rp_info, u8 *eh, u8 *iph, u8 *udph, u8 *pkt, int pkt_len, int send);

int repeater_init(void);
void repeater_exit(void);
int repeater_register(struct repeater_info **rp_info, struct net_device *vif_dev, int max_sta);
int repeater_unregister(struct repeater_info *rp_info);
int repeater_rev_proc(struct repeater_info *rp_info, struct sk_buff **skb);
int repeater_send_proc(struct repeater_info *rp_info, struct sk_buff **skb);
#endif
