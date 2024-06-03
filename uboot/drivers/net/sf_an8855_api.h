/* FILE NAME:  an8855_phy.h
 * PURPOSE:
 *      It provides AN8855 phy definition.
 * NOTES:
 *
 */
#ifndef _AN8855_PHY_H_
#define _AN8855_PHY_H_

#include "sfa18_gmac.h"

#define AN8855_PHY_NUM           5
#define AN8855_SMI_ADDR         (1)
#define MII_ADDR_C45            (1<<30)

void an8855_reg_read(struct sgmac_priv *priv, u32 reg, u32 *reg_val);
void an8855_reg_write(struct sgmac_priv *priv, u32 reg, u32 val);
int an8855_phy_read(struct sgmac_priv *priv, u32 port_num, u32 reg, u32 *p_val);
int an8855_phy_write(struct sgmac_priv *priv, u32 port_num, u32 reg, u32 val);
void gphy_calibration(struct sgmac_priv *priv, uint8_t phy_base);
void an8855_run_sram_code(struct sgmac_priv *priv);
void an8855_gbe_1g_setting(struct sgmac_priv *priv);

#endif /* _AN8855_PHY_H_ */