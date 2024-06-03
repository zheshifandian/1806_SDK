// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 JLSemi Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "jlsemi-core.h"

#define JL1XXX_PAGE24		24
#define JL1XXX_LED_BLINK_REG	25

#define JL1XXX_PAGE128		128
#define JL1XXX_LED_GPIO_REG	29
#define JL1XXX_WOL_CTRL_REG	28

#define JL2XXX_PAGE3332		3332
#define JL2XXX_LED_CTRL_REG	16
#define JL2XXX_PAGE4096		4096
#define JL2XXX_LED_BLINK_REG	20
#define JL2XXX_LED_POLARITY_REG	19

#define JL2XXX_PAGE128		128
#define JL2XXX_FLD_CTRL_REG	28
#define JL2XXX_FLD_EN		BIT(13)
#define JL2XXX_FLD_MASK		0x1800
#define JL2XXX_FLD_MASK_HEAD	11
#define JL2XXX_FLD_DELAY_00MS	0
#define JL2XXX_FLD_DELAY_10MS	1
#define JL2XXX_FLD_DELAY_20MS	2
#define JL2XXX_FLD_DELAY_40MS	3

#define JL2XXX_SPEED10		0
#define JL2XXX_SPEED100		1
#define JL2XXX_SPEED1000	2

#define JL2XXX_PHY_MODE_REG	30
#define JL2XXX_FIBER_1000	BIT(12)
#define JL2XXX_FIBER_100	BIT(11)
#define JL2XXX_PHY_FIBER_MODE_MASK	0x1800
#define JL2XXX_BMCR_DUPLEX	BIT(8)
#define JL2XXX_LPA_FIBER_1000HALF	0x40
#define JL2XXX_LPA_FIBER_1000FULL	0x20
#define JL2XXX_BMCR_SPEED_LSB	BIT(13)
#define JL2XXX_BMCR_SPEED_MSB	BIT(6)
#define JL2XXX_BMCR_AN_RESTART	BIT(9)



#define JL2XXX_SUPP_LED_MODE	(JL2XXX_LED0_LINK10 | \
				 JL2XXX_LED0_LINK100 | \
				 JL2XXX_LED0_LINK1000 | \
				 JL2XXX_LED0_ACTIVITY | \
				 JL2XXX_LED1_LINK10 | \
				 JL2XXX_LED1_LINK100 | \
				 JL2XXX_LED1_LINK1000 | \
				 JL2XXX_LED1_ACTIVITY | \
				 JL2XXX_LED2_LINK10 | \
				 JL2XXX_LED2_LINK100 | \
				 JL2XXX_LED2_LINK1000 | \
				 JL2XXX_LED2_ACTIVITY)

#define JL1XXX_SUPP_GPIO	(JL1XXX_GPIO_LED0_EN | \
				 JL1XXX_GPIO_LED0_OUT | \
				 JL1XXX_GPIO_LED1_EN | \
				 JL1XXX_GPIO_LED1_OUT)

#define JL1XXX_SUPP_LED_MODE	(JL1XXX_LED0_EEE | \
				 JL1XXX_LED0_100_ACTIVITY | \
				 JL1XXX_LED0_10_ACTIVITY | \
				 JL1XXX_LED0_100_LINK | \
				 JL1XXX_LED0_10_LINK | \
				 JL1XXX_LED1_EEE | \
				 JL1XXX_LED1_100_ACTIVITY | \
				 JL1XXX_LED1_10_ACTIVITY | \
				 JL1XXX_LED1_100_LINK | \
				 JL1XXX_LED1_10_LINK)

/************************* Configuration section *************************/


/************************* JLSemi iteration code *************************/
static int jl1xxx_led_static_op_set(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;

	/* Enable LED operation */
	jlsemi_set_bits(phydev, JL1XXX_PAGE7,
			JL1XXX_LED_REG, JL1XXX_LED_EN);

	/* Set led mode */
	if (priv->led.enable & JL1XXX_LED_MODE_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE129,
					      JL1XXX_LED_MODE_REG,
					      JL1XXX_SUPP_LED_MODE,
					      priv->led.mode);
		if (err < 0)
			return err;
	}
	/* Set led period */
	if (priv->led.enable & JL1XXX_LED_GLOABL_PERIOD_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE24,
					      JL1XXX_LED_BLINK_REG,
					      LED_PERIOD_MASK,
					      LEDPERIOD(
					      priv->led.global_period));
		if (err < 0)
			return err;
	}
	/* Set led on time */
	if (priv->led.enable & JL1XXX_LED_GLOBAL_ON_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE24,
					      JL1XXX_LED_BLINK_REG,
					      LED_ON_MASK,
					      LEDON(priv->led.global_on));
		if (err < 0)
			return err;
	}
	/*Set led gpio output */
	if (priv->led.enable & JL1XXX_LED_GPIO_OUT_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE128,
					      JL1XXX_LED_GPIO_REG,
					      JL1XXX_SUPP_GPIO,
					      priv->led.gpio_output);
		if (err < 0)
			return err;
	}

	return 0;
}

static int jl1xxx_c_macro_led_cfg_get(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	/* Config LED */
	struct jl_led_ctrl led_cfg = {
		.enable		= JL1XXX_LED_CTRL_EN,
		.mode		= JL1XXX_CFG_LED_MODE,
		.global_period	= JL1XXX_GLOBAL_PERIOD_MS,
		.global_on	= JL1XXX_GLOBAL_ON_MS,
		.gpio_output	= JL1XXX_CFG_GPIO,
	};

	priv->led = led_cfg;

	return 0;
}

static int jl1xxx_c_macro_wol_cfg_get(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	struct jl_wol_ctrl wol_cfg = {
		.enable		= JL1XXX_WOL_CTRL_EN,
	};

	priv->wol = wol_cfg;

	return 0;
}

static int jl1xxx_c_macro_intr_cfg_get(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	struct jl_intr_ctrl intr_cfg = {
		.enable		= JL1XXX_INTR_CTRL_EN,
	};

	priv->intr = intr_cfg;

	return 0;
}

static int jl1xxx_c_macro_mdi_cfg_get(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	struct jl_mdi_ctrl mdi_cfg = {
		.enable		= JL1XXX_MDI_CTRL_EN,
		.rate		= JL1XXX_MDI_RATE,
		.amplitude	= JL1XXX_MDI_AMPLITUDE,
	};

	priv->mdi = mdi_cfg;

	return 0;
}

static int jl1xxx_c_macro_rmii_cfg_get(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;

	struct jl_rmii_ctrl rmii_cfg = {
		.enable		= JL1XXX_RMII_CTRL_EN,
		.tx_timing	= JL1XXX_RMII_TX_TIMING,
		.rx_timing	= JL1XXX_RMII_RX_TIMING,
	};

	priv->rmii = rmii_cfg;

	return 0;
}

static int jl1xxx_wol_operation_args(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	struct jl_wol_ctrl *wol = &priv->wol;

	jl1xxx_c_macro_wol_cfg_get(phydev);

	/* Supported by default */
	wol->ethtool = false;

	return 0;
}

static int jl1xxx_intr_operation_args(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	struct jl_intr_ctrl *intr = &priv->intr;

	jl1xxx_c_macro_intr_cfg_get(phydev);

	/* Not supported by default */
	intr->ethtool = false;

	return 0;
}

static int jl1xxx_mdi_operation_args(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	struct jl_mdi_ctrl *mdi = &priv->mdi;

	jl1xxx_c_macro_mdi_cfg_get(phydev);

	/* Not supported by default */
	mdi->ethtool = false;

	return 0;
}

static int jl1xxx_rmii_operation_args(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	struct jl_rmii_ctrl *rmii = &priv->rmii;

	jl1xxx_c_macro_rmii_cfg_get(phydev);

	/* Not supported by default */
	rmii->ethtool = false;

	return 0;
}

static int jl1xxx_led_operation_args(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	struct jl_led_ctrl *led = &priv->led;

	jl1xxx_c_macro_led_cfg_get(phydev);

	/* Not supported by default */
	led->ethtool = false;

	return 0;
}

static inline int __genphy_setup_forced(struct phy_device *phydev)
{
	int err;
	int ctl = 0;

	phydev->pause = phydev->asym_pause = 0;

	if (phydev->speed == SPEED_1000)
		ctl |= BMCR_SPEED1000;
	else if (phydev->speed == SPEED_100)
		ctl |= BMCR_SPEED100;

	if (phydev->duplex == DUPLEX_FULL)
		ctl |= BMCR_FULLDPLX;

	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

	return err;
}

int jl1xxx_mdi_static_op_set(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;

	if (priv->mdi.enable & JL1XXX_MDI_RATE_EN) {
		err = jlsemi_set_bits(phydev, JL1XXX_PAGE24,
				      JL1XXX_REG24, priv->mdi.rate);
		if (err < 0)
			return err;
	}

	if (priv->mdi.enable & JL1XXX_MDI_AMPLITUDE_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE24,
					      JL1XXX_REG24,
					      JL1XXX_MDI_TX_BM_MASK,
					      JL1XXX_MDI_TX_BM(
					      priv->mdi.amplitude));
		if (err < 0)
			return err;
	}

	return 0;
}

int jl1xxx_rmii_static_op_set(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;

	if (priv->rmii.enable & JL1XXX_RMII_MODE_EN) {
		err = jlsemi_set_bits(phydev, JL1XXX_PAGE7,
				      JL1XXX_REG16, JL1XXX_RMII_MODE);
		if (err < 0)
			return err;
	} else {
		err = jlsemi_clear_bits(phydev, JL1XXX_PAGE7,
					JL1XXX_REG16, JL1XXX_RMII_MODE);
		if (err < 0)
			return err;
		return 0;
	}

	if (priv->rmii.enable & JL1XXX_RMII_CLK_50M_INPUT_EN) {
		err = jlsemi_set_bits(phydev, JL1XXX_PAGE7,
				      JL1XXX_REG16,
				      JL1XXX_RMII_CLK_50M_INPUT);
		if (err < 0)
			return err;
	} else {
		err = jlsemi_clear_bits(phydev, JL1XXX_PAGE7,
					JL1XXX_REG16,
					JL1XXX_RMII_CLK_50M_INPUT);
		if (err < 0)
			return err;
	}

	if (priv->rmii.enable & JL1XXX_RMII_CRS_DV_EN) {
		err = jlsemi_set_bits(phydev, JL1XXX_PAGE7,
				      JL1XXX_REG16,
				      JL1XXX_RMII_CRS_DV);
		if (err < 0)
			return err;
	} else {
		err = jlsemi_clear_bits(phydev, JL1XXX_PAGE7,
					JL1XXX_REG16,
					JL1XXX_RMII_CRS_DV);
		if (err < 0)
			return err;
	}

	if (priv->rmii.enable & JL1XXX_RMII_TX_SKEW_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE7,
					      JL1XXX_REG16,
					      JL1XXX_RMII_TX_SKEW_MASK,
					      JL1XXX_RMII_TX_SKEW(
					      priv->rmii.tx_timing));
		if (err < 0)
			return err;
	}

	if (priv->rmii.enable & JL1XXX_RMII_RX_SKEW_EN) {
		err = jlsemi_modify_paged_reg(phydev, JL1XXX_PAGE7,
					      JL1XXX_REG16,
					      JL1XXX_RMII_RX_SKEW_MASK,
					      JL1XXX_RMII_RX_SKEW(
					      priv->rmii.rx_timing));
		if (err < 0)
			return err;
	}

	return 0;
}

int jl1xxx_wol_dynamic_op_get(struct phy_device *phydev)
{
	return jlsemi_fetch_bit(phydev, JL1XXX_PAGE129,
				JL1XXX_WOL_CTRL_REG, JL1XXX_WOL_DIS);
}

int jl1xxx_intr_ack_event(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;

	if (priv->intr.enable & JL1XXX_INTR_STATIC_OP_EN) {
		err = phy_read(phydev, MDIO_DEVAD_NONE, JL1XXX_INTR_STATUS_REG);
		if (err < 0)
			return err;
	}

	return 0;
}

int jl1xxx_intr_static_op_set(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;
	int ret = 0;

	if (priv->intr.enable & JL1XXX_INTR_LINK_CHANGE_EN)
		ret |= JL1XXX_INTR_LINK;
	if (priv->intr.enable & JL1XXX_INTR_AN_ERR_EN)
		ret |= JL1XXX_INTR_AN_ERR;

	err = jlsemi_set_bits(phydev, JL1XXX_PAGE7,
			      JL1XXX_INTR_REG, ret);
	if (err < 0)
		return err;

	return 0;
}

int jl1xxx_operation_args_get(struct phy_device *phydev)
{
	jl1xxx_led_operation_args(phydev);
	jl1xxx_wol_operation_args(phydev);
	jl1xxx_intr_operation_args(phydev);
	jl1xxx_mdi_operation_args(phydev);
	jl1xxx_rmii_operation_args(phydev);

	return 0;
}

int jl1xxx_static_op_init(struct phy_device *phydev)
{
	struct jl1xxx_priv *priv = phydev->priv;
	int err;

	if (priv->led.enable & JL1XXX_LED_STATIC_OP_EN) {
		err = jl1xxx_led_static_op_set(phydev);
		if (err < 0)
			return err;
	}

	if (priv->intr.enable & JL1XXX_INTR_STATIC_OP_EN) {
		err = jl1xxx_intr_static_op_set(phydev);
		if (err < 0)
			return err;
	}

	if (priv->mdi.enable & JL1XXX_MDI_STATIC_OP_EN) {
		err = jl1xxx_mdi_static_op_set(phydev);
		if (err < 0)
			return err;
	}

	if (priv->rmii.enable & JL1XXX_RMII_STATIC_OP_EN) {
		err = jl1xxx_rmii_static_op_set(phydev);
		if (err < 0)
			return err;
	}

	return 0;
}

int jlsemi_soft_reset(struct phy_device *phydev)
{
	int err;

	err = jlsemi_set_bits(phydev, JL2XXX_PAGE0,
			      JL2XXX_BMCR_REG, JL2XXX_SOFT_RESET);
	if (err < 0)
		return err;
	/* Wait soft reset complete*/
	udelay(600000);

	return 0;
}

/********************** Convenience function for phy **********************/

/**
 * jlsemi_write_page() - write the page register
 * @phydev: a pointer to a &struct phy_device
 * @page: page values
 */
int jlsemi_write_page(struct phy_device *phydev, int page)
{
	return phy_write(phydev, MDIO_DEVAD_NONE, JLSEMI_PAGE31, page);
}

/**
 * jlsemi_read_page() - write the page register
 * @phydev: a pointer to a &struct phy_device
 *
 * Return: get page values at present
 */
int jlsemi_read_page(struct phy_device *phydev)
{
	return phy_read(phydev, MDIO_DEVAD_NONE, JLSEMI_PAGE31);
}

/**
 * __jlsemi_save_page() - save the page value
 *@phydev: a pointer to a &struct phy_device
 *
 * Return: save page value
 */
static inline int __jlsemi_save_page(struct phy_device *phydev)
{
	return jlsemi_read_page(phydev);
}

/**
 * __jlsemi_select_page() - restore the page register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page
 *
 * Return:
 * @oldpgae: this is last page value
 * @ret: if page is change it will return new page value
 */
static inline int __jlsemi_select_page(struct phy_device *phydev, int page)
{
	int ret, oldpage;

	oldpage = ret = __jlsemi_save_page(phydev);
	if (ret < 0)
		return ret;

	if (oldpage != page) {
		ret = jlsemi_write_page(phydev, page);
		if (ret < 0)
			return ret;
	}

	return oldpage;
}

/**
 * __jlsemi_restore_page() - restore the page register
 * @phydev: a pointer to a &struct phy_device
 * @oldpage: the old page, return value from __jlsemi_save_page() or
 * __jlsemi_select_page()
 * @ret: operation's return code
 *
 * Returns:
 *   @oldpage if it was a negative value, otherwise
 *   @ret if it was a negative errno value, otherwise
 *   phy_write_page()'s negative value if it were in error, otherwise
 *   @ret
 */
static inline int __jlsemi_restore_page(struct phy_device *phydev,
					int oldpage, int ret)
{
	int r;

	if (oldpage >= 0) {
		r = jlsemi_write_page(phydev, oldpage);

		/* Propagate the operation return code if the page write
		 * was successful.
		 */
		if (ret >= 0 && r < 0)
			ret = r;
	} else {
		/* Propagate the phy page selection error code */
		ret = oldpage;
	}

	return ret;
}

/**
 * __jlsemi_modify_reg() - Convenience function for modifying a PHY register
 * @phydev: a pointer to a &struct phy_device
 * @regnum: register number
 * @mask: bit mask of bits to clear
 * @set: bit mask of bits to set
 *
 * Returns negative errno, 0 if there was no change, and 1 in case of change
 */
static inline int __jlsemi_modify_reg(struct phy_device *phydev,
				      u32 regnum, u16 mask, u16 set)
{
	int newval, ret;

	ret = phy_read(phydev, MDIO_DEVAD_NONE, regnum);
	if (ret < 0)
		return ret;

	newval = (ret & ~mask) | set;
	if (newval == ret)
		return 0;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, regnum, newval);

	return ret < 0 ? ret : 1;
}

/**
 * jlsemi_modify_paged_reg() - Function for modifying a paged register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number
 * @mask: bit mask of bits to clear
 * @set: bit mask of bits to set
 *
 * Returns negative errno, 0 if there was no change, and 1 in case of change
 */
int jlsemi_modify_paged_reg(struct phy_device *phydev,
			    int page, u32 regnum,
			    u16 mask, u16 set)
{
	int ret = 0, oldpage;

	oldpage = __jlsemi_select_page(phydev, page);
	if (oldpage >= 0)
		ret = __jlsemi_modify_reg(phydev, regnum, mask, set);

	return __jlsemi_restore_page(phydev, oldpage, ret);
}

/**
 * jlsemi_set_bits() - Convenience function for setting bits in a PHY register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number to write
 * @val: bits to set
 */
int jlsemi_set_bits(struct phy_device *phydev,
		    int page, u32 regnum, u16 val)
{
	return jlsemi_modify_paged_reg(phydev, page, regnum, 0, val);
}

/**
 * jlsemi_clear_bits - Convenience function for clearing bits in a PHY register
 * @phydev: the phy_device struct
 * @page: the page for the phy
 * @regnum: register number to write
 * @val: bits to clear
 */
int jlsemi_clear_bits(struct phy_device *phydev,
		      int page, u32 regnum, u16 val)
{
	return jlsemi_modify_paged_reg(phydev, page, regnum, val, 0);
}

/**
 * jlsemi_fetch_bit() - Convenience function for setting bits in a PHY register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number to write
 * @val: bit to get
 *
 * Note:
 * you only get one bit at meanwhile
 *
 */
int jlsemi_fetch_bit(struct phy_device *phydev,
		   int page, u32 regnum, u16 val)
{
	int ret = 0, oldpage;

	oldpage = __jlsemi_select_page(phydev, page);
	if (oldpage >= 0) {
		ret = phy_read(phydev, MDIO_DEVAD_NONE, regnum);
		if (ret < 0)
			return ret;
		ret = ((ret & val) == val) ? 1 : 0;
	}

	return __jlsemi_restore_page(phydev, oldpage, ret);
}

/**
 * jlsemi_read_paged() - Convenience function for reading a paged register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number
 *
 * Same rules as for phy_read().
 */
int jlsemi_read_paged(struct phy_device *phydev, int page, u32 regnum)
{
	int ret = 0, oldpage;

	oldpage = __jlsemi_select_page(phydev, page);
	if (oldpage >= 0)
		ret = phy_read(phydev, MDIO_DEVAD_NONE, regnum);

	return __jlsemi_restore_page(phydev, oldpage, ret);
}
