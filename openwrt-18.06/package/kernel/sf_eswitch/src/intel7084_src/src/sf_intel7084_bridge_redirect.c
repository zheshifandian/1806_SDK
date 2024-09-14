#include <linux/inet.h>
#include "../../sf_eswitch.h"
#include "gsw_sw_init.h"

#define RULE_INDEX 1
#define pr_fmt(fmt) "sf_bridge_redirect_hw: " fmt

extern ethsw_api_dev_t *pedev0[GSW_DEV_MAX];

static GSW_return_t intel7084_bridge_redirect_hw_apply(u32 daddr)
{
	GSW_return_t ret;

	GSW_PCE_rule_t rule = {
		.pattern = {
			.nIndex		= RULE_INDEX,
			.bEnable	= LTQ_TRUE,
			.eDstIP_Select	= GSW_PCE_IP_V4,
			.nDstIP.nIPv4	= daddr,
			.nDstIP_Mask	= 0xff00,
		},

		.action = {
			.ePortMapAction	= GSW_PCE_ACTION_PORTMAP_ALTERNATIVE,
			.nForwardPortMap	= BIT(5), /* CPU port */
		},
	};

	SF_MDIO_LOCK();
	ret = GSW_PceRuleWrite(pedev0[0], &rule);
	SF_MDIO_UNLOCK();

	if (ret == GSW_statusOk)
		pr_info("enabled addr %08x\n", daddr);

	return ret;
}

static GSW_return_t intel7084_bridge_redirect_hw_remove(void)
{
	GSW_return_t ret;

	GSW_PCE_ruleDelete_t del = {
		.nIndex	= RULE_INDEX,
	};

	SF_MDIO_LOCK();
	ret = GSW_PceRuleDelete(pedev0[0], &del);
	SF_MDIO_UNLOCK();

	if (ret == GSW_statusOk)
		pr_info("disabled\n");

	return ret;
}

int intel7084_bridge_redirect_ip(const char *ip)
{
	__be32 daddr;

	if (!ip || !ip[0]) {
		pr_err("please specify parameter ip!\n");
		return -EINVAL;
	}

	if (!in4_pton(ip, -1, (u8 *)&daddr, -1, NULL)) {
		pr_err("invalid IP address \"%s\"\n", ip);
		return -EINVAL;
	}

	return intel7084_bridge_redirect_hw_apply(ntohl(daddr));
}

int intel7084_bridge_redirect_disable(void)
{
	return intel7084_bridge_redirect_hw_remove();
}
