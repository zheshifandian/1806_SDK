#ifndef _SF_YT9215RB_OPS_H_
#define _SF_YT9215RB_OPS_H_

/* SMI format */
#define REG_ADDR_BIT1_ADDR      0
#define REG_ADDR_BIT1_DATA      1
#define REG_ADDR_BIT0_WRITE     0
#define REG_ADDR_BIT0_READ      1

#define YT9215_PHY_ADDR		0x1d
#define YT_DEFAULT_ID		0x0	/* switch_id_1, switch_id_0 */

#define CHIP_INTERFACE_MAC8		0x80400
#define CHIP_INTERFACE_MAC9		0x80408


#define MAC8_SPEED_SET			0x80120
#define MAC9_SPEED_SET			0x80124

#define SFA18_YT9215_GMAC_TX_DELAY	0x2
#define SFA18_YT9215_GMAC_RX_DELAY	0x0

extern struct sf_eswitch_api_t yt9215rb_api;
extern uint32_t yt9215_port_nums;
extern uint32_t yt9215_cpu_port;

int yt9215rb_getAsicReg(u32 reg_addr, u32 *reg_val);
int yt9215rb_setAsicReg(u32 reg_addr, u32 reg_val);
int yt9215rb_setAsicPHYReg(unsigned int phyNo, unsigned int phyAddr, unsigned int pRegData);
int yt9215rb_getAsicPHYReg(unsigned int phyNo, unsigned int phyAddr, unsigned int *pRegData);
#endif /* _SF_YT9215RB_OPS_H_ */
