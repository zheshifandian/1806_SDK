/**
 ****************************************************************************************
 *
 * @file siwifi_testmode.c
 *
 * @brief Test mode function definitions
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

#include <net/mac80211.h>
#include <net/netlink.h>

#include "siwifi_testmode.h"
#include "siwifi_msg_tx.h"
#include "reg_access.h"

/*
 * This function handles the user application commands for register access.
 *
 * It retrieves command ID carried with SIWIFI_TM_ATTR_COMMAND and calls to the
 * handlers respectively.
 *
 * If it's an unknown commdn ID, -ENOSYS is returned; or -ENOMSG if the
 * mandatory fields(SIWIFI_TM_ATTR_REG_OFFSET,SIWIFI_TM_ATTR_REG_VALUE32)
 * are missing; Otherwise 0 is replied indicating the success of the command execution.
 *
 * If SIWIFI_TM_ATTR_COMMAND is SIWIFI_TM_CMD_APP2DEV_REG_READ, the register read
 * value is returned with SIWIFI_TM_ATTR_REG_VALUE32.
 *
 * @hw: ieee80211_hw object that represents the device
 * @tb: general message fields from the user space
 */
int siwifi_testmode_reg(struct ieee80211_hw *hw, struct nlattr **tb)
{
    struct siwifi_hw *siwifi_hw = hw->priv;
    u32 mem_addr, val32;
    struct sk_buff *skb;
    int status = 0;

    /* First check if register address is there */
    if (!tb[SIWIFI_TM_ATTR_REG_OFFSET]) {
        printk("Error finding register offset\n");
        return -ENOMSG;
    }

    mem_addr = nla_get_u32(tb[SIWIFI_TM_ATTR_REG_OFFSET]);

    switch (nla_get_u32(tb[SIWIFI_TM_ATTR_COMMAND])) {
    case SIWIFI_TM_CMD_APP2DEV_REG_READ:
        {
            struct dbg_mem_read_cfm mem_read_cfm;

            /*** Send the command to the LMAC ***/
            if ((status = siwifi_send_dbg_mem_read_req(siwifi_hw, mem_addr, &mem_read_cfm)))
                return status;

            /* Allocate the answer message */
            skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, 20);
            if (!skb) {
                printk("Error allocating memory\n");
                return -ENOMEM;
            }

            val32 = mem_read_cfm.memdata;
            if (nla_put_u32(skb, SIWIFI_TM_ATTR_REG_VALUE32, val32))
                goto nla_put_failure;

            /* Send the answer to upper layer */
            status = cfg80211_testmode_reply(skb);
            if (status < 0)
                printk("Error sending msg : %d\n", status);
        }
        break;

    case SIWIFI_TM_CMD_APP2DEV_REG_WRITE:
        {
            if (!tb[SIWIFI_TM_ATTR_REG_VALUE32]) {
                printk("Error finding value to write\n");
                return -ENOMSG;
            } else {
                val32 = nla_get_u32(tb[SIWIFI_TM_ATTR_REG_VALUE32]);
                /* Send the command to the LMAC */
                if ((status = siwifi_send_dbg_mem_write_req(siwifi_hw, mem_addr, val32)))
                    return status;
            }
        }
        break;

    default:
        printk("Unknown testmode register command ID\n");
        return -ENOSYS;
    }

    return status;

nla_put_failure:
    kfree_skb(skb);
    return -EMSGSIZE;
}

/*
 * This function handles the user application commands for Debug filter settings.
 *
 * @hw: ieee80211_hw object that represents the device
 * @tb: general message fields from the user space
 */
int siwifi_testmode_dbg_filter(struct ieee80211_hw *hw, struct nlattr **tb)
{
    struct siwifi_hw *siwifi_hw = hw->priv;
    u32 filter;
    int status = 0;

    /* First check if the filter is there */
    if (!tb[SIWIFI_TM_ATTR_REG_FILTER]) {
        printk("Error finding filter value\n");
        return -ENOMSG;
    }

    filter = nla_get_u32(tb[SIWIFI_TM_ATTR_REG_FILTER]);
    SIWIFI_DBG("testmode debug filter, setting: 0x%x\n", filter);

    switch (nla_get_u32(tb[SIWIFI_TM_ATTR_COMMAND])) {
    case SIWIFI_TM_CMD_APP2DEV_SET_DBGMODFILTER:
        {
            /* Send the command to the LMAC */
            if ((status = siwifi_send_dbg_set_mod_filter_req(siwifi_hw, filter)))
                return status;
        }
        break;
    case SIWIFI_TM_CMD_APP2DEV_SET_DBGSEVFILTER:
        {
            /* Send the command to the LMAC */
            if ((status = siwifi_send_dbg_set_sev_filter_req(siwifi_hw, filter)))
                return status;
        }
        break;

    default:
        printk("Unknown testmode register command ID\n");
        return -ENOSYS;
    }

    return status;
}

/*
 * This function handles the user application commands for register access without using
 * the normal LMAC messaging way.
 * This time register access will be done through direct PCI BAR windows. This can be used
 * to access registers even when the :AMC FW is stuck.
 *
 * @hw: ieee80211_hw object that represents the device
 * @tb: general message fields from the user space
 */
int siwifi_testmode_reg_dbg(struct ieee80211_hw *hw, struct nlattr **tb)
{
#if 0
    struct siwifi_hw *siwifi_hw = hw->priv;
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
#endif
    u32 mem_addr;
    struct sk_buff *skb;
    int status = 0;
    volatile unsigned int reg_value = 0;
    unsigned int offset;

    /* First check if register address is there */
    if (!tb[SIWIFI_TM_ATTR_REG_OFFSET]) {
        printk("Error finding register offset\n");
        return -ENOMSG;
    }

    mem_addr = nla_get_u32(tb[SIWIFI_TM_ATTR_REG_OFFSET]);
    offset = mem_addr & 0x00FFFFFF;

    switch (nla_get_u32(tb[SIWIFI_TM_ATTR_COMMAND])) {
    case SIWIFI_TM_CMD_APP2DEV_REG_READ_DBG:
        {
#if 1
        //todo: ycchang sync with siwifi_patform.h
        reg_value = REG_PL_RD(mem_addr);
#else
        /*** Send the command to the LMAC ***/
        reg_value = SIWIFI_REG_READ(siwifi_plat, SIWIFI_ADDR_SYSTEM, offset);
#endif

        /* Allocate the answer message */
        skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, 20);
        if (!skb) {
            printk("Error allocating memory\n");
            return -ENOMEM;
        }

        if (nla_put_u32(skb, SIWIFI_TM_ATTR_REG_VALUE32, reg_value))
            goto nla_put_failure;

        /* Send the answer to upper layer */
        status = cfg80211_testmode_reply(skb);
        if (status < 0)
            printk("Error sending msg : %d\n", status);
    } break;

    case SIWIFI_TM_CMD_APP2DEV_REG_WRITE_DBG:
        {
            if (!tb[SIWIFI_TM_ATTR_REG_VALUE32]) {
                printk("Error finding value to write\n");
                return -ENOMSG;
            } else {
                reg_value = nla_get_u32(tb[SIWIFI_TM_ATTR_REG_VALUE32]);

                /* Send the command to the LMAC */
#if 1
            //todo: ycchang sync with siwifi_patform.h
            REG_PL_WR(reg_value, mem_addr);
#else
                SIWIFI_REG_WRITE(reg_value, siwifi_plat, SIWIFI_ADDR_SYSTEM,
                               offset);
#endif
            }
        }
        break;

    default:
        printk("Unknown testmode register command ID\n");
        return -ENOSYS;
    }

    return status;

nla_put_failure:
    kfree_skb(skb);
    return -EMSGSIZE;
}
