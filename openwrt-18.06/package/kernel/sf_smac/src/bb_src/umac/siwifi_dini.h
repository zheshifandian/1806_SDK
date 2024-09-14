/**
 ****************************************************************************************
 *
 * @file siwifi_dini.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_DINI_H_
#define _SIWIFI_DINI_H_

#include <linux/pci.h>
#include "siwifi_platform.h"

int siwifi_dini_platform_init(struct pci_dev *pci_dev,
                            struct siwifi_plat **siwifi_plat);

#endif /* _SIWIFI_DINI_H_ */
