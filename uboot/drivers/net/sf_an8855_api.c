#include "sf_an8855_api.h"

extern struct sgmac_priv *g_priv;
extern int sgmac_mdio_read(struct mii_dev *bus, int addr, int devad, int reg);
extern int sgmac_mdio_write(struct mii_dev *bus, int addr, int devad, int reg, u16 val);

u32 g_smi_addr = AN8855_SMI_ADDR;

void an8855_reg_read(struct sgmac_priv *priv, u32 reg, u32 *reg_val)
{
    u32 rData;
    u16 data_h = 0;
    u16 data_l = 0;

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x1F, 0x4);
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x10, 0x0);

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x15, ((reg >> 16) & 0xFFFF));
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x16, (reg & 0xFFFF));

    data_h = sgmac_mdio_read(priv->bus, g_smi_addr, 0, 0x17);

    data_l = sgmac_mdio_read(priv->bus, g_smi_addr, 0, 0x18);

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x1F, 0x0);
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x10, 0x0);
    //printf("read reg:0x%08x data:0x%08x\n", reg, ((data_h << 16) | (data_l & 0xffff)));

    rData = ((data_h << 16) | (data_l & 0xffff));
    *reg_val = rData;
}

void an8855_reg_write(struct sgmac_priv *priv, u32 reg, u32 val)
{
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x1F, 0x4);
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x10, 0x0);

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x11, ((reg >> 16) & 0xFFFF));
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x12, (reg & 0xFFFF));

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x13, ((val >> 16) & 0xFFFF));
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x14, (val & 0xFFFF));

    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x1F, 0x0);
    sgmac_mdio_write(priv->bus, g_smi_addr, 0, 0x10, 0x0);
    //printf("write reg:0x%08x data:0x%08x\n", reg, val);
}

int an8855_phy_write(struct sgmac_priv *priv, u32 port_num, u32 reg, u32 val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    data = val & 0x0000FFFF;
    sgmac_mdio_write(priv->bus, phy, 0, reg, data);

    return 0;
}

int an8855_phy_read(struct sgmac_priv *priv, u32 port_num, u32 reg, u32 *p_val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    data = sgmac_mdio_read(priv->bus, phy, 0, reg);
    *p_val = data & 0x0000FFFF;

    return 0;
}

void set_gphy_reg_cl45(uint8_t prtid, uint8_t devid, uint16_t reg, uint16_t value)
{
	an8855_phy_write(g_priv, prtid, MII_MMD_CTRL, devid);
	an8855_phy_write(g_priv, prtid, MII_MMD_DATA, reg);
	an8855_phy_write(g_priv, prtid, MII_MMD_CTRL, devid | MII_MMD_CTRL_NOINCR);

	an8855_phy_write(g_priv, prtid, MII_MMD_DATA, value);
}

//todo
uint16_t get_gphy_reg_cl45(uint8_t prtid, uint8_t devid, uint16_t reg)
{
	uint32_t rdata = 0;
	an8855_phy_write(g_priv, prtid, MII_MMD_CTRL, devid);
	an8855_phy_write(g_priv, prtid, MII_MMD_DATA, reg);
	an8855_phy_write(g_priv, prtid, MII_MMD_CTRL, devid | MII_MMD_CTRL_NOINCR);

	an8855_phy_read(g_priv, prtid, MII_MMD_DATA, &rdata);
	return ((uint16_t)rdata);
}

void an8855_run_sram_code(struct sgmac_priv *priv)
{
	an8855_reg_write(priv, 0x10010000, 0x846); //sram code
	an8855_reg_write(priv, 0x10010004, 0x04a); //sram code
	an8855_reg_write(priv, 0x10005034, 0x00f); //cpu enable
	mdelay(10);
	an8855_reg_write(priv, 0x1000a100, 0x023); //disable watch
	mdelay(10);
	an8855_reg_write(priv, 0x1028c840, 0x10);
}

void an8855_gbe_1g_setting(struct sgmac_priv *priv)
{
	u32 port = 0, reg = 0x200, val = 0;

	for (port = 0; port < 5; port++)
	{
		set_gphy_reg_cl45(port, 0x1e, 0x11, 0x0f00);
	}

	while (reg < 0x230)
	{
		val = get_gphy_reg_cl45(0, 0x1f, reg);
		val = (val & (~0x3f)) | 0x20;
		set_gphy_reg_cl45(0, 0x1f, reg, val);
		reg += 2;
	}

	// force rgmii mode
	an8855_reg_write(priv, 0x10210a00, 0xa3159000);
	an8855_reg_write(priv, 0x1028c82c, 0x20101);

	// dsiable tx/rx delay
	an8855_reg_write(priv, 0x1028C854, 0);
	an8855_reg_write(priv, 0x1028C84c, 0);

	/* port phy power up */
	for (port = 0; port < 5; port++)
	{
		an8855_phy_write(priv, port, 0x0, 0x1240);
	}
}
