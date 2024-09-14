/**
 ******************************************************************************
 *
 * @file siwifi_irqs.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */
#ifndef _SIWIFI_IRQS_H_
#define _SIWIFI_IRQS_H_

#include <linux/interrupt.h>

/* IRQ handler to be registered by platform driver */
irqreturn_t siwifi_irq_hdlr(int irq, void *dev_id);

void siwifi_task(unsigned long data);

#endif /* _SIWIFI_IRQS_H_ */
