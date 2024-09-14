#ifndef _SF_ESWITCH_ETHTOOL_H_
#define _SF_ESWITCH_ETHTOOL_H_


#define GSW_MAX_PORTS			5
#define DRV_MODULE_NAME			"gswicth"
#define DRV_MODULE_VERSION		"1.0.0"

#define GMAC_NUM_STATS      15
#define ETH_GSTRING_LEN     32

// generic phy reg
#define PHY_BASE_CONTROL_REG		0x0
#define PHY_BASE_STATUS_REG		0x1
#define PHY_AUTONEG_ADVERTISE_REG	0x4
#define PHY_AUTONEG_LINK_PARTNER_REG	0x5
#define PHY_1000BASE_CTRL_REG		0x9
#define PHY_1000BASE_STAT_REG		0xa
#define PHY_MODEL_CTRL_REG		0x11

// generic phy mask bit
#define GSW_ANENABLE			0x1000
#define GSW_ANRESTART			0x0200
#define GSW_ANEGCOMPLETE		0x0020
#define GSW_SPEED1000			0x0040
#define GSW_SPEED100			0x2000
#define GSW_FULLDPLX			0x0100


#endif
