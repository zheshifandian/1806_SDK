/**
 ****************************************************************************************
 *
 * @file siwifi_ioctl.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */
#ifndef _SIWIFI_IOCTL_H_
#define _SIWIFI_IOCTL_H_

#include "siwifi_defs.h"
#include "siwifi_tx.h"

#define ETH_P_SFCFG 0x1688
#define SFCFG_MAGIC_NO      0x18181688

/*CMD TYPE*/
/*  command id */
#define SFCFG_CMD_ATE_START			0x0000
#define SFCFG_CMD_ATE_STOP   		0x0001
#define SFCFG_CMD_ATE_TX_START		0x0002
#define SFCFG_CMD_ATE_TX_STOP		0x0003
#define SFCFG_CMD_ATE_RX_START		0x0004
#define SFCFG_CMD_ATE_RX_STOP		0x0005
#define SFCFG_CMD_ATE_TX_FRAME_START 0x0006
#define SFCFG_CMD_ATE_TX_CONT_START  0x0007
#define SFCFG_CMD_ATE_RX_FRAME_START 0x0008
#define SFCFG_CMD_ATE_MACBYPASS_TX_START 0x0009
#define SFCFG_CMD_ATE_MACBYPASS_TX_STOP  0x000a
#define SFCFG_CMD_ATE_TX_TEST_TONE_START 0x000b
#define SFCFG_CMD_ATE_TX_TEST_TONE_STOP  0x000c

#define SFCFG_CMD_ATE_SET_BANDWIDTH	   0x0100
#define SFCFG_CMD_ATE_SET_CHANNEL	   0x0101
#define SFCFG_CMD_ATE_SET_PHY_MODE	   0x0102
#define SFCFG_CMD_ATE_SET_RATE         0x0103
#define SFCFG_CMD_ATE_SET_PREAMBLE     0x0104
#define SFCFG_CMD_ATE_SET_GI           0x0105
#define SFCFG_CMD_ATE_SET_POWER        0x0106
#define SFCFG_CMD_ATE_GET_INFO         0x0107
#define SFCFG_CMD_ATE_SAVE_TO_MTD	   0x0108
#define SFCFG_CMD_ATE_SET_CENTER_FREQ1 0x0109
#define SFCFG_CMD_ATE_SET_FRAME_BW	   0x010a

#define SFCFG_CMD_ATE_WHOLE_FRAME     0x1000
#define SFCFG_CMD_ATE_TX_COUNT        0x1001
#define SFCFG_CMD_ATE_PAYLOAD_LENGTH  0x1002
#define SFCFG_CMD_ATE_TX_FC          0x1003
#define SFCFG_CMD_ATE_TX_DUR         0x1004
#define SFCFG_CMD_ATE_TX_BSSID       0x1005
#define SFCFG_CMD_ATE_TX_DA          0x1006
#define SFCFG_CMD_ATE_TX_SA          0x1007
#define SFCFG_CMD_ATE_TX_SEQC        0x1008
#define SFCFG_CMD_ATE_PAYLOAD        0x1009
#define SFCFG_CMD_ATE_TX_FRAME_BW	 0x100a

#define SFCFG_CMD_ATE_READ_REG           0x10000
#define SFCFG_CMD_ATE_WRITE_REG          0x10001

#define SIWIFI_IOCTL_RET_SUCCESS                   0x00
#define SIWIFI_IOCTL_RET_FAILURE                   0x01
#define SIWIFI_IOCTL_RET_INVALID_DATA              0x02
#define SIWIFI_IOCTL_RET_NO_RESOURCE               0x03

struct siwifi_ioctl_cfg {
    uint32_t magic_no;
    uint32_t command_type;
    uint32_t command_id;
    uint16_t length;
    uint16_t sequence;
    uint32_t status;
    uint8_t data[4096];
}__attribute__((packed));

//back to user space  /sync to user space parameter
struct siwifi_ioctl_ate_dump_info {
    int bandwidth;
    int frame_bandwidth;
    int band;
    int freq;
    int freq1;
    int rate;
    int mode;
    int gi;
    int pre;
    int power;
    int len;
    int rssi;
    int reg_status;
    uint32_t xof1;
    uint32_t xof2;
    uint32_t count;
    uint32_t tx_cont;
    uint32_t tx_successful;
    uint32_t tx_retry;
    uint32_t rec_rx_count;
    uint32_t fcs_err;
    uint32_t per;
    uint32_t phy_err;
	uint32_t fcs_ok;
	uint32_t fcs_ok_for_macaddr;
	uint32_t fcs_group;
    uint32_t reg_val;
    uint8_t bssid[12];
    uint8_t da[12];
    uint8_t sa[12];
}__attribute__((packed));

enum register_mode {
    MAC_REG,
    PHY_REG,
    RF_REG,
};
//sync to user space parameter
struct tx_frame_param {
    uint32_t frame_num;
};

void siwifi_ate_task(unsigned long data);

void siwifi_ate_rx_cb_rssi(struct siwifi_hw *siwifi_hw, struct sk_buff *skb);
void siwifi_ate_tx_cb(struct siwifi_hw *siwifi_hw, struct sk_buff *skb);

int siwifi_ops_ioctl(struct siwifi_hw *siwifi_hw, struct ifreq *rq, int cmd);

#endif /* _SIWIFI_IOCTL_H_ */
