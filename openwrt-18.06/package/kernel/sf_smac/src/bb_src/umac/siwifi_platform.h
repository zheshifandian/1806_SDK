/**
 ******************************************************************************
 *
 * @file siwifi_platorm.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_PLAT_H_
#define _SIWIFI_PLAT_H_

#include <linux/pci.h>

#define SIWIFI_CONFIG_FW_NAME             "siwifi_settings.ini"
#define SIWIFI_PHY_CONFIG_TRD_NAME        "siwifi_trident.ini"
#define SIWIFI_PHY_CONFIG_KARST_NAME      "siwifi_karst.ini"
#define SIWIFI_PHY_CONFIG_AETNENSIS_NAME  "siwifi_aetnensis.ini"
#define SIWIFI_AGC_FW_NAME                "agcram.bin"
#define SIWIFI_LDPC_RAM_NAME              "ldpcram.bin"
#define SIWIFI_MAC_FW_BASE_NAME           "fmacfw"

#ifdef CONFIG_SIWIFI_TL4
#define SIWIFI_MAC_FW_NAME SIWIFI_MAC_FW_BASE_NAME".hex"
#else
#define SIWIFI_MAC_FW_NAME  SIWIFI_MAC_FW_BASE_NAME".ihex"
#define SIWIFI_MAC_FW_NAME2 SIWIFI_MAC_FW_BASE_NAME".bin"
#endif

#define SIWIFI_FCU_FW_NAME                "fcuram.bin"

/**
 * Type of memory to access (cf siwifi_plat.get_address)
 *
 * @SIWIFI_ADDR_CPU To access memory of the embedded CPU
 * @SIWIFI_ADDR_SYSTEM To access memory/registers of one subsystem of the
 * embedded system
 *
 */
enum siwifi_platform_addr {
    SIWIFI_ADDR_CPU,
    SIWIFI_ADDR_SYSTEM,
    SIWIFI_ADDR_MAX,
};

struct siwifi_hw;

/**
 * struct siwifi_plat - Operation pointers for SIWIFI PCI platform
 *
 * @pci_dev: pointer to pci dev
 * @enabled: Set if embedded platform has been enabled (i.e. fw loaded and
 *          ipc started)
 * @enable: Configure communication with the fw (i.e. configure the transfers
 *         enable and register interrupt)
 * @disable: Stop communication with the fw
 * @deinit: Free all ressources allocated for the embedded platform
 * @get_address: Return the virtual address to access the requested address on
 *              the platform.
 * @ack_irq: Acknowledge the irq at link level.
 * @get_config_reg: Return the list (size + pointer) of registers to restore in
 * order to reload the platform while keeping the current configuration.
 *
 * @priv Private data for the link driver
 */
struct siwifi_plat {
    struct pci_dev *pci_dev;
    bool enabled;

    int (*enable)(struct siwifi_hw *siwifi_hw);
    int (*disable)(struct siwifi_hw *siwifi_hw);
    void (*deinit)(struct siwifi_plat *siwifi_plat);
    u8* (*get_address)(struct siwifi_plat *siwifi_plat, int addr_name,
                       unsigned int offset);
    void (*ack_irq)(struct siwifi_plat *siwifi_plat);
    int (*get_config_reg)(const u32 **list);

    u8 priv[0] __aligned(sizeof(void *));
};

#define SIWIFI_ADDR(plat, base, offset)           \
    plat->get_address(plat, base, offset)

#define SIWIFI_REG_READ(plat, base, offset)               \
    readl(plat->get_address(plat, base, offset))

#define SIWIFI_REG_WRITE(val, plat, base, offset)         \
    writel(val, plat->get_address(plat, base, offset))

int siwifi_platform_init(struct siwifi_plat *siwifi_plat, void **platform_data);
void siwifi_platform_deinit(struct siwifi_hw *siwifi_hw);

int siwifi_platform_on(struct siwifi_hw *siwifi_hw, void *config);
void siwifi_platform_off(struct siwifi_hw *siwifi_hw, void **config);

int siwifi_platform_register_drv(void);
void siwifi_platform_unregister_drv(void);

static inline struct device *siwifi_platform_get_dev(struct siwifi_plat *siwifi_plat)
{
    return &(siwifi_plat->pci_dev->dev);
}

static inline unsigned int siwifi_platform_get_irq(struct siwifi_plat *siwifi_plat)
{
    return siwifi_plat->pci_dev->irq;
}

#endif /* _SIWIFI_PLAT_H_ */
