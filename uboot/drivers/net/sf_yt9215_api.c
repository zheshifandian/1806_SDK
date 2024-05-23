#include "sf_yt9215_api.h"

extern int sgmac_mdio_read(struct mii_dev *bus, int addr, int devad, int reg);
extern int sgmac_mdio_write(struct mii_dev *bus, int addr, int devad, int reg, u16 val);

void yt9215_reg_read(struct sgmac_priv *priv, u32 reg_addr, u32 *reg_val)
{
	u32 rData;
	u8 regAddr;
	u16 regVal;

	regAddr = ((YT_DEFAULT_ID<<2) | (REG_ADDR_BIT1_ADDR<<1) | (REG_ADDR_BIT0_READ));
	/* Set reg_addr[31:16] */
	regVal = (reg_addr >> 16) & 0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);

	/* Set reg_addr[15:0] */
	regVal = reg_addr & 0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);

	regAddr = ((YT_DEFAULT_ID<<2) | (REG_ADDR_BIT1_DATA<<1) | ((REG_ADDR_BIT0_READ)));
	/* Read Data [31:16] */
	regVal = 0x0;
	regVal = sgmac_mdio_read(priv->bus, YT9215_PHY_ADDR, 0, regAddr);
	rData = (u32)(regVal<<16);

	/* Read Data [15:0] */
	regVal = 0x0;
	regVal =sgmac_mdio_read(priv->bus, YT9215_PHY_ADDR, 0, regAddr);

	rData |= regVal;
	*reg_val = rData;
}

void yt9215_reg_write(struct sgmac_priv *priv, u32 reg_addr, u32 reg_value)
{
	u8 regAddr;
	u16 regVal;

	regAddr = ((YT_DEFAULT_ID<<2) | (REG_ADDR_BIT1_ADDR<<1) | (REG_ADDR_BIT0_WRITE));

	/* Set reg_addr[31:16] */
	regVal = (reg_addr >> 16) & 0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);
	/* Set reg_addr[15:0] */
	regVal = reg_addr & 0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);

	/* Write Data [31:16] out */
	regAddr = ((YT_DEFAULT_ID<<2) | (REG_ADDR_BIT1_DATA<<1) | (REG_ADDR_BIT0_WRITE));
	regVal = (reg_value >> 16) & 0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);

	/* Write Data [15:0] out */
	regVal = reg_value&0xffff;
	sgmac_mdio_write(priv->bus, YT9215_PHY_ADDR, 0, regAddr, regVal);
}
