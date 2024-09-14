/**
 ******************************************************************************
 *
 * siwifi_cmds.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_CMDS_H_
#define _SIWIFI_CMDS_H_

#include <linux/spinlock.h>
#include <linux/completion.h>
#include "lmac_msg.h"

//the whole debug message dump will took us sometimes,so increase the cmd timeout
#if (defined(CONFIG_SIWIFI_SDM) || defined(CFG_DBGDUMP))
#define SIWIFI_80211_CMD_TIMEOUT_MS    (20 * 300)
#else
#define SIWIFI_80211_CMD_TIMEOUT_MS    (20 * 300)
#endif

#define SIWIFI_CMD_FLAG_NONBLOCK      BIT(0)
#define SIWIFI_CMD_FLAG_REQ_CFM       BIT(1)
#define SIWIFI_CMD_FLAG_WAIT_PUSH     BIT(2)
#define SIWIFI_CMD_FLAG_WAIT_ACK      BIT(3)
#define SIWIFI_CMD_FLAG_WAIT_CFM      BIT(4)
#define SIWIFI_CMD_FLAG_DONE          BIT(5)
#ifdef CONFIG_SIWIFI_RF_RECALI
//this commmand is pushed by force
#define SIWIFI_CMD_FLAG_FORCE_PUSH    BIT(6)
//after this command is doned, the whole command queue manager is asked to be paused
#define SIWIFI_CMD_FLAG_ASK_PAUSE     BIT(7)
#define SIWIFI_CMD_FLAG_ASK_RESUME    BIT(8)
#endif
/* ATM IPC design makes it possible to get the CFM before the ACK,
 * otherwise this could have simply been a state enum */
#define SIWIFI_CMD_WAIT_COMPLETE(flags) \
    (!(flags & (SIWIFI_CMD_FLAG_WAIT_ACK | SIWIFI_CMD_FLAG_WAIT_CFM)))

//Note that modify MM_TIM_UPDATE_REQ to 40 for the possibility of 40 burst request MM_TIM_UPDATE_REQ
//which is non blocking command and may accumulated in our cmd queue. when accumulated num > SIWIFI_CMD_MAX_QUEUED,
//it will discard the MM_TIM_UPDATE_REQ request command, that's not we expect.
#define SIWIFI_CMD_MAX_QUEUED         40

#include "ipc_shared.h"
#define siwifi_cmd_e2amsg ipc_e2a_msg
#define siwifi_cmd_a2emsg lmac_msg
#define SIWIFI_CMD_A2EMSG_LEN(m) (sizeof(struct lmac_msg) + m->param_len)
#define SIWIFI_CMD_E2AMSG_LEN_MAX (IPC_E2A_MSG_PARAM_SIZE * 4)

struct siwifi_hw;
struct siwifi_cmd;
typedef int (*msg_cb_fct)(struct siwifi_hw *siwifi_hw, struct siwifi_cmd *cmd,
                          struct siwifi_cmd_e2amsg *msg);

enum siwifi_cmd_mgr_state {
    SIWIFI_CMD_MGR_STATE_DEINIT,
    SIWIFI_CMD_MGR_STATE_INITED,
#ifdef CONFIG_SIWIFI_RF_RECALI
    SIWIFI_CMD_MGR_STATE_PAUSED,
#endif
    SIWIFI_CMD_MGR_STATE_CRASHED,
};

struct siwifi_cmd {
    struct list_head list;
    lmac_msg_id_t id;
    lmac_msg_id_t reqid;
    struct siwifi_cmd_a2emsg *a2e_msg;
    char *e2a_msg;
    u32 tkn;
    u16 flags;

    struct completion complete;
    u32 result;
};

struct siwifi_cmd_mgr {
    enum siwifi_cmd_mgr_state state;
    spinlock_t lock;
    u32 next_tkn;
    u32 queue_sz;
    u32 max_queue_sz;

    struct list_head cmds;

    int  (*queue)(struct siwifi_cmd_mgr *, struct siwifi_cmd *);
    int  (*llind)(struct siwifi_cmd_mgr *, struct siwifi_cmd *);
    int  (*msgind)(struct siwifi_cmd_mgr *, struct siwifi_cmd_e2amsg *, msg_cb_fct);
    void (*print)(struct siwifi_cmd_mgr *);
    void (*drain)(struct siwifi_cmd_mgr *);
};

void siwifi_cmd_mgr_init(struct siwifi_cmd_mgr *cmd_mgr);
void siwifi_cmd_mgr_deinit(struct siwifi_cmd_mgr *cmd_mgr);

#endif /* _SIWIFI_CMDS_H_ */
