// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/net/phy/jlsemi.c
 *
 * Driver for JLSemi PHYs
 *
 * Author: Gangqiao Kuang <gqkuang@jlsemi.com>
 *
 * Copyright (c) 2021 JingLue Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include "jlsemi-core.h"

#define DRIVER_VERSION		"1.2.15"
#define DRIVER_NAME_100M	"JL1xxx Fast Ethernet " DRIVER_VERSION
#define DRIVER_NAME_1000M	"JL2xxx Gigabit Ethernet " DRIVER_VERSION

static int jl1xxx_probe(struct phy_device *phydev)
{
	struct jl1xxx_priv *jl1xxx = NULL;
	int err;

	jl1xxx = malloc(sizeof(*jl1xxx));
	if (!jl1xxx)
		return -1;

	phydev->priv = jl1xxx;

	err = jl1xxx_operation_args_get(phydev);
	if (err < 0)
		return err;

	jl1xxx->static_inited = false;

	return 0;
}

static int jl1xxx_config_init(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int ret;

	if (!priv->static_inited) {
		ret = jl1xxx_static_op_init(phydev);
		if (ret < 0)
			return ret;
		priv->static_inited = JLSEMI_PHY_NOT_REENTRANT;
	}

	return 0;
}

static int jl1xxx_remove(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	if (priv)
		free(priv);

	return 0;
}

static struct phy_driver jlsemi_driver = {
	.uid		= JL1XXX_PHY_ID,
	.mask		= JLSEMI_PHY_ID_MASK,
	.name           = DRIVER_NAME_100M,
	/* PHY_BASIC_FEATURES */
	.features	= PHY_BASIC_FEATURES,
	.probe		= jl1xxx_probe,
	.config		= jl1xxx_config_init,
	.startup	= &genphy_startup,
	.shutdown	= jl1xxx_remove,
};

int phy_jlsemi_init(void)
{
	phy_register(&jlsemi_driver);

	return 0;
}
