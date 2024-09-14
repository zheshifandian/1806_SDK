/**
 ******************************************************************************
 *
 * @file siwifi_v7.c - Support for v7 platform
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#include "siwifi_v7.h"
#include "siwifi_defs.h"
#include "siwifi_irqs.h"
#include "reg_access.h"

struct siwifi_v7
{
    u8 *pci_bar0_vaddr;
    u8 *pci_bar1_vaddr;
};

static int siwifi_v7_platform_enable(struct siwifi_hw *siwifi_hw)
{
    int ret;

    /* sched_setscheduler on ONESHOT threaded irq handler for BCNs ? */
    ret = request_irq(siwifi_hw->plat->pci_dev->irq, siwifi_irq_hdlr, 0,
                      "siwifi", siwifi_hw);
    return ret;
}

static int siwifi_v7_platform_disable(struct siwifi_hw *siwifi_hw)
{
    free_irq(siwifi_hw->plat->pci_dev->irq, siwifi_hw);
    return 0;
}

static void siwifi_v7_platform_deinit(struct siwifi_plat *siwifi_plat)
{
    struct siwifi_v7 *siwifi_v7 = (struct siwifi_v7 *)siwifi_plat->priv;

    pci_disable_device(siwifi_plat->pci_dev);
    iounmap(siwifi_v7->pci_bar0_vaddr);
    iounmap(siwifi_v7->pci_bar1_vaddr);
    pci_release_regions(siwifi_plat->pci_dev);
    pci_clear_master(siwifi_plat->pci_dev);
    pci_disable_msi(siwifi_plat->pci_dev);
    siwifi_kfree(siwifi_plat);
}

static u8* siwifi_v7_get_address(struct siwifi_plat *siwifi_plat, int addr_name,
                                 unsigned int offset)
{
    struct siwifi_v7 *siwifi_v7 = (struct siwifi_v7 *)siwifi_plat->priv;

    if (WARN(addr_name >= SIWIFI_ADDR_MAX, "Invalid address %d", addr_name))
        return NULL;

    if (addr_name == SIWIFI_ADDR_CPU)
        return siwifi_v7->pci_bar0_vaddr + offset;
    else
        return siwifi_v7->pci_bar1_vaddr + offset;
}

static void siwifi_v7_ack_irq(struct siwifi_plat *siwifi_plat)
{

}

static const u32 siwifi_v7_config_reg[] = {
    NXMAC_DEBUG_PORT_SEL_ADDR,
    SYSCTRL_DIAG_CONF_ADDR,
    SYSCTRL_PHYDIAG_CONF_ADDR,
    SYSCTRL_RIUDIAG_CONF_ADDR,
    RF_V7_DIAGPORT_CONF1_ADDR,
};

static int siwifi_v7_get_config_reg(const u32 **list)
{
    if (!list)
        return 0;

    *list = siwifi_v7_config_reg;
    return ARRAY_SIZE(siwifi_v7_config_reg);
}


/**
 * siwifi_v7_platform_init - Initialize the DINI platform
 *
 * @pci_dev PCI device
 * @siwifi_plat Pointer on struct siwifi_stat * to be populated
 *
 * @return 0 on success, < 0 otherwise
 *
 * Allocate and initialize a siwifi_plat structure for the dini platform.
 */
int siwifi_v7_platform_init(struct pci_dev *pci_dev, struct siwifi_plat **siwifi_plat)
{
    struct siwifi_v7 *siwifi_v7;
    u16 pci_cmd;
    int ret = 0;

    *siwifi_plat = siwifi_kzalloc(sizeof(struct siwifi_plat) + sizeof(struct siwifi_v7),
                        GFP_KERNEL);
    if (!*siwifi_plat)
        return -ENOMEM;

    siwifi_v7 = (struct siwifi_v7 *)(*siwifi_plat)->priv;

    /* Hotplug fixups */
    pci_read_config_word(pci_dev, PCI_COMMAND, &pci_cmd);
    pci_cmd |= PCI_COMMAND_PARITY | PCI_COMMAND_SERR;
    pci_write_config_word(pci_dev, PCI_COMMAND, pci_cmd);
    pci_write_config_byte(pci_dev, PCI_CACHE_LINE_SIZE, L1_CACHE_BYTES >> 2);

    if ((ret = pci_enable_device(pci_dev))) {
        dev_err(&(pci_dev->dev), "pci_enable_device failed\n");
        goto out_enable;
    }

    pci_set_master(pci_dev);

    if ((ret = pci_request_regions(pci_dev, KBUILD_MODNAME))) {
        dev_err(&(pci_dev->dev), "pci_request_regions failed\n");
        goto out_request;
    }

    if (pci_enable_msi(pci_dev))
    {
        dev_err(&(pci_dev->dev), "pci_enable_msi failed\n");
        goto out_msi;

    }

    if (!(siwifi_v7->pci_bar0_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 0))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 0);
        ret = -ENOMEM;
        goto out_bar0;
    }
    if (!(siwifi_v7->pci_bar1_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 1))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 1);
        ret = -ENOMEM;
        goto out_bar1;
    }

    (*siwifi_plat)->enable = siwifi_v7_platform_enable;
    (*siwifi_plat)->disable = siwifi_v7_platform_disable;
    (*siwifi_plat)->deinit = siwifi_v7_platform_deinit;
    (*siwifi_plat)->get_address = siwifi_v7_get_address;
    (*siwifi_plat)->ack_irq = siwifi_v7_ack_irq;
    (*siwifi_plat)->get_config_reg = siwifi_v7_get_config_reg;

    return 0;

  out_bar1:
    iounmap(siwifi_v7->pci_bar0_vaddr);
  out_bar0:
    pci_disable_msi(pci_dev);
  out_msi:
    pci_release_regions(pci_dev);
  out_request:
    pci_clear_master(pci_dev);
    pci_disable_device(pci_dev);
  out_enable:
    siwifi_kfree(*siwifi_plat);
    return ret;
}
