#ifndef _SF_YT9215_API_H_
#define _SF_YT9215_API_H_

#include "sfa18_gmac.h"

/* SMI format */
#define REG_ADDR_BIT1_ADDR      0
#define REG_ADDR_BIT1_DATA      1
#define REG_ADDR_BIT0_WRITE     0
#define REG_ADDR_BIT0_READ      1

#define YT9215_PHY_ADDR		0x1d
#define YT_DEFAULT_ID		0x0	/* switch_id_1, switch_id_0 */
#define YT_SW_ID_9215		0x9002

#define CHIP_INTERFACE_MAC8		0x80400
#define CHIP_INTERFACE_MAC9		0x80408

#define CHIP_INTERFACE_SELECT_REG	0x80394
#define CHIP_INTERFACE_CTRL_REG		0x80028
#define CHIP_CHIP_ID_REG		0x80008
#define CHIP_CHIP_MODE_REG		0x80388

#define SWCHIP_YT9215RB			0x3
#define SWCHIP_YT9215S          0x2
#define SWCHIP_YT9215SC          0x1

#define MAC8_SPEED_SET			0x80120
#define MAC9_SPEED_SET			0x80124

#define SFA18_YT9215_GMAC_TX_DELAY	0x29
#define SFA18_YT9215_GMAC_RX_DELAY	0xb

void yt9215_reg_read(struct sgmac_priv *priv, u32 reg_addr, u32 *reg_val);
void yt9215_reg_write(struct sgmac_priv *priv, u32 reg_addr, u32 reg_value);

#endif /* _SF_YT9215_API_H_ */
