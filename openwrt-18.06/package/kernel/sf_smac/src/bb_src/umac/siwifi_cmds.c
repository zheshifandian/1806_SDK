/**
 ******************************************************************************
 *
 * siwifi_cmds.c
 *
 * Handles queueing (push to IPC, ack/cfm from IPC) of commands issued to
 * LMAC FW
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#include <linux/list.h>

#include "siwifi_cmds.h"
#include "siwifi_defs.h"
#include "siwifi_strs.h"
#define CREATE_TRACE_POINTS
#include "siwifi_events.h"

#if defined (CONFIG_SIWIFI_ACS) || defined (CONFIG_SIWIFI_ACS_INTERNAL)
#include "siwifi_acs.h"
#endif

#include "ipc_host.h"
#include "reg_ipc_app.h"
#include "reg_ipc_emb.h"
#include "siwifi_mem.h"

/**
 *
 */
static void cmd_dump(const struct siwifi_cmd *cmd)
{
    printk(KERN_CRIT "tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
           cmd->tkn, cmd->flags, cmd->result, cmd->id, SIWIFI_ID2STR(cmd->id),
           cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? SIWIFI_ID2STR(cmd->reqid) : "none");
}

/**
 * this must be called under the spin_lock_bh(&cmd_mgr->lock);
 */
static void cmd_gmr_dump(struct siwifi_cmd_mgr *cmd_mgr)
{
    struct siwifi_cmd *cur;

    SIWIFI_DBG("q_sz/max: %2d / %2d - next tkn: %d\n",
             cmd_mgr->queue_sz, cmd_mgr->max_queue_sz,
             cmd_mgr->next_tkn);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        cmd_dump(cur);
    }
}

/**
 *
 */
static void cmd_complete(struct siwifi_cmd_mgr *cmd_mgr, struct siwifi_cmd *cmd)
{
    lockdep_assert_held(&cmd_mgr->lock);

    list_del(&cmd->list);
    cmd_mgr->queue_sz--;

    cmd->flags |= SIWIFI_CMD_FLAG_DONE;
    if (cmd->flags & SIWIFI_CMD_FLAG_NONBLOCK) {
        siwifi_kfree(cmd);
    } else {
        if (SIWIFI_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd->result = 0;
            complete(&cmd->complete);
        }
    }
}

#ifdef CONFIG_ERROR_DUMP
static void cmd_err_msg_save(struct siwifi_hw *siwifi_hw,
		struct siwifi_cmd *cmd, int type)
{
	char msg[ERROR_BUF_MAX_SIZE];
	int result = type;
	if (result == 0)
		result = cmd->result;
	switch(result){
		case -EPERM:
								sprintf(msg,"cmd queue[%d] deinit",siwifi_hw->mod_params->is_hb);
								break;
		case -EPIPE:			sprintf(msg,"cmd queue[%d] crashed",siwifi_hw->mod_params->is_hb);
								break;
		case -ENOMEM:			sprintf(msg,"Too many cmds (%d) already queued",siwifi_hw->mod_params->is_hb);
								break;
		case -ETIMEDOUT:		sprintf(msg,"wait cmd timeout:tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
										cmd->tkn, cmd->flags, cmd->result, cmd->id, SIWIFI_ID2STR(cmd->id),
										cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? SIWIFI_ID2STR(cmd->reqid) : "none");
								break;
		default:					printk("%s:error result\n",__func__);
								return;
	}
	siwifi_save_error_info(siwifi_hw, msg, strlen(msg));
}
#endif

#ifdef CONFIG_SEND_ERR
extern int ker_err_send(char *type, int module, int code, char *text, char *path, int flag);
#endif

/**
 *
 */
static int cmd_mgr_queue(struct siwifi_cmd_mgr *cmd_mgr, struct siwifi_cmd *cmd)
{
    struct siwifi_hw *siwifi_hw = container_of(cmd_mgr, struct siwifi_hw, cmd_mgr);
#ifdef CONFIG_SIWIFI_RF_RECALI
    struct siwifi_cmd *__last;
#endif
    unsigned long tout = 0;
    bool defer_push = false;
    long wait_ret = 0;
#ifdef CONFIG_SEND_ERR
	int err = 0;
#endif

    //SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);
    trace_msg_send(cmd->id);

	/*
	 * when lmac is scaning,if send some cmd(apm_start,apm_stop,...) to lmac,lmac will error
	 * have cancel scan in softmac,but no cancel scan in fullmac
	 * now we do not know when and how to cancel scan
	 * */

	if(siwifi_hw->scaning)
	{
		if (!(cmd->flags & SIWIFI_CMD_FLAG_NONBLOCK)) {
			tout = msecs_to_jiffies(siwifi_hw->scan_timeout);
			if (!wait_for_completion_killable_timeout(&siwifi_hw->scan_complete, tout)) {
				printk("wait scan timeout, tout : %d(ms)\n", jiffies_to_msecs(tout));
                // RM#12017 If an assert occurs during scanning, a cfg80211_scan_done message should be sent.
                // Otherwise, the upper layer thinks that the resource is busy
                if (siwifi_hw->scan_request) {
                    struct cfg80211_scan_info info = {
                        .aborted = true,
                    };
                    cfg80211_scan_done(siwifi_hw->scan_request, &info);
                    siwifi_hw->scan_request = NULL;
                }
                if(siwifi_hw->scaning){
                    complete_all(&siwifi_hw->scan_complete);
                    siwifi_hw->scaning = false;
                }

#if defined (CONFIG_SIWIFI_ACS) || defined (CONFIG_SIWIFI_ACS_INTERNAL)
                siwifi_acs_scan_abord(siwifi_hw, true);
#endif
			}
            if (test_bit(SIWIFI_DEV_STACK_RESTARTING, &siwifi_hw->drv_flags))
            {
                printk(KERN_CRIT"drop msg[%d] case restarting\n", siwifi_hw->mod_params->is_hb);
                cmd->result = -EPIPE;
                return -EPIPE;
            }
			tout = 0;
		}
	}
    spin_lock_bh(&cmd_mgr->lock);

    if(cmd_mgr->state == SIWIFI_CMD_MGR_STATE_DEINIT) {
        printk(KERN_CRIT"cmd queue[%d] deinit\n", siwifi_hw->mod_params->is_hb);
        cmd->result = -EPERM;
        spin_unlock_bh(&cmd_mgr->lock);
#ifdef CONFIG_ERROR_DUMP
		cmd_err_msg_save(siwifi_hw,cmd,0);
#endif
        return -EPIPE;
    }else if (cmd_mgr->state == SIWIFI_CMD_MGR_STATE_CRASHED) {
        printk(KERN_CRIT"cmd queue[%d] crashed\n", siwifi_hw->mod_params->is_hb);
        cmd->result = -EPIPE;
        spin_unlock_bh(&cmd_mgr->lock);
#ifdef CONFIG_ERROR_DUMP
		cmd_err_msg_save(siwifi_hw,cmd,0);
#endif
        return -EPIPE;
    }

    if (!list_empty(&cmd_mgr->cmds)) {
        struct siwifi_cmd *last;

        if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz) {
            cmd_gmr_dump(cmd_mgr);
            spin_unlock_bh(&cmd_mgr->lock);
            printk(KERN_CRIT"Too many cmds (%d) already queued\n",
                   cmd_mgr->max_queue_sz);
            cmd->result = -ENOMEM;
#ifdef CONFIG_ERROR_DUMP
			cmd_err_msg_save(siwifi_hw,cmd,0);
#endif
            return -ENOMEM;
        }
#ifdef CONFIG_SIWIFI_RF_RECALI
        //FIXME
        //do not support continuous FORCE_PUSH
        //Now, we do not have this problems
        last = NULL;
        list_for_each_entry_reverse(__last, &cmd_mgr->cmds, list){
            if(!(__last->flags & SIWIFI_CMD_FLAG_FORCE_PUSH) || (__last->flags & SIWIFI_CMD_FLAG_WAIT_ACK)){
                last = __last;
                break;
            }
        }
#else
        last = list_entry(cmd_mgr->cmds.prev, struct siwifi_cmd, list);
#endif
        if (last && (last->flags & (SIWIFI_CMD_FLAG_WAIT_ACK | SIWIFI_CMD_FLAG_WAIT_PUSH))) {
            cmd->flags |= SIWIFI_CMD_FLAG_WAIT_PUSH;
            defer_push = true;
        }
    }
#ifdef CONFIG_SIWIFI_RF_RECALI
    //if the command queue has been entered into paused state.
    //Just defer all the followed command until it received
    if(cmd->flags & SIWIFI_CMD_FLAG_FORCE_PUSH){
        WARN_ON(!(cmd_mgr->state == SIWIFI_CMD_MGR_STATE_PAUSED));
        cmd->flags &= ~SIWIFI_CMD_FLAG_WAIT_PUSH;
        defer_push = false;
        cmd_gmr_dump(cmd_mgr);
    }
    else if(cmd_mgr->state == SIWIFI_CMD_MGR_STATE_PAUSED) {
        printk("SIWIFI_CMD_MGR_STATE_PAUSED(%d), so defer this cmd, cmd->id : %d\n",siwifi_hw->mod_params->is_hb,cmd->id);
        cmd->flags |= SIWIFI_CMD_FLAG_WAIT_PUSH;
        defer_push = true;
        //increase 1s if the state is paused
        tout = msecs_to_jiffies(1000);
    }
#endif

    cmd->flags |= SIWIFI_CMD_FLAG_WAIT_ACK;
    if (cmd->flags & SIWIFI_CMD_FLAG_REQ_CFM)
        cmd->flags |= SIWIFI_CMD_FLAG_WAIT_CFM;

    cmd->tkn    = cmd_mgr->next_tkn++;
    cmd->result = -EINTR;

    if (!(cmd->flags & SIWIFI_CMD_FLAG_NONBLOCK))
        init_completion(&cmd->complete);

    list_add_tail(&cmd->list, &cmd_mgr->cmds);
    cmd_mgr->queue_sz++;

    if (!defer_push) {
        siwifi_ipc_msg_push(siwifi_hw, cmd, SIWIFI_CMD_A2EMSG_LEN(cmd->a2e_msg));
        siwifi_kfree(cmd->a2e_msg);
        cmd->a2e_msg = NULL;
    }

    if (!(cmd->flags & SIWIFI_CMD_FLAG_NONBLOCK)) {
        spin_unlock_bh(&cmd_mgr->lock);

        tout += msecs_to_jiffies(SIWIFI_80211_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);

        wait_ret = wait_for_completion_killable_timeout(&cmd->complete, tout);

        spin_lock_bh(&cmd_mgr->lock);
        //interrupted by a kill signal
        if(wait_ret == -ERESTARTSYS && !(cmd->flags & SIWIFI_CMD_FLAG_DONE)){
            //RM#11016 remove from cmd_mgmr list if wait was interrupted by signal
            if(cmd->flags & SIWIFI_CMD_FLAG_WAIT_PUSH)
            {
                list_del(&cmd->list);
                cmd_mgr->queue_sz--;
                cmd->flags &= ~SIWIFI_CMD_FLAG_WAIT_PUSH;
            }else{
                //RM#8955 don't free cmd it not wait complete yet
                //if the wait is interrupted and cmd not done, we make it noblock message, so it will not be freed after return
                //and it will be freed when cmd_complete
                cmd->flags |= SIWIFI_CMD_FLAG_NONBLOCK;
            }
        }
        spin_unlock_bh(&cmd_mgr->lock);

        if (!wait_ret) {
            printk("wait cmd timeout, tout : %d(ms), defer_push : %d!\n", jiffies_to_msecs(tout), defer_push);
            cmd_dump(cmd);
            cmd_gmr_dump(cmd_mgr);
            //for debug cmd queue crash registers
#if 1
            {
                if(!siwifi_hw->mod_params->is_hb) {
                    printk("[lb]gic mask : 0x%x, pending : 0x%x\n", readl((void *)0xBBDC0404), readl((void *)0xBBDC0484));
                } else {
                    printk("[hb]gic mask : 0x%x, pending : 0x%x\n", readl((void *)0xBBDC0408), readl((void *)0xBBDC0488));
                }

                printk("ipc_env->reg_base : %p\n", siwifi_hw->ipc_env->reg_base);
				printk("a2e status : 0x%x, raw staus : 0x%x, mask : 0x%x, trigger(e2a) : 0x%x\n",
                        ipc_app2emb_status_get(siwifi_hw), ipc_app2emb_rawstatus_get(siwifi_hw),    \
                        ipc_app2emb_unmask_get(siwifi_hw), ipc_emb2app_trigger_get(siwifi_hw));
                printk("[by API]e2a status : 0x%x, raw staus : 0x%x, mask : 0x%x, trigger(a2e) : 0x%x\n",
                        ipc_emb2app_status_get(siwifi_hw->ipc_env->reg_base), ipc_emb2app_rawstatus_get(siwifi_hw->ipc_env->reg_base),    \
                        ipc_emb2app_unmask_get(siwifi_hw->ipc_env->reg_base), ipc_app2emb_trigger_get(siwifi_hw->ipc_env->reg_base));
            }
#ifdef CONFIG_SEND_ERR
			//report to log server
			err = ker_err_send("WARNING", 5, 100, "CMD_CRASH", "klog/system log", 0);
			printk("ker_err_send(%d) ret1=%d\n",siwifi_hw->mod_params->is_hb,err);
#endif
#endif
            spin_lock_bh(&cmd_mgr->lock);
            cmd_mgr->state = SIWIFI_CMD_MGR_STATE_CRASHED;
            if (!(cmd->flags & SIWIFI_CMD_FLAG_DONE)) {
                cmd->result = -ETIMEDOUT;
                cmd_complete(cmd_mgr, cmd);
            }
            spin_unlock_bh(&cmd_mgr->lock);
#ifdef CONFIG_ERROR_DUMP
			cmd_err_msg_save(siwifi_hw,cmd,-ETIMEDOUT);
#endif
            return -ETIMEDOUT;
        }
    } else {
        cmd->result = 0;
        spin_unlock_bh(&cmd_mgr->lock);
    }

    return 0;
}

/**
 *
 */
static int cmd_mgr_llind(struct siwifi_cmd_mgr *cmd_mgr, struct siwifi_cmd *cmd)
{
    struct siwifi_hw *siwifi_hw = container_of(cmd_mgr, struct siwifi_hw, cmd_mgr);
    struct siwifi_cmd *cur, *acked = NULL, *next = NULL;

    //SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);

    spin_lock(&cmd_mgr->lock);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        if (!acked) {
            if (cur->tkn == cmd->tkn) {
                if (WARN_ON_ONCE(cur != cmd)) {
                    cmd_dump(cmd);
                }
                acked = cur;
                continue;
            }
        }
#ifdef CONFIG_SIWIFI_RF_RECALI
        //if we support rf recalibration, the command queue could be like this
        // cmd1(wait_psuh) cmd2(wait_push) cmd3(force_push)
        // so cmd3 could be responsed before cmd1/2
        if (cur->flags & SIWIFI_CMD_FLAG_WAIT_PUSH) {
            if(!next){
                next = cur;
                continue;
            }
        }
#else
        if (cur->flags & SIWIFI_CMD_FLAG_WAIT_PUSH) {
            next = cur;
            break;
        }
#endif
    }
    if (!acked) {
        printk(KERN_CRIT "Error: acked cmd not found\n");
    } else {
        //case cmd maybe freed in cmd_complete,so we store the value first
        u16 cmd_flags = cmd->flags;
        cmd->flags &= ~SIWIFI_CMD_FLAG_WAIT_ACK;
        if (SIWIFI_CMD_WAIT_COMPLETE(cmd->flags))
            cmd_complete(cmd_mgr, cmd);
#ifdef CONFIG_SIWIFI_RF_RECALI
        if(cmd_flags & SIWIFI_CMD_FLAG_ASK_PAUSE) {
            printk("ask cmd queue pause(%d)!\n",siwifi_hw->mod_params->is_hb);
            cmd_mgr->state = SIWIFI_CMD_MGR_STATE_PAUSED;
        }else if(cmd_flags & SIWIFI_CMD_FLAG_ASK_RESUME) {
            printk("ask cmd queue resume(%d)!\n", siwifi_hw->mod_params->is_hb);
            WARN_ON(!(cmd_flags & SIWIFI_CMD_FLAG_FORCE_PUSH));
            cmd_mgr->state = SIWIFI_CMD_MGR_STATE_INITED;
        }
#endif
    }
    if (next
#ifdef CONFIG_SIWIFI_RF_RECALI
            && (cmd_mgr->state != SIWIFI_CMD_MGR_STATE_PAUSED)
#endif
            && (next->a2e_msg)
            ) {
        struct siwifi_hw *siwifi_hw = container_of(cmd_mgr, struct siwifi_hw, cmd_mgr);
        next->flags &= ~SIWIFI_CMD_FLAG_WAIT_PUSH;
        siwifi_ipc_msg_push(siwifi_hw, next, SIWIFI_CMD_A2EMSG_LEN(next->a2e_msg));
        siwifi_kfree(next->a2e_msg);
        next->a2e_msg = NULL;
    }
    spin_unlock(&cmd_mgr->lock);

    return 0;
}



static int cmd_mgr_run_callback(struct siwifi_hw *siwifi_hw, struct siwifi_cmd *cmd,
                                struct siwifi_cmd_e2amsg *msg, msg_cb_fct cb)
{
    int res;

    if (! cb)
        return 0;

    spin_lock(&siwifi_hw->cb_lock);
    res = cb(siwifi_hw, cmd, msg);
    spin_unlock(&siwifi_hw->cb_lock);

    return res;
}

/**
 *

 */
static int cmd_mgr_msgind(struct siwifi_cmd_mgr *cmd_mgr, struct siwifi_cmd_e2amsg *msg,
                          msg_cb_fct cb)
{
    struct siwifi_hw *siwifi_hw = container_of(cmd_mgr, struct siwifi_hw, cmd_mgr);
    struct siwifi_cmd *cmd, *next;
    bool found = false;
    bool ret = 0;

    trace_msg_recv(msg->id);

    //the cmd_mgr lock should not protect the cb content. It should only protect the command queue.
    //FIXME:
    //1, But this changes assume that all the cb function implemented by user or in siwifi_msg_rx.c do not show interested in
    //MSG_XXX_CFM.
    //2, this function now is not suitable for fmac(START_CFM)
    spin_lock(&cmd_mgr->lock);
    list_for_each_entry_safe(cmd, next, &cmd_mgr->cmds, list) {
        if (cmd->reqid == msg->id &&
            (cmd->flags & SIWIFI_CMD_FLAG_WAIT_CFM)) {
            WARN_ON(cb);
            found = true;
            cmd->flags &= ~SIWIFI_CMD_FLAG_WAIT_CFM;

            if (WARN((msg->param_len > SIWIFI_CMD_E2AMSG_LEN_MAX),
                     "Unexpect E2A msg len %d > %d\n", msg->param_len,
                     SIWIFI_CMD_E2AMSG_LEN_MAX)) {
                msg->param_len = SIWIFI_CMD_E2AMSG_LEN_MAX;
            }

            if (cmd->e2a_msg && msg->param_len)
                memcpy(cmd->e2a_msg, &msg->param, msg->param_len);

            if (SIWIFI_CMD_WAIT_COMPLETE(cmd->flags))
                cmd_complete(cmd_mgr, cmd);

            break;
        }
    }
    spin_unlock(&cmd_mgr->lock);

    if (!found && cb)
        ret = cb(siwifi_hw, NULL, msg);

    if(ret || (!found && !cb))
        printk("a msg missed by umac, msg->id : %d\n", msg->id);

    return 0;
}

/**
 *
 */
static void cmd_mgr_print(struct siwifi_cmd_mgr *cmd_mgr)
{
    struct siwifi_cmd *cur;

    spin_lock_bh(&cmd_mgr->lock);
    SIWIFI_DBG("q_sz/max: %2d / %2d - next tkn: %d\n",
             cmd_mgr->queue_sz, cmd_mgr->max_queue_sz,
             cmd_mgr->next_tkn);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        cmd_dump(cur);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

/**
 *
 */
static void cmd_mgr_drain(struct siwifi_cmd_mgr *cmd_mgr)
{
    struct siwifi_cmd *cur, *nxt;

    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);

    spin_lock_bh(&cmd_mgr->lock);
    list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
        list_del(&cur->list);
        cmd_mgr->queue_sz--;
        if (cur->a2e_msg) {
            siwifi_kfree(cur->a2e_msg);
            cur->a2e_msg = NULL;
        }
        if (!(cur->flags & SIWIFI_CMD_FLAG_NONBLOCK))
            complete(&cur->complete);
        else
            siwifi_kfree(cur);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

/**
 *
 */
void siwifi_cmd_mgr_init(struct siwifi_cmd_mgr *cmd_mgr)
{
    SIWIFI_DBG(SIWIFI_FN_ENTRY_STR);

    INIT_LIST_HEAD(&cmd_mgr->cmds);
    spin_lock_init(&cmd_mgr->lock);
    cmd_mgr->max_queue_sz = SIWIFI_CMD_MAX_QUEUED;
    cmd_mgr->queue  = &cmd_mgr_queue;
    cmd_mgr->print  = &cmd_mgr_print;
    cmd_mgr->drain  = &cmd_mgr_drain;
    cmd_mgr->llind  = &cmd_mgr_llind;
    cmd_mgr->msgind = &cmd_mgr_msgind;
    cmd_mgr->state = SIWIFI_CMD_MGR_STATE_INITED;
}

/**
 *
 */
void siwifi_cmd_mgr_deinit(struct siwifi_cmd_mgr *cmd_mgr)
{
    spin_lock_bh(&cmd_mgr->lock);
    cmd_mgr->state = SIWIFI_CMD_MGR_STATE_DEINIT;
    spin_unlock_bh(&cmd_mgr->lock);
    cmd_mgr->print(cmd_mgr);
    cmd_mgr->drain(cmd_mgr);
    cmd_mgr->print(cmd_mgr);
    memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}
