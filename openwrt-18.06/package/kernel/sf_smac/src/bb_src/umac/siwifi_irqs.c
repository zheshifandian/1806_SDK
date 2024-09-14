/**
 ******************************************************************************
 *
 * @file siwifi_irqs.c
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */
#include <linux/interrupt.h>

#include "siwifi_defs.h"
#include "ipc_host.h"
#include "siwifi_prof.h"

/**
 * siwifi_irq_hdlr - IRQ handler
 *
 * Handler registerd by the platform driver
 */
irqreturn_t siwifi_irq_hdlr(int irq, void *dev_id)
{
    struct siwifi_hw *siwifi_hw = (struct siwifi_hw *)dev_id;
    disable_irq_nosync(irq);
    tasklet_schedule(&siwifi_hw->task);
    return IRQ_HANDLED;
}

/**
 * siwifi_task - Bottom half for IRQ handler
 *
 * Read irq status and process accordingly
 */
void siwifi_task(unsigned long data)
{
    struct siwifi_hw *siwifi_hw = (struct siwifi_hw *)data;
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
    u32 status, statuses = 0;
    unsigned long now = jiffies;

    REG_SW_SET_PROFILING(siwifi_hw, SW_PROF_SIWIFI_IPC_IRQ_HDLR);

    /* Ack unconditionnally in case ipc_host_get_status does not see the irq */
    siwifi_plat->ack_irq(siwifi_plat);

    while ((status = ipc_host_get_status(siwifi_hw->ipc_env))) {
        statuses |= status;
        /* All kinds of IRQs will be handled in one shot (RX, MSG, DBG, ...)
         * this will ack IPC irqs not the cfpga irqs */
        ipc_host_irq(siwifi_hw->ipc_env, status);

        siwifi_plat->ack_irq(siwifi_plat);
    }

#if defined (CONFIG_SIWIFI_DEBUGFS) || defined (CONFIG_SIWIFI_PROCFS)
    if (statuses & IPC_IRQ_E2A_RXDESC)
        siwifi_hw->stats.last_rx = now;
    if (statuses & IPC_IRQ_E2A_TXCFM)
        siwifi_hw->stats.last_tx = now;
#endif

    spin_lock(&siwifi_hw->tx_lock);
    siwifi_hwq_process_all(siwifi_hw);
    spin_unlock(&siwifi_hw->tx_lock);

    enable_irq(siwifi_platform_get_irq(siwifi_plat));

    REG_SW_CLEAR_PROFILING(siwifi_hw, SW_PROF_SIWIFI_IPC_IRQ_HDLR);
}
