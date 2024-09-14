/**
 ****************************************************************************************
 *
 * @file siwifi_testmode.h
 *
 * @brief Test mode function declarations
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

#ifndef SIWIFI_TESTMODE_H_
#define SIWIFI_TESTMODE_H_

#include <net/mac80211.h>
#include <net/netlink.h>

/* Commands from user space to kernel space(SIWIFI_TM_CMD_APP2DEV_XX) and
 * from and kernel space to user space(SIWIFI_TM_CMD_DEV2APP_XX).
 * The command ID is carried with SIWIFI_TM_ATTR_COMMAND.
 */
enum siwifi_tm_cmd_t {
    /* commands from user application to access register */
    SIWIFI_TM_CMD_APP2DEV_REG_READ = 1,
    SIWIFI_TM_CMD_APP2DEV_REG_WRITE,

    /* commands from user application to select the Debug levels */
    SIWIFI_TM_CMD_APP2DEV_SET_DBGMODFILTER,
    SIWIFI_TM_CMD_APP2DEV_SET_DBGSEVFILTER,

    /* commands to access registers without sending messages to LMAC layer,
     * this must be used when LMAC FW is stuck. */
    SIWIFI_TM_CMD_APP2DEV_REG_READ_DBG,
    SIWIFI_TM_CMD_APP2DEV_REG_WRITE_DBG,

    SIWIFI_TM_CMD_MAX,
};

enum siwifi_tm_attr_t {
    SIWIFI_TM_ATTR_NOT_APPLICABLE = 0,

    SIWIFI_TM_ATTR_COMMAND,

    /* When SIWIFI_TM_ATTR_COMMAND is SIWIFI_TM_CMD_APP2DEV_REG_XXX,
     * The mandatory fields are:
     * SIWIFI_TM_ATTR_REG_OFFSET for the offset of the target register;
     * SIWIFI_TM_ATTR_REG_VALUE32 for value */
    SIWIFI_TM_ATTR_REG_OFFSET,
    SIWIFI_TM_ATTR_REG_VALUE32,

    /* When SIWIFI_TM_ATTR_COMMAND is SIWIFI_TM_CMD_APP2DEV_SET_DBGXXXFILTER,
     * The mandatory field is SIWIFI_TM_ATTR_REG_FILTER. */
    SIWIFI_TM_ATTR_REG_FILTER,

    SIWIFI_TM_ATTR_MAX,
};

/***********************************************************************/
int siwifi_testmode_reg(struct ieee80211_hw *hw, struct nlattr **tb);
int siwifi_testmode_dbg_filter(struct ieee80211_hw *hw, struct nlattr **tb);
int siwifi_testmode_reg_dbg(struct ieee80211_hw *hw, struct nlattr **tb);

#endif /* SIWIFI_TESTMODE_H_ */
