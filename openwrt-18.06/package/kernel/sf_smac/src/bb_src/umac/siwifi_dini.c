/**
 ******************************************************************************
 *
 * @file siwifi_dini.c - Add support for dini platform
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#include "siwifi_dini.h"
#include "siwifi_defs.h"
#include "siwifi_irqs.h"
#include "reg_access.h"

/* Config FPGA is accessed via bar0 */
#define CFPGA_DMA0_CTRL_REG             0x02C
#define CFPGA_DMA1_CTRL_REG             0x04C
#define CFPGA_DMA2_CTRL_REG             0x06C
#define CFPGA_UINTR_SRC_REG             0x0E8
#define CFPGA_UINTR_MASK_REG            0x0EC
#define CFPGA_BAR4_HIADDR_REG           0x100
#define CFPGA_BAR4_LOADDR_REG           0x104
#define CFPGA_BAR4_LOADDR_MASK_REG      0x110
#define CFPGA_BAR_TOUT                  0x120

#define CFPGA_DMA_CTRL_ENABLE           0x00001400
#define CFPGA_DMA_CTRL_DISABLE          0x00001000
#define CFPGA_DMA_CTRL_CLEAR            0x00001800
#define CFPGA_DMA_CTRL_REREAD_TIME_MASK (BIT(10) - 1)

#define CFPGA_BAR4_LOADDR_MASK_MAX      0xFF000000

#define CFPGA_PCIEX_IT                  0x00000001
#define CFPGA_ALL_ITS                   0x0000000F

/* Programmable BAR4 Window start address */
#define CPU_RAM_WINDOW_HIGH      0x00000000
#define CPU_RAM_WINDOW_LOW       0x00000000
#define AHB_BRIDGE_WINDOW_HIGH   0x00000000
#define AHB_BRIDGE_WINDOW_LOW    0x60000000

struct siwifi_dini
{
    u8 *pci_bar0_vaddr;
    u8 *pci_bar4_vaddr;
};

static const u32 mv_cfg_fpga_dma_ctrl_regs[] = {
    CFPGA_DMA0_CTRL_REG,
    CFPGA_DMA1_CTRL_REG,
    CFPGA_DMA2_CTRL_REG
};

/* This also clears running transactions */
static void dini_dma_on(struct siwifi_dini *siwifi_dini)
{
    int i;
    u32 reread_time;
    volatile void *reg;

    for (i = 0; i < ARRAY_SIZE(mv_cfg_fpga_dma_ctrl_regs); i++) {
        reg = siwifi_dini->pci_bar0_vaddr + mv_cfg_fpga_dma_ctrl_regs[i];
        reread_time = readl(reg) & CFPGA_DMA_CTRL_REREAD_TIME_MASK;

        writel(CFPGA_DMA_CTRL_CLEAR  | reread_time, reg);
        writel(CFPGA_DMA_CTRL_ENABLE | reread_time, reg);
    }
}

/* This also clears running transactions */
static void dini_dma_off(struct siwifi_dini *siwifi_dini)
{
    int i;
    u32 reread_time;
    volatile void *reg;

    for (i = 0; i < ARRAY_SIZE(mv_cfg_fpga_dma_ctrl_regs); i++) {
        reg = siwifi_dini->pci_bar0_vaddr + mv_cfg_fpga_dma_ctrl_regs[i];
        reread_time = readl(reg) & CFPGA_DMA_CTRL_REREAD_TIME_MASK;

        writel(CFPGA_DMA_CTRL_DISABLE | reread_time, reg);
        writel(CFPGA_DMA_CTRL_CLEAR   | reread_time, reg);
    }
}


/* Configure address range for BAR4.
 * By default BAR4_LOADDR_MASK value is 0xFF000000, then there is no need to
 * change it because the addresses we need to access are covered by this mask
 */
static void dini_set_bar4_win(u32 low, u32 high, struct siwifi_dini *siwifi_dini)
{
    writel(low, siwifi_dini->pci_bar0_vaddr + CFPGA_BAR4_LOADDR_REG);
    writel(high, siwifi_dini->pci_bar0_vaddr + CFPGA_BAR4_HIADDR_REG);
    writel(CFPGA_BAR4_LOADDR_MASK_MAX,
           siwifi_dini->pci_bar0_vaddr + CFPGA_BAR4_LOADDR_MASK_REG);
}


/**
 * Enable User Interrupts of CFPGA that trigger PCIe IRQs on PCIE_10
 * and request the corresponding IRQ line
 */
int siwifi_cfpga_irq_enable(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;
    unsigned int cfpga_uintr_mask;
    volatile void *reg;
    int ret;

    /* sched_setscheduler on ONESHOT threaded irq handler for BCNs ? */
    if ((ret = request_irq(siwifi_hw->plat->pci_dev->irq, siwifi_irq_hdlr, 0,
                           "siwifi", siwifi_hw)))
            return ret;

    reg = siwifi_dini->pci_bar0_vaddr + CFPGA_UINTR_MASK_REG;
    cfpga_uintr_mask = readl(reg);
    writel(cfpga_uintr_mask | CFPGA_PCIEX_IT, reg);

    return ret;
}

/**
 * Disable User Interrupts of CFPGA that trigger PCIe IRQs on PCIE_10
 * and free the corresponding IRQ line
 */
int siwifi_cfpga_irq_disable(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;
    unsigned int cfpga_uintr_mask;
    volatile void *reg;

    reg = siwifi_dini->pci_bar0_vaddr + CFPGA_UINTR_MASK_REG;
    cfpga_uintr_mask = readl(reg);
    writel(cfpga_uintr_mask & ~CFPGA_PCIEX_IT, reg);

    free_irq(siwifi_hw->plat->pci_dev->irq, siwifi_hw);

    return 0;
}

static int siwifi_dini_platform_enable(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;

#ifdef CONFIG_SIWIFI_SDM
    writel(0x0000FFFF, siwifi_dini->pci_bar0_vaddr + CFPGA_BAR_TOUT);
#endif

    dini_dma_on(siwifi_dini);
    return siwifi_cfpga_irq_enable(siwifi_hw);
}

static int siwifi_dini_platform_disable(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_plat *siwifi_plat = siwifi_hw->plat;
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;
    int ret;

    ret = siwifi_cfpga_irq_disable(siwifi_hw);
    dini_dma_off(siwifi_dini);
    return ret;
}

static void siwifi_dini_platform_deinit(struct siwifi_plat *siwifi_plat)
{
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;

    pci_disable_device(siwifi_plat->pci_dev);
    iounmap(siwifi_dini->pci_bar0_vaddr);
    iounmap(siwifi_dini->pci_bar4_vaddr);
    pci_release_regions(siwifi_plat->pci_dev);

    siwifi_kfree(siwifi_plat);
}

static u8* siwifi_dini_get_address(struct siwifi_plat *siwifi_plat, int addr_name,
                                 unsigned int offset)
{
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;

    if (WARN(addr_name >= SIWIFI_ADDR_MAX, "Invalid address %d", addr_name))
        return NULL;

    if (addr_name == SIWIFI_ADDR_CPU)
        dini_set_bar4_win(CPU_RAM_WINDOW_LOW, CPU_RAM_WINDOW_HIGH, siwifi_dini);
    else
        dini_set_bar4_win(AHB_BRIDGE_WINDOW_LOW, AHB_BRIDGE_WINDOW_HIGH, siwifi_dini);

    return siwifi_dini->pci_bar4_vaddr + offset;
}

static void siwifi_dini_ack_irq(struct siwifi_plat *siwifi_plat)
{
    struct siwifi_dini *siwifi_dini = (struct siwifi_dini *)siwifi_plat->priv;

    writel(CFPGA_ALL_ITS, siwifi_dini->pci_bar0_vaddr + CFPGA_UINTR_SRC_REG);
}

static const u32 siwifi_dini_config_reg[] = {
    NXMAC_DEBUG_PORT_SEL_ADDR,
    SYSCTRL_DIAG_CONF_ADDR,
    RF_V6_DIAGPORT_CONF1_ADDR,
    RF_v6_PHYDIAG_CONF1_ADDR,
};

static int siwifi_dini_get_config_reg(const u32 **list)
{
    if (!list)
        return 0;

    *list = siwifi_dini_config_reg;
    return ARRAY_SIZE(siwifi_dini_config_reg);
}

/**
 * siwifi_dini_platform_init - Initialize the DINI platform
 *
 * @pci_dev PCI device
 * @siwifi_plat Pointer on struct siwifi_stat * to be populated
 *
 * @return 0 on success, < 0 otherwise
 *
 * Allocate and initialize a siwifi_plat structure for the dini platform.
 */
int siwifi_dini_platform_init(struct pci_dev *pci_dev, struct siwifi_plat **siwifi_plat)
{
    struct siwifi_dini *siwifi_dini;
    u16 pci_cmd;
    int ret = 0;

    *siwifi_plat = siwifi_kzalloc(sizeof(struct siwifi_plat) + sizeof(struct siwifi_dini),
                        GFP_KERNEL);
    if (!*siwifi_plat)
        return -ENOMEM;

    siwifi_dini = (struct siwifi_dini *)(*siwifi_plat)->priv;

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

    if (!(siwifi_dini->pci_bar0_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 0))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 0);
        ret = -ENOMEM;
        goto out_bar0;
    }
    if (!(siwifi_dini->pci_bar4_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 4))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 4);
        ret = -ENOMEM;
        goto out_bar4;
    }

    (*siwifi_plat)->enable = siwifi_dini_platform_enable;
    (*siwifi_plat)->disable = siwifi_dini_platform_disable;
    (*siwifi_plat)->deinit = siwifi_dini_platform_deinit;
    (*siwifi_plat)->get_address = siwifi_dini_get_address;
    (*siwifi_plat)->ack_irq = siwifi_dini_ack_irq;
    (*siwifi_plat)->get_config_reg = siwifi_dini_get_config_reg;

#ifdef CONFIG_SIWIFI_SDM
    writel(0x0000FFFF, siwifi_dini->pci_bar0_vaddr + CFPGA_BAR_TOUT);
#endif

    return 0;

  out_bar4:
    iounmap(siwifi_dini->pci_bar0_vaddr);
  out_bar0:
    pci_release_regions(pci_dev);
  out_request:
    pci_disable_device(pci_dev);
  out_enable:
    siwifi_kfree(*siwifi_plat);
    return ret;
}
