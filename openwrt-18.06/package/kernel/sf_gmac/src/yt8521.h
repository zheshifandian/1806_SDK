#ifndef _YT8521_H_
#define _YT8521_H_

#define REG_DEBUG_ADDR_OFFSET				0x1e
#define REG_DEBUG_DATA						0x1f
#define YT8521_EXTREG_SLEEP_CONTROL1		0x27
#define YT8521_SPEED_AUTO_DOWNGRADE_CTRL    0x14
#define YT8521_EN_SLEEP_SW_BIT				15

int ytphy_read_ext(struct mii_bus *bus, struct phy_device *phydev, u32 regnum);
int ytphy_write_ext(struct mii_bus *bus, struct phy_device *phydev, u32 regnum, u16 val);
int yt8521_config_init(struct mii_bus *bus, struct phy_device *phydev);

#endif
