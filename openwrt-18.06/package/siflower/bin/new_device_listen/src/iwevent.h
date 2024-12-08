#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/rtnetlink.h>
#include <libubox/uloop.h>
#include "iwlib.h"
#include "device_info.h"



/*
 * Static information about wireless interface.
 * We cache this info for performance reason.
 */
typedef struct wireless_iface
{
  /* Linked list */
  struct wireless_iface *   next;

  /* Interface identification */
  int       ifindex;        /* Interface index == black magic */

  /* Interface data */
  char          ifname[IFNAMSIZ + 1];   /* Interface name */
  struct iw_range   range;          /* Wireless static data */
  int           has_range;
} wireless_iface;


typedef struct rtnl_handle
{
    int         fd;
    struct sockaddr_nl  local;
    struct sockaddr_nl  peer;
    __u32           seq;
    __u32           dump;
	__u16	family_id;
} rtnl_handle;

typedef struct custom_iface_ctx
{
    char custom[IW_CUSTOM_MAX+1];
    char iface[20];
	int online;
} custom_iface_ctx;

enum {
	SF_ETH_CMD_ATTR_UNSPEC = 0,
	SF_ETH_CMD_ATTR_DPS_PORT,            /* eth phy port*/
	SF_ETH_CMD_ATTR_DPS_LINK,            /* 0---link down  1---link up */
	SF_ETH_CMD_ATTR_DPS_MAC,
	SF_ETH_CMD_ATTR_DPS_IFNAME,
	__SF_ETH_CMD_ATTR_MAX,
};

#define MAX_MSG_SIZE 8182
#define SOL_NETLINK	 270
typedef struct msgtemplate {
	struct nlmsghdr n;
	struct genlmsghdr g;
	char data[MAX_MSG_SIZE];
} msgtemplate_t;

struct arp_info{
	u_int8_t ip[4];
	u_int8_t mac[6];
};

#define MESH_PORT   1112

#define GENLMSG_DATA(glh)       ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define NLA_DATA(na)            ((void *)((char *)(na) + NLA_HDRLEN))
#define NLA_NEXT(na)			((void *)((char *)(na) + NLA_ALIGN(na->nla_len)))
#define TOCHAR(n)				(n>0x9 ? n+0x37 : n+0x30)

#define NDA_RTA(r)			((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))

int get_genl_group(struct rtnl_handle *rth, char *family, char *group);
int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions, int type);
void rtnl_close(struct rtnl_handle *rth);
int wait_for_event(struct rtnl_handle * rth);
void check_dev_from_boot(void);
int name2index(char *name);
#ifdef USE_CUSTOM_SOCKET
int init_wifi_custom_socket(struct rtnl_handle *rth,int port);
#endif
