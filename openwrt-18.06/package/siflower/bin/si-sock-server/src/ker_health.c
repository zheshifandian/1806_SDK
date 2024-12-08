#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "ssst_request.h"
#include "utils.h"
#include "iwevent.h"
#include "ker_health.h"
#include "publish.h"
#define SF_CMD_GENERIC		1
#define SF_CMD_ATTR_DPS		1
#define DAILY_TIMEOUT 86400

int handle_health_event(struct rtnl_handle *rth){
	struct sockaddr_nl sanl;
	struct nlattr *nlattr;
	msgtemplate_t *genlmsg;
	socklen_t sanllen = sizeof(struct sockaddr_nl);
	int amt, nl_len, i = 0, ret = 0;
	char buf[1024];
    char *buff, *msg[6];
    struct timeval tv;
    tv.tv_sec = DAILY_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(rth->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	amt = recvfrom(rth->fd, buf, sizeof(buf), 0, (struct sockaddr*)&sanl, &sanllen);
	if(amt < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
		{
			LOG("%s: error reading netlink: %s.\n",
					__PRETTY_FUNCTION__, strerror(errno));
		}
		return -1;
	}
	if(amt == 0)
	{
		LOG("%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
		return -1;
	}
	genlmsg = (msgtemplate_t *)buf;
	if(genlmsg->n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&genlmsg->n), amt)){
		LOG("Genlmsg type is %d, amt is %d, msg len is %d\n", genlmsg->n.nlmsg_type, amt, genlmsg->n.nlmsg_len);
		return -1;
	}
	sleep(3);
    if(genlmsg->g.cmd == SF_CMD_GENERIC){
		LOG("cmd SF_CMD_GENERIC\n");
		nlattr = (struct nlattr *)GENLMSG_DATA(genlmsg);
		nl_len = genlmsg->n.nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN - NLA_HDRLEN;
		buff = (char *)NLA_DATA(nlattr);
        LOG("buff is %s\n",buff);
        msg[i] = strtok(buff, " ");
        while(msg[i]){
            i++;
            msg[i] = strtok(NULL, " ");
        }
        ret = kernelLogToServer(msg[0], msg[1], msg[2], msg[3], msg[4], msg[5]);
    }
    return ret;
}

int sys_sta_process(void* args)
{
	struct rtnl_handle    rth[1];
	int ret = 0, hlt_group;

	if(rtnl_open(&rth[0], 0, NETLINK_GENERIC) < 0)
	{
		perror("Can't initialize rtnetlink socket");
		return -1;
	}
	hlt_group = get_genl_group(&rth[0], "COMMON_NL", "common_nl");
	if (hlt_group < 0){
		LOG("hlt_group number is %d",hlt_group);
		goto err1;
	}

	ret = rtnl_open(&rth[0], 1 << (hlt_group - 1), NETLINK_GENERIC);
	if (ret < 0){
		perror("Can't initialize generic netlink\n");
		goto err1;
    }

    ret = handle_health_event(rth);

err1:
	rtnl_close(&rth[0]);

	return ret;
}

