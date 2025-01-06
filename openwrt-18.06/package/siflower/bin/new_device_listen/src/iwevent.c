#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>
#include <linux/nl80211.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>

#include <string.h>
#include <pthread.h>
#include <uci.h>
#include <arpa/inet.h>

#include "iwlib.h"      /* Header */
#include "iwevent.h"
#include "utils.h"
#include "device_info.h"
#include "publish.h"
#include "common.h"
#include "local_http.h"

#define UPTIME_MAXLEN 128
#define UPTIME_REALLEN 32
#define UPTIME_PATH "/proc/uptime"

/* Cache of wireless interfaces */
struct wireless_iface * interface_cache = NULL;
u_int16_t wan_ifi = 0;
extern int g_force_exit_signal;
extern struct uloop_fd wifi_fd;
void handle_custom_wifi_events(struct uloop_fd *rth, unsigned int uevents);

int genl_send_msg(int fd, u_int16_t nlmsg_type, u_int32_t nlmsg_pid, u_int8_t genl_cmd, u_int8_t genl_version,
		u_int16_t nla_type, void *nla_data, int nla_len){
    struct nlattr *na;
	struct sockaddr_nl nladdr;
	int r, buflen;
	char *buf;
	msgtemplate_t msg;

	if(nlmsg_type == 0){
		return 0;
	}

	msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	msg.n.nlmsg_type = nlmsg_type;
	msg.n.nlmsg_flags = NLM_F_REQUEST;
	msg.n.nlmsg_seq = 0;

	msg.n.nlmsg_pid = nlmsg_pid;
	msg.g.cmd = genl_cmd;
	msg.g.version = genl_version;
	na = (struct nlattr *) GENLMSG_DATA(&msg);
	na->nla_type = nla_type;
	na->nla_len = nla_len + 1 + NLA_HDRLEN;
	memcpy(NLA_DATA(na), nla_data, nla_len);
	msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);

	buf = (char *) &msg;
	buflen = msg.n.nlmsg_len;
	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	while ((r = sendto(fd, buf, buflen, 0, (struct sockaddr *) &nladdr, sizeof(nladdr))) < buflen) {
		if (r > 0) {
			buf += r;
			buflen -= r;
		}else if(errno != EAGAIN){
			return -1;
		}
	}
	LOG("Send genl get message ok!\n");
	return 0;
}

#define ATTR_NEXT(attr) (struct nlattr *)(((char *)attr) + NLA_ALIGN(attr->nla_len))

int find_group(struct nlattr *nlattr, char *group){
	struct nlattr *gattr = (struct nlattr *)(((char *)nlattr) + NLA_HDRLEN);
	int nl_len = nlattr->nla_len - NLA_HDRLEN;
	while(1){
		if(strncmp(((char *)gattr)+3*NLA_HDRLEN+4, group, strlen(group))== 0){
			return *((int *)((char *)gattr + 2*NLA_HDRLEN));
		}
		nl_len -= NLA_ALIGN(gattr->nla_len);
		if( nl_len > 0){
			gattr = ATTR_NEXT(gattr);
		}else{
			LOG("find group fail\n");
			return -1;
		}
	}
}

int get_genl_group(struct rtnl_handle *rth, char *family, char *group){
	msgtemplate_t genlmsg;
	int ret;
	struct nlattr *nlattr;
	int rc_len;
	int nl_len;

	ret = genl_send_msg(rth->fd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1, CTRL_ATTR_FAMILY_NAME,
		   	family, strlen(family)+1);
	if(ret){
		LOG("Send genl fail\n");
		goto fail;
	}
	rc_len = recv(rth->fd, &genlmsg, sizeof(genlmsg), 0);

	if(rc_len < 0){
		LOG("Receive error!\n");
		goto fail;
	}

	if(genlmsg.n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&genlmsg.n), rc_len)){
		LOG("Genlmsg type is %d, rc_len is %d, msg len is %d\n", genlmsg.n.nlmsg_type, rc_len, genlmsg.n.nlmsg_len);
		goto fail;
	}

	LOG("test genlmsg type %d\n", genlmsg.n.nlmsg_type);
	if(genlmsg.n.nlmsg_type == GENL_ID_CTRL){
		LOG("nl type ok\n");
		if(genlmsg.g.cmd == CTRL_CMD_NEWFAMILY){
			LOG("return cmd right\n");
			nlattr = (struct nlattr *)GENLMSG_DATA(&genlmsg);
			nl_len = genlmsg.n.nlmsg_len - NLMSG_HDRLEN - NLA_HDRLEN;
			if(nlattr->nla_type == CTRL_ATTR_FAMILY_NAME){
				if(strncmp(((char *)nlattr)+NLA_HDRLEN, family, strlen(family)) == 0){
					nlattr = ATTR_NEXT(nlattr);
					rth->family_id = *((__u16 *)(NLA_DATA(nlattr)));
					LOG("return family is %d\n", rth->family_id);
				    while(1){
						nl_len -= NLA_ALIGN(nlattr->nla_len);
						if( nl_len <=0){
							LOG("Not find attr\n");
							goto fail;
						}
						nlattr = ATTR_NEXT(nlattr);
						if( nlattr->nla_type == CTRL_ATTR_MCAST_GROUPS){
							return find_group(nlattr, group);
						}else{
							continue;
						}
					}
				}
			}
		}
	}
fail:
	return -1;

}
int name2index(char *name)
{
    struct ifreq  irq;
    int ret = 0;
	int skfd;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        LOG("iw_sockets_open");
        return -1;
    }
	strncpy(irq.ifr_name, name, IFNAMSIZ);

    if(ioctl(skfd, SIOCGIFINDEX, &irq) < 0)
		ret = -1;
    else
		ret = irq.ifr_ifindex;

	close(skfd);
    return(ret);
}

#ifdef USE_CUSTOM_SOCKET
int init_wifi_custom_socket(struct rtnl_handle *rth,int port)
{
	struct sockaddr_in sockinaddr;
	int retry = 0;
	int ret = 0;

	memset(rth, 0, sizeof(rtnl_handle));
	rth->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (rth->fd < 0) {
		LOG("Cannot open netlink socket");
		return -1;
	}
	memset(&sockinaddr, 0, sizeof(sockinaddr));
	sockinaddr.sin_family = AF_INET;
	sockinaddr.sin_port = htons(port);
	sockinaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/*	panxu
		when this router is 192.168.4.1 and a upper router is also 192.168.4.1
		loopback 127.0.0.1 will be restarted sometimes
		if loopback 127.0.0.1 is disabled when code comes here,it will bind fail
		so retry 1 minute
	*/

	for(retry = 0; retry < 40; retry++)
	{
		ret = bind(rth->fd, (struct sockaddr *) &sockinaddr, sizeof(sockinaddr));
		if (ret < 0) {
			LOG("Cannot bind wifi custom socket ret %d, retry %d times", ret, retry+1);
			if(retry == 39)
				goto SOCKET_ERR;
			sleep(3);
		}
		else
		{
			LOG("bind wifi custom socket success");
			break;
		}
	}
	wifi_fd.cb = handle_custom_wifi_events;
	wifi_fd.fd = rth->fd;
	wifi_fd.registered =  false;
	wifi_fd.flags = ULOOP_READ;

	return 0;

SOCKET_ERR:
	close(rth->fd);
	return -1;
}
#endif

int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions, int type)
{
    int addr_len;

    memset(rth, 0, sizeof(rtnl_handle));

    rth->fd = socket(PF_NETLINK, SOCK_RAW, type);
    if (rth->fd < 0) {
        perror("Cannot open netlink socket");
        return -1;
    }

    memset(&rth->local, 0, sizeof(rth->local));
    rth->local.nl_family = AF_NETLINK;
    rth->local.nl_groups = subscriptions;

    if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
        perror("Cannot bind netlink socket");
        return -1;
    }
    addr_len = sizeof(rth->local);
    if (getsockname(rth->fd, (struct sockaddr*)&rth->local,
                (socklen_t *) &addr_len) < 0) {
        perror("Cannot getsockname");
        return -1;
    }
    if (addr_len != sizeof(rth->local)) {
        LOG("Wrong address length %d\n", addr_len);
        return -1;
    }
    if (rth->local.nl_family != AF_NETLINK) {
        LOG("Wrong address family %d\n", rth->local.nl_family);
        return -1;
    }
    rth->seq = time(NULL);
    return 0;
}

void rtnl_close(struct rtnl_handle *rth)
{
    close(rth->fd);
}

/*------------------------------------------------------------------*/
/*
 * Get name of interface based on interface index...
 */
    static inline int
index2name(int      skfd,
        int      ifindex,
        char *   name)
{
    struct ifreq  irq;
    int       ret = 0;

    memset(name, 0, IFNAMSIZ + 1);

    /* Get interface name */
    irq.ifr_ifindex = ifindex;
    if(ioctl(skfd, SIOCGIFNAME, &irq) < 0)
        ret = -1;
    else
        strncpy(name, irq.ifr_name, IFNAMSIZ);

    return(ret);
}



/*------------------------------------------------------------------*/
/*
 * Get interface data from cache or live interface
 */
    static struct wireless_iface *
iw_get_interface_data(int   ifindex)
{
    struct wireless_iface *   curr;
    int               skfd = -1;  /* ioctl socket */

    /* Search for it in the database */
    curr = interface_cache;
    while(curr != NULL)
    {
        /* Match ? */
        if(curr->ifindex == ifindex)
        {
            //printf("Cache : found %d-%s\n", curr->ifindex, curr->ifname);

            /* Return */
            return(curr);
        }
        /* Next entry */
        curr = curr->next;
    }

    /* Create a channel to the NET kernel. Doesn't happen too often, so
     * socket creation overhead is minimal... */
    if((skfd = iw_sockets_open()) < 0)
    {
        perror("iw_sockets_open");
        return(NULL);
    }

    /* Create new entry, zero, init */
    curr = calloc(1, sizeof(struct wireless_iface));
    if(!curr)
    {
        LOG("Malloc failed\n");
        return(NULL);
    }
    curr->ifindex = ifindex;

    /* Extract static data */
    if(index2name(skfd, ifindex, curr->ifname) < 0)
    {
        perror("index2name");
        free(curr);
        return(NULL);
    }
    curr->has_range = (iw_get_range_info(skfd, curr->ifname, &curr->range) >= 0);
    //printf("Cache : create %d-%s\n", curr->ifindex, curr->ifname);

    /* Done */
    iw_sockets_close(skfd);

    /* Link it */
    curr->next = interface_cache;
    interface_cache = curr;

    return(curr);
}

/****************************************/
//function: checknewdev_process
//brief: this function is a new thread to check the associating device is new device, and update the devlist table.
//param: the custom context
//return: NULL
/****************************************/
void *checknewdev_process(custom_iface_ctx *custom_and_iface)
{
    char *interface = NULL;
    interface = custom_and_iface->iface;
    __u8 buf[IW_CUSTOM_MAX+1];
    __u8 mac[MAC_LENGTH+1];
    unsigned char i = 0;
    char isonline[] = "1";
	char cmd[128];
	int ret;

    memcpy(buf, custom_and_iface->custom, IW_CUSTOM_MAX+1);

    /*************fetch the mac address*********************/
    for(i = 0; i<6; i++)
    {
		int j= 3*i;
		char tmp;
		tmp = buf[i] >> 4;
		mac[j] = TOCHAR(tmp);
		tmp = buf[i] & 0xf;
		mac[j+1] = TOCHAR(tmp);
		mac[j+2] = ':';
    }
	mac[MAC_LENGTH] = '\0';
	if(custom_and_iface->online){
		strcpy(isonline , "1");
	}else{
		strcpy(isonline , "0");
	}

    if(strcmp("00:00:00:00:00:00", mac) == 0)
    {
        free(custom_and_iface);
        return NULL;
    }

    DB_LOG( "mac is %20s, interface is %10s, online is %d\n", mac, interface, custom_and_iface->online);

    devinfo *device = NULL;
    device = (devinfo *)calloc(1, sizeof(devinfo));

    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *pkg = NULL;
	char config_buf[16];
	char limitupload[16];
	char limitdownload[16];
	char devicemac[20];

    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, WLDEVLIST, &pkg) == UCI_OK)
    {
        struct uci_section *section;
        char mac_section_name[32] = "";
        memset(mac_section_name, 0, 32);
        strcpy(mac_section_name, mac);
        strrpl(mac_section_name, ':', '_');
        DB_LOG( "======mac_section_name===%s====", mac_section_name);
        section = uci_lookup_section(ctx, pkg, mac_section_name);
        DB_LOG( "======section=====%p===",section);
        if(section)
        {
			device->newdevice = 0;
            const char *value = uci_lookup_option_string(ctx, section, "dev");
            if(value != NULL){
				if(strstr(interface,"lease") == NULL && strstr(value,"lease") != NULL){
					sprintf(cmd,"tscript add %s",mac);
					ret = system(cmd);
					if(ret == -1)
						LOG("ts add system return error\n");
					/*if it is from lease change to main wifi, delete dns blcok*/
					sprintf(cmd,"dec-tool lease del %s", mac);
					DB_LOG("%s",cmd);
					ret = system(cmd);
					if(ret == -1)
						LOG("lease dns block del dev return error\n");
				}else if(strstr(value,"lease") == NULL && strstr(interface,"lease") != NULL){
					sprintf(cmd,"tscript del %s",mac);
					ret = system(cmd);
					if(ret == -1)
						LOG("ts del system return error\n");
				}
            }
        }
        else
        {
			if(strstr(interface,"rai") != NULL){
				strcpy(device->internet, "1");
				strcpy(device->lan, "1");
			}else{
				getUciConfig("sidefault", "acl", "internet",config_buf);
				DB_LOG("default config internet is %s", config_buf);
				if(strncmp(config_buf, "0", 1) == 0){
					strcpy(device->internet, "0");
				}else{
					strcpy(device->internet, "1");
				}
				getUciConfig("sidefault", "acl", "lan",config_buf);
				DB_LOG("default config lan is %s", config_buf);
				if(strncmp(config_buf, "0", 1) == 0){
					strcpy(device->lan, "0");
				}else{
					strcpy(device->lan, "1");
				}
			}
			device->newdevice = 1;
			/*clean wldevlist if too much*/
			sprintf(cmd,"devlistclean &");
			ret = system(cmd);
			if(ret == -1)
				LOG("ts add system return error\n");
        }

        DB_LOG( "======device->newdevice==%d==", device->newdevice);

        uci_unload(ctx,pkg);
    }
    uci_free_context(ctx);

    if(strcmp(isonline, "1") == 0){
        char item[UPTIME_MAXLEN] = "";
        FILE *fd = fopen(UPTIME_PATH, "r");
        int32_t i = 0;
        int32_t item_len = 0;
        int32_t uptime_fetched = 0;
        char uptime_str[UPTIME_REALLEN] = "";
        if(fd){
            fgets(item, UPTIME_MAXLEN, fd);
            item_len = strlen(item);
            for(i=0; i<item_len; i++){
                if(item[i] == '.'){
                    item[i] = '\n';
                    uptime_fetched = 1;
                    break;
                }
            }
            if(uptime_fetched){
                strcpy(uptime_str, item);
                device->associate_time = atoi(uptime_str);
                DB_LOG( "[ssst]===function:%s==fetch uptime success===", __func__);
            }else{
                LOG( "[ssst]===function:%s==fetch uptime failed!!!===", __func__);
            }
            fclose(fd);
        }else{
            LOG( "[ssst]===function:%s==open %s failed!!!===", __func__, UPTIME_PATH);
        }
    }else{
        device->associate_time = -1;
    }
    strcpy(device->macaddr, mac);
    strcpy(device->interface, interface);
    strcpy(device->online, isonline);
// TODO add wireless attr here
	strcpy(device->is_wireless, "1");

    updateDevlistUci(device, device->newdevice);

    DB_LOG( "mac is %20s", device->macaddr);
	if(device->newdevice){
		if(strstr(device->interface,"lease") == NULL){
			sprintf(cmd,"aclscript add %s %s %s; tscript add %s", mac, device->lan, device->internet, mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("system return error\n");
			sprintf(cmd,"online-warn start %s",mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("new wifi device online warning error\n");
		}else{
			sprintf(cmd,"aclscript l_add %s", mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("aclscript lease wifi add dev return error\n");
			sprintf(cmd,"online-warn start %s",mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("new wifi device online warning error\n");
		}
	}else{
		if(strstr(device->interface,"lease") != NULL){
			sprintf(cmd,"aclscript l_add %s", mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("aclscript lease wifi add dev return error\n");
		}else{
			sprintf(cmd,"aclscript l_clean %s", mac);
			DB_LOG("%s",cmd);
			ret = system(cmd);
			if(ret == -1)
				LOG("aclscript lease wifi del dev return error\n");
		}
	}
    if(strcmp(device->online, "1") == 0){              //update the qos setting while the device state switch to online
		struct HttpResonseData response;
		char postData[36] = "{\"version\":\"V10\"}";
        ret = postDataToHttpdCommon(LOCAL_COMMAND_UPDATE_QOS,postData,&response);
        DB_LOG( "==========update_qos====ret=%d",ret);
        if(response.size > 0) free(response.data);
    }
	DB_LOG( "%s !!!!!!! device->notify == %d", device->hostname, device->notify);
	if(device->hostname[0] && device->notify == 1 && strcmp(device->online, "1") == 0){
		Pushinfo push_setting;
		getPushSetting(&push_setting);
		struct tm *cur_time = NULL;
		cur_time = (struct tm *)calloc(1, sizeof(struct tm));
		if(cur_time == NULL)
		{
			LOG( "function:%s  line:%d cur_time calloc failed!", __func__, __LINE__);
			free(device);
			free(custom_and_iface);
			return NULL;
		}

		if(push_setting.enable == 1)
		{
			getCurTime(cur_time);
			DB_LOG( "function:%s  line:%d push_setting.mode == %d || cur_time->tm_hour == %d", __func__, __LINE__, push_setting.mode, cur_time->tm_hour);
			if(push_setting.mode == 0)
			{
				DB_LOG("==========PUSH! PUSH! PUSH!============");
				publishDeviceOnlineEvent(device->newdevice, device->hostname, device->macaddr);
			}
			else if(cur_time->tm_hour >= 8 && cur_time->tm_hour<20)
			{
				DB_LOG("==========PUSH! PUSH! PUSH!============");
				publishDeviceOnlineEvent(device->newdevice, device->hostname, device->macaddr);
			}
		}
		free(cur_time);
	}

	/*RM11455 fix the bug of guest wifi speed limit, add/del device in hnat list*/
	sprintf(devicemac, "%s", device->macaddr);
	for(i = 0;i < strlen(devicemac);i ++){
		if(toascii(devicemac[i]) == 95)
			devicemac[i] = devicemac[i] - 37;
	}
	if(!(strcmp(device->interface, "wlan0-guest"))) {
		getUciConfig("wireless", "guest_radio0", "limitupload", limitupload);
		getUciConfig("wireless", "guest_radio0", "limitdownload", limitdownload);
		if(!(strcmp(limitupload, "0")) && !(strcmp(limitdownload, "0"))) {
			sprintf(cmd, "%s%s%s", "echo 3 ", devicemac, " > /proc/ts");
			ret = system(cmd);
			if(ret == -1)
				LOG("add device into hnat list failed\r\n");
		}
		else {
			sprintf(cmd, "%s%s%s", "echo 2 ", devicemac, " > /proc/ts");
			ret = system(cmd);
			if(ret == -1)
				LOG("del device from hnat list failed\r\n");
		}
	}
	if(!(strcmp(device->interface, "wlan1-guest"))) {
		getUciConfig("wireless", "guest_radio1", "limitupload", limitupload);
		getUciConfig("wireless", "guest_radio1", "limitdownload", limitdownload);
		if(!(strcmp(limitupload, "0")) && !(strcmp(limitdownload, "0"))) {
			sprintf(cmd, "%s%s%s", "echo 3 ", devicemac, " > /proc/ts");
			ret = system(cmd);
			if(ret == -1)
				LOG("add device into hnat list failed\r\n");
		}
		else {
			sprintf(cmd, "%s%s%s", "echo 2 ", devicemac, " > /proc/ts");
			ret = system(cmd);
			if(ret == -1)
				LOG("del device from hnat list failed\r\n");
		}
	}

    free(device);
    free(custom_and_iface);
    LOG( "custom_and_iface free!  device free!");
    return NULL;
}

/*------------------------------------------------------------------*/
/*
 * Print one element from the scanning results
 */
    static inline int
print_event_token(struct wireless_iface *wireless_data, char *data, int len, int online)
{
	DB_LOG("prepare to check new dev\n");

    custom_iface_ctx *custom_and_iface = NULL;
	custom_and_iface = (custom_iface_ctx *)calloc(1, sizeof(custom_iface_ctx));
	if(!custom_and_iface){
		LOG( "custom_and_iface calloc failed!\n");
		return 0;
	}

	custom_and_iface->online = online;
	memcpy(custom_and_iface->custom, data, len);
	strcpy(custom_and_iface->iface,  wireless_data->ifname);
	checknewdev_process(custom_and_iface);

	DB_LOG( "checknewdev_process has finished!");
	return(0);
}

/*------------------------------------------------------------------*/
/*
 * Print out all Wireless Events part of the RTNetlink message
 * Most often, there will be only one event per message, but
 * just make sure we read everything...
 */
    static inline int
print_event_stream(int ifindex, char *data, int len, int online)
{
    struct timeval    recv_time;
    struct timezone   tz;
    struct wireless_iface *   wireless_data;


    /* Get data from cache */
    wireless_data = iw_get_interface_data(ifindex);
    if(wireless_data == NULL){
		LOG("get wireless_data error\n");
        return(-1);
	}
    /* Print received time in readable form */
    gettimeofday(&recv_time, &tz);
	DB_LOG(" ifname is %-8.16s\n", wireless_data->ifname);
	print_event_token(wireless_data, data, len, online);
	/* Push data out *now*, in case we are redirected to a pipe */
	fflush(stdout);

    return(0);
}


/*------------------------------------------------------------------*/
/*
 * Respond to a single RTM_NEWLINK event from the rtnetlink socket.
 */
    static int
LinkCatcher(struct nlmsghdr *nlh)
{
    struct genlmsghdr* ifi;

    ifi = NLMSG_DATA(nlh);

	DB_LOG("nlmsg_type is %d\n", nlh->nlmsg_type);

    /* Check for attributes */
    if (nlh->nlmsg_len > GENL_HDRLEN)
    {
		int ifi_index;
        struct nlattr *attr = (void *) ((char *) ifi + GENL_HDRLEN);

		ifi_index = *((__u32 *)NLA_DATA(attr));
		attr = NLA_NEXT(attr);
		DB_LOG("ifi_index is %d, cmd is %d\n", ifi_index, ifi->cmd);
		/* Go to display it */
		switch(ifi->cmd)
		{
			case NL80211_CMD_NEW_STATION:
				LOG("NEW station\n");
				print_event_stream(ifi_index, NLA_DATA(attr), 6, 1);
				break;
			case NL80211_CMD_DEL_STATION:
				LOG("DEL station\n");
				print_event_stream(ifi_index, NLA_DATA(attr), 6, 0);
				break;
			default:
				break;
		}


    }

    return 0;
}


void handle_netlink_events(struct uloop_fd *rth, unsigned int uevents)
{
    while(1)
    {
        struct sockaddr_nl sanl;
        socklen_t sanllen = sizeof(struct sockaddr_nl);

        struct nlmsghdr *h;
        int amt;
        char buf[8192];

		DB_LOG("get new dev\n");
        amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
        if(amt < 0)
        {
            if(errno != EINTR && errno != EAGAIN)
            {
                LOG("%s: error reading netlink: %s.\n",
                        __PRETTY_FUNCTION__, strerror(errno));
            }
            return;
        }

        if(amt == 0)
        {
            LOG("%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
            return;
        }

        h = (struct nlmsghdr*)buf;
        while(amt >= NLMSG_HDRLEN)
        {
            int len = h->nlmsg_len;
            int l = len - NLMSG_HDRLEN;

            if(l < 0 || len > amt)
            {
                LOG("%s: malformed netlink message: len=%d\n", __PRETTY_FUNCTION__, len);
                break;
            }

			LinkCatcher(h);
            len = NLMSG_ALIGN(len);
            amt -= len;
            h = (struct nlmsghdr*)((char*)h + len);
        }

        if(amt > 0)
            LOG("%s: remnant of size %d on netlink\n", __PRETTY_FUNCTION__, amt);
    }
}

void *check_new_wire_dev(void *arg){
    char postData[64] = "{\"version\":\"V10\",\"arp_check_dev\":\"1\"}";
    struct HttpResonseData response;

    int ret = postDataToHttpdCommon(LOCAL_COMMAND_DEVICE_LIST_BACKSTAGE,postData,&response);
    DB_LOG( "===========periodSendDevinfoEvent======ret=%d",ret);
    if(response.size > 0) free(response.data);
}

void check_dev_from_boot(void){
	pthread_t check_dev_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&check_dev_thread, &attr, &check_new_wire_dev, NULL);
	my_assert(check_dev_thread > 0);
}

void substr(char *dst, char *src, char cut, int is_left)
{
	int i, len;

	if (src == NULL || dst == NULL)
		return;

	len = strlen(src);
	for (i = 0; i < len; i++) {
		if (*(src+i) == cut)
			break;
	}

	// not includ cut
	if (is_left)
		strncpy(dst, src, i);
	else
		strncpy(dst, (src + i + 1), (len - i - 1));
}

int get_wan_port(int *wan_port)
{
	struct uci_package *pkg;
	struct uci_context *ctx = uci_alloc_context();;
	struct uci_section *wan_section, *s;
	struct uci_element *e;
	const char *wan_ifname, *vid, *ports;
	char vid_str[4] = {'\0'}, port_str[4] = {'\0'};

	uci_set_confdir(ctx, "/etc/config");
	if (uci_load(ctx, "network", &pkg) == 0){

		wan_section = uci_lookup_section(ctx, pkg, "wan");
		if (!wan_section)
			goto err_out;

		wan_ifname = uci_lookup_option_string(ctx, wan_section, "ifname");
		if (!wan_ifname)
			goto err_out;

		substr(vid_str, (char *)wan_ifname, '.', 0);
		uci_foreach_element(&pkg->sections, e)
		{
			s = uci_to_section(e);
			if (strcmp(s->type, "switch_vlan"))
				continue;

			vid = uci_lookup_option_string(ctx, s, "vlan");
			if (!vid)
				continue;

			if (!strcmp(vid, vid_str)) {
				ports = uci_lookup_option_string(ctx, s, "ports");
				substr(port_str, (char *)ports, ' ', 1);
				*wan_port = atoi(port_str);
				break;
			}
		}
		uci_unload(ctx, pkg);
	}

	uci_free_context(ctx);
	return 0;

err_out:
	uci_unload(ctx, pkg);
	uci_free_context(ctx);
	return 0;
}

#define SF_CMD_GENERIC      1
void handle_dps_netlink_event(struct uloop_fd *rth, unsigned int uevents){
	struct sockaddr_nl sanl;
	struct nlattr *nlattr;
	msgtemplate_t *genlmsg;
	socklen_t sanllen = sizeof(struct sockaddr_nl);
	int amt, i;
	char buf[1024], str[40], *mac;
	u_int32_t port;
	u_int32_t updown;
	int is_wan = 0, wan_port = -1;


	amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
	if(amt < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
		{
			LOG("%s: error reading netlink: %s.\n",
					__PRETTY_FUNCTION__, strerror(errno));
		}
		return;
	}

	if(amt == 0)
	{
		LOG("%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
		return;
	}


	genlmsg = (msgtemplate_t *)buf;
	if(genlmsg->n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&genlmsg->n), amt)){
		LOG("Genlmsg type is %d, amt is %d, msg len is %d\n", genlmsg->n.nlmsg_type, amt, genlmsg->n.nlmsg_len);
		return;
	}

	if(genlmsg->g.cmd == SF_CMD_GENERIC){
		LOG("cmd SF_CMD_GENERIC\n");
		nlattr = (struct nlattr *)GENLMSG_DATA(genlmsg);
		if(nlattr->nla_type == SF_ETH_CMD_ATTR_DPS_PORT){
			port = *((__u32 *)(NLA_DATA(nlattr)));
			LOG("port is %d\n", port);
		}
		else
			LOG("Not found port");

		nlattr = ATTR_NEXT(nlattr);
		if(nlattr->nla_type == SF_ETH_CMD_ATTR_DPS_LINK){
			updown = *((__u32 *)(NLA_DATA(nlattr)));
			LOG("updown is %d\n", updown);
		}
		else
			LOG("Not found updown");

		nlattr = ATTR_NEXT(nlattr);
		if(nlattr->nla_type == SF_ETH_CMD_ATTR_DPS_MAC){
			mac = (char *)(NLA_DATA(nlattr));
			LOG("mac is %s\n", mac);
		}
		else
			LOG("Not found mac");
	}

	get_wan_port(&wan_port);
	if (port == wan_port)
		is_wan = 1;
	LOG("is_wan %d\n", is_wan);

	if(!is_wan){
		LOG("this port is lan");
		if(updown)
			sprintf(str, "/sbin/dev.sh %d %d %s", port, updown, mac);
		else
			sprintf(str, "/sbin/dev.sh %d %d", port, updown);
		sleep(3);//when a new device connect to router,it can not get ip very soon, so sleep for waiting dhcp finsihed.
		system(str);
	}
	else
		LOG("this port is wan");

	LOG( "dps_check_newdev_process has finished!");
}

#define SF_TS_CMD		1
#define FLOW_DATA		1
void handle_ts_netlink_event(struct uloop_fd *rth, unsigned int uevents){
	struct sockaddr_nl sanl;
	struct nlattr *nlattr;
	msgtemplate_t *genlmsg;
	socklen_t sanllen = sizeof(struct sockaddr_nl);
	int amt, ret, i=0, nl_len;
	char buf[1024], strmac[32], strflow[32], *buff;
	u_int8_t mac[6];
	u_int64_t flow;


	amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
	if(amt < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
		{
			LOG("%s: error reading netlink: %s.\n",
					__PRETTY_FUNCTION__, strerror(errno));
		}
		return;
	}

	if(amt == 0)
	{
		LOG("%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
		return;
	}
	genlmsg = (msgtemplate_t *)buf;
	if(genlmsg->n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&genlmsg->n), amt)){
		LOG("Genlmsg type is %d, amt is %d, msg len is %d\n", genlmsg->n.nlmsg_type, amt, genlmsg->n.nlmsg_len);
		return;
	}
	if(genlmsg->g.cmd == SF_TS_CMD){
		LOG("cmd SF_TS_CMD\n");
		nlattr = (struct nlattr *)GENLMSG_DATA(genlmsg);
		nl_len = genlmsg->n.nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN - NLA_HDRLEN;
		buff = (char *)NLA_DATA(nlattr);
		if(nlattr->nla_type == FLOW_DATA){
			while(nl_len > 6){
				LOG("SF_TS_CMD FLOW_DATA offset %d nl_len %d\n",i, nl_len);
				strncpy(mac, buff+i, 6);
				flow = *((u_int64_t *)(buff+i+6));
				sprintf(strmac,"%2hhX_%2hhX_%2hhX_%2hhX_%2hhX_%2hhX",mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);
				sprintf(strflow,"%lld",flow);
				LOG("save flow mac %s flow %lld", strmac, flow);
				ret = setUciConfig("devlist",strmac, "flow", strflow);
				if(ret < 0){
					ret = setUciConfig("wldevlist",strmac, "flow", strflow);
					if(ret < 0){
						LOG("save flow error mac %s flow %lld", strmac, flow);
					}
				}
				i += 6 + sizeof(u_int64_t);
				nl_len -= i;
			}
		}
	}
/*
	for(i=0;i < 32;i++){
		printf("%2hhx %2hhx %2hhx %2hhx\n",buf[i*4+0],buf[i*4+1],buf[i*4+2],buf[i*4+3]);
	}
*/
	LOG( "ts netlink event has finished!");
}

#ifdef USE_CUSTOM_SOCKET
struct wifi_custom_data
{
	u_int8_t mac[6];
	int online;
	char ifname[20];
} __attribute__((packed));

void handle_custom_wifi_events(struct uloop_fd *rth, unsigned int uevents)
{
	struct sockaddr_nl sanl;
	socklen_t sanllen = sizeof(struct sockaddr_nl);
	int data_len = 0,buf_len = 0;
	char buf[1024];

	struct wifi_custom_data *custom_data;
	u_int8_t mac[6];
	custom_iface_ctx *custom_and_iface = NULL;
	buf_len = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
	if(buf_len >= sizeof(struct wifi_custom_data))
	{
		custom_data = (struct wifi_custom_data *)buf;
		custom_and_iface = (custom_iface_ctx *)calloc(1, sizeof(custom_iface_ctx));
		memset(custom_and_iface, 0, sizeof(custom_iface_ctx));
		if(!custom_and_iface){
			LOG( "custom_and_iface calloc failed!\n");
			return;
		}

		custom_and_iface->online = custom_data->online;
		memcpy(custom_and_iface->custom, custom_data->mac, 6);
		strcpy(custom_and_iface->iface, custom_data->ifname);
		checknewdev_process(custom_and_iface);
	}
	return;
}
#endif

#define		ADD_ACTION	"add"
#define		RM_ACTION	"remove"
static int action_status = 2;

void handle_usb_netlink_uevent(struct uloop_fd *rth, unsigned int uevents){
	char buf[256] = {0};
	char data[32];
	int ret, usb_action ;

	ret = recv(rth->fd, buf, sizeof(buf), 0);
	if (ret < 0){
		LOG("Receive msg error %d!\n", ret);
		return;
	}

	DB_LOG( "%s\n",buf);
	if (strstr(buf, "usb")){
		if(strncmp(buf, ADD_ACTION, sizeof(ADD_ACTION) - 1) == 0){
			usb_action = 0;
		}else if(strncmp(buf, RM_ACTION, sizeof(RM_ACTION) -1) == 0){
			usb_action = 1;
		}else{
			return;
		}
		/*
		 *when a usb device add or remove,then kernel will send some add/remove message to userspace,
		 *so we save usb action status, do not send the same action message twice.
		 */
		if (usb_action != action_status){
			LOG( "action is %d\n",usb_action);
			prepareStorageChangeData(usb_action, data);
			ret = publishRouterEvent(ROUTER_EVENT_STORAGE_CHANGE, data);
			ret = publishEventToUser(ROUTER_EVENT_STORAGE_CHANGE, data);
			action_status = usb_action;
		}
        return;
	}
}

void *arpevent_process(void *arg){
	struct arp_info *info = (struct arp_info *)arg;
    struct HttpResonseData response;
	char postData[256] = "";
	u_int8_t macaddr[32];
	u_int8_t ipaddr[32];
	int ret = 0;

	sprintf(macaddr,"%02X:%02X:%02X:%02X:%02X:%02X",info->mac[0],info->mac[1],info->mac[2],info->mac[3],info->mac[4],info->mac[5]);
	sprintf(ipaddr,"%u.%u.%u.%u",info->ip[0],info->ip[1],info->ip[2],info->ip[3]);
	sprintf(postData,"{\"version\":\"V10\",\"ip\":\"%s\",\"mac\":\"%s\",\"arp_check_dev\":\"1\"}",ipaddr,macaddr);

    DB_LOG( "arp event ip %u.%u.%u.%u mac %s\n",info->ip[0],info->ip[1],info->ip[2],info->ip[3],macaddr);

    ret = postDataToHttpdCommon(LOCAL_COMMAND_ARP_CHECK_DEV, postData, &response);
    DB_LOG( "===========Send arp check dev event======ret=%d",ret);

	if(response.size > 0 && strstr(response.data, "notify") != NULL){
		struct tm *cur_time = NULL;
		char hostname[128] = "";
        char mac_section_name[32] = "";

        memset(mac_section_name, 0, 32);
        strcpy(mac_section_name, macaddr);
        strrpl(mac_section_name, ':', '_');
		ret = getUciConfig("wldevlist", mac_section_name, "hostname",hostname);
		if(ret){
			DB_LOG("Get hostname error");
//			strcpy(hostname,"unknow");
		}
		Pushinfo push_setting;
		getPushSetting(&push_setting);

		cur_time = (struct tm *)calloc(1, sizeof(struct tm));
		if(cur_time == NULL)
		{
			LOG( "function:%s  line:%d cur_time calloc failed!", __func__, __LINE__);
			goto done;
		}

		if(push_setting.enable == 1)
		{
			getCurTime(cur_time);
			if(push_setting.mode == 0)
			{
				DB_LOG("==========PUSH! PUSH! PUSH!============");
				publishDeviceOnlineEvent(1, hostname, macaddr);
			}
			else if(cur_time->tm_hour >= 8 && cur_time->tm_hour<20)
			{
				DB_LOG("==========PUSH! PUSH! PUSH!============");
				publishDeviceOnlineEvent(1, hostname, macaddr);
			}
		}
		free(cur_time);
	}
done:
   if(response.size > 0) free(response.data);
	free(info);
	pthread_exit((void *)0);
}

void do_arp_event(struct ndmsg *ndm, u_int32_t ndml){
	struct rtattr *arp_attr = NDA_RTA(ndm);
	u_int32_t attrlen = ndml - NLMSG_LENGTH(sizeof(*ndm));
	u_int8_t ip[4];
	u_int8_t mac[6];
	u_int8_t flag = 0;
	char status[32] = {0};
	char tmp_mac[32];

	DB_LOG("Start deal arp event %d, state is %d", ndm->ndm_family, ndm->ndm_state);
	if(ndm->ndm_family == AF_INET && (ndm->ndm_state & NUD_REACHABLE) && ndm->ndm_ifindex != wan_ifi){
		while(RTA_OK(arp_attr, attrlen)){
			if(arp_attr->rta_type == NDA_DST){
				memcpy(ip, RTA_DATA(arp_attr), 4);
				flag++;
			}
			if(arp_attr->rta_type == NDA_LLADDR){
				memcpy(mac, RTA_DATA(arp_attr), 6);
				flag++;
			}
			arp_attr = RTA_NEXT(arp_attr, attrlen);
		}
		if(flag < 2){
			LOG("Get arp information from netlink error\n");
			return;
		}

		struct arp_info *info = NULL;
		info = (struct arp_info *)calloc(1, sizeof(struct arp_info));
		if(!info){
			LOG( "info calloc failed!\n");
			return;
		}

		memcpy(info->ip, ip, 4);
		memcpy(info->mac, mac, 6);
		sprintf(tmp_mac,"%02X_%02X_%02X_%02X_%02X_%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		getUciConfig("devlist",tmp_mac, "online", status);
		DB_LOG("device %s status is %s", tmp_mac, status);
		if(strncmp(status, "1",1) != 0){
			pthread_t arpevent_thread;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			pthread_create(&arpevent_thread, &attr, &arpevent_process, (void *)info);
			my_assert(arpevent_thread > 0);
			if(arpevent_thread <= 0)
			{
				LOG( "can not create arpevent_thread!\n");
			}
		}else{
			free(info);
		}
	}
	return;
}

void handle_arp_netlink_event(struct uloop_fd *rth, unsigned int uevents)
{
	struct sockaddr_nl sanl;
	socklen_t sanllen = sizeof(struct sockaddr_nl);

	struct nlmsghdr *h;
	int amt;
	char buf[512];

	amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
	if(amt < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
		{
			LOG("%s: error reading netlink: %s.\n",
					__PRETTY_FUNCTION__, strerror(errno));
		}
		return;
	}

	if(amt == 0)
	{
		LOG("%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
		return;
	}
	while(amt >= NLMSG_HDRLEN){
		h = (struct nlmsghdr*)buf;
		DB_LOG("len is %d, nl type is %d", amt, h->nlmsg_type);
		u_int32_t len = h->nlmsg_len;
		u_int32_t l = len - NLMSG_HDRLEN;
		if(h->nlmsg_type == RTM_NEWNEIGH)
		{

			if(l < 0 || len > amt)
			{
				LOG("%s: malformed netlink message: len=%d\n", __PRETTY_FUNCTION__, len);
				return;
			}

			do_arp_event(NLMSG_DATA(h), len);
		}
		amt -= NLMSG_ALIGN(len);
		h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
	}
}

#if 0
/* ---------------------------------------------------------------- */
/*
 * Wait until we get an event
*/
int wait_for_event(struct rtnl_handle * rth)
{

	struct rtnl_handle *dps_rth = &rth[1];
	struct rtnl_handle *uevent_rth = &rth[2];
	struct rtnl_handle *arp_rth = &rth[3];
	struct rtnl_handle *ts_rth = &rth[4];
#ifdef USE_CUSTOM_SOCKET
	struct rtnl_handle *custom_wifi_rth = &rth[5];
#endif
	int       last_fd;    /* Last fd */

#ifndef USE_CUSTOM_SOCKET
    last_fd = rth->fd > dps_rth->fd ? rth->fd : dps_rth->fd;
	last_fd = last_fd > uevent_rth->fd ? last_fd : uevent_rth->fd;
#else
	last_fd = dps_rth->fd > uevent_rth->fd ? dps_rth->fd : uevent_rth->fd;
#endif

	last_fd = last_fd > arp_rth->fd ? last_fd : arp_rth->fd;
	last_fd = last_fd > ts_rth->fd ? last_fd : ts_rth->fd;
#ifdef USE_CUSTOM_SOCKET
	last_fd = last_fd > custom_wifi_rth->fd ? last_fd : custom_wifi_rth->fd;
#endif
    /* Forever */
    while(!g_force_exit_signal)
    {
        fd_set        rfds;       /* File descriptors for select */
        int       ret;
        /* Guess what ? We must re-generate rfds each time */
        FD_ZERO(&rfds);
#ifndef USE_CUSTOM_SOCKET
        FD_SET(rth->fd, &rfds);
#endif
		FD_SET(dps_rth->fd, &rfds);
		FD_SET(uevent_rth->fd, &rfds);
		FD_SET(arp_rth->fd, &rfds);
		FD_SET(ts_rth->fd, &rfds);
#ifdef USE_CUSTOM_SOCKET
		FD_SET(custom_wifi_rth->fd, &rfds);
#endif
        /* Wait until something happens */
        ret = select(last_fd + 1, &rfds, NULL, NULL, NULL);

        /* Check if there was an error */
        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EINTR)
                continue;
            LOG("Unhandled signal - exiting...\n");
            break;
        }

        /* Check if there was a timeout */
        if(ret == 0)
        {
            continue;
        }

        /* Check for interface discovery events. */
        if(FD_ISSET(rth->fd, &rfds))
            handle_netlink_events(rth);
		if(FD_ISSET(dps_rth->fd, &rfds)){
			DB_LOG( "get wire dev information!\n");
			handle_dps_netlink_event(dps_rth);
		}
		if(FD_ISSET(uevent_rth->fd, &rfds)){
			LOG("get uevent information!\n");
			handle_usb_netlink_uevent(uevent_rth);
		}
		if(FD_ISSET(arp_rth->fd, &rfds)){
			DB_LOG( "get arp event information!\n");
			handle_arp_netlink_event(arp_rth);
		}
		if(FD_ISSET(ts_rth->fd, &rfds)){
			DB_LOG( "get ts event information!\n");
			handle_ts_netlink_event(ts_rth);
		}
#ifdef USE_CUSTOM_SOCKET
		if(FD_ISSET(custom_wifi_rth->fd, &rfds)){
			DB_LOG( "get custom wifi event information!\n");
			handle_custom_wifi_events(custom_wifi_rth);
		}
#endif
    }

    return(0);
}
#endif
