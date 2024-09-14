/* FILE NAME:  an8855_mdio.c
 * PURPOSE:
 *    It provides an8855 registers and PHY mdio access.
 *
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */
#include "an8855_mdio.h"

/* NAMING CONSTANT DECLARATIONS
*/

/* MACRO FUNCTION DECLARATIONS
 */
#define AN8855_SMI_ADDR     (1)

/* DATA TYPE DECLARATIONS
*/

/* GLOBAL VARIABLE DECLARATIONS
*/

/* LOCAL SUBPROGRAM DECLARATIONS
*/

/* STATIC VARIABLE DECLARATIONS
*/
u32 g_smi_addr = AN8855_SMI_ADDR;

extern int mdio_read_ext(int phyaddr, int phyreg, int *phydata);
extern int mdio_write_ext(int phyaddr, int phyreg, int phydata);

u32 sf_mii_read_ext(u32 phy_addr, u32 reg, u32 *p_data)
{
	return mdio_read_ext(phy_addr, reg, p_data);
}

u32 sf_mii_write_ext(u32 phy_addr, u32 reg, u32 data)
{
	return mdio_write_ext(phy_addr, reg, data);
}

u32 sf_mii_c45_read_ext(u32 phy_addr, u32 dev, u32 reg, u32 *p_data)
{
	mdio_write_ext(phy_addr, MII_MMD_CTRL, dev);
	mdio_write_ext(phy_addr, MII_MMD_DATA, reg);
	mdio_write_ext(phy_addr, MII_MMD_CTRL, dev | MII_MMD_CTRL_NOINCR);
	return mdio_read_ext(phy_addr, MII_MMD_DATA, p_data);
}

u32 sf_mii_c45_write_ext(u32 phy_addr, u32 dev, u32 reg, u32 data)
{
	mdio_write_ext(phy_addr, MII_MMD_CTRL, dev);
	mdio_write_ext(phy_addr, MII_MMD_DATA, reg);
	mdio_write_ext(phy_addr, MII_MMD_CTRL, dev | MII_MMD_CTRL_NOINCR);
	return mdio_write_ext(phy_addr, MII_MMD_DATA, data);
}


static AIR_MII_READ_FUNC_T g_mii_read = sf_mii_read_ext;
static AIR_MII_WRITE_FUNC_T g_mii_write = sf_mii_write_ext;
static AIR_MII_C45_READ_FUNC_T g_mii_c45_read = sf_mii_c45_read_ext;
static AIR_MII_C45_WRITE_FUNC_T g_mii_c45_write = sf_mii_c45_write_ext;

/* EXPORTED SUBPROGRAM BODIES
*/

/* FUNCTION NAME:   an8855_set_smi_addr
 * PURPOSE:
 *      This API is used to set an8855 smi address.
 * INPUT:
 *      smi_addr -- AN8855 smi address
 * OUTPUT:
 * RETURN:
 * NOTES:
 *      None
 */
void
an8855_set_smi_addr(u32 smi_addr)
{
    an8855_reg_write(0x1028C848, smi_addr);
    g_smi_addr = smi_addr;
}

/* FUNCTION NAME:   an8855_set_mii_callback
 * PURPOSE:
 *      This API is used to set an8855 mii access callbacks.
 * INPUT:
 *      mii_read -- mii read api function
 *      mii_write -- mii write api function
 *      mii_c45_read --  mii c45 read api function
 *      mii_c45_write -- mii c45 write api function
 * OUTPUT:
 * RETURN:
 *      0     -- Successfully set callback.
 *      -1    -- Setting callback failed.
 * NOTES:
 *      None
 */
int
an8855_set_mii_callback(
    AIR_MII_READ_FUNC_T mii_read,
    AIR_MII_WRITE_FUNC_T mii_write,
    AIR_MII_C45_READ_FUNC_T mii_c45_read,
    AIR_MII_C45_WRITE_FUNC_T mii_c45_write)
{
    if (!mii_read || !mii_write || !mii_c45_read || !mii_c45_write)
        return -1;

    g_mii_read = NULL;
    g_mii_write = NULL;
    g_mii_c45_read = NULL;
    g_mii_c45_write = NULL;

    return 0;
}

/* FUNCTION NAME:   an8855_reg_read
 * PURPOSE:
 *      This API is used read an8855 registers.
 * INPUT:
 *      reg -- register offset
 * OUTPUT:
 * RETURN:
 *      Register value
 * NOTES:
 *      Attention!! Customer should implement mdio mutex
 *      lock in this func
 */
u32
an8855_reg_read(u32 reg)
{
    u32 data_h = 0;
    u32 data_l = 0;
    int ret = 0;

    g_mii_write(g_smi_addr, 0x1F, 0x4);
    g_mii_write(g_smi_addr, 0x10, 0x0);

    g_mii_write(g_smi_addr, 0x15, ((reg >> 16) & 0xFFFF));
    g_mii_write(g_smi_addr, 0x16, (reg & 0xFFFF));

    ret = g_mii_read(g_smi_addr, 0x17, &data_h);
    if(ret != 0)
    {
        printk("read reg 0x%x 0x17 fail\n", reg);
    }
    ret = g_mii_read(g_smi_addr, 0x18, &data_l);
    if(ret != 0)
    {
        printk("read reg 0x%x 0x18 fail\n", reg);
    }

    g_mii_write(g_smi_addr, 0x1F, 0x0);
    g_mii_write(g_smi_addr, 0x10, 0x0);
    //printk("read reg:0x%08x data:0x%08x\n", reg, ((data_h << 16) | (data_l & 0xffff)));

    return (((data_h & 0xffff) << 16) | (data_l & 0xffff));
}

/* FUNCTION NAME:   an8855_reg_write
 * PURPOSE:
 *      This API is used write an8855 registers.
 * INPUT:
 *      reg -- register offset
 *      val -- register value
 * OUTPUT:
 * RETURN:
 * NOTES:
 *      Attention!! Customer should implement mdio mutex
 *      lock in this func
 */
void
an8855_reg_write(u32 reg, u32 val)
{
    g_mii_write(g_smi_addr, 0x1F, 0x4);
    g_mii_write(g_smi_addr, 0x10, 0x0);

    g_mii_write(g_smi_addr, 0x11, ((reg >> 16) & 0xFFFF));
    g_mii_write(g_smi_addr, 0x12, (reg & 0xFFFF));

    g_mii_write(g_smi_addr, 0x13, ((val >> 16) & 0xFFFF));
    g_mii_write(g_smi_addr, 0x14, (val & 0xFFFF));

    g_mii_write(g_smi_addr, 0x1F, 0x0);
    g_mii_write(g_smi_addr, 0x10, 0x0);
    //printk("write reg:0x%08x data:0x%08x\n", reg, val);
}

/* FUNCTION NAME:   an8855_phy_read
 * PURPOSE:
 *      This API is used read an8855 phy registers.
 * INPUT:
 *      port_num -- port number, 0~4
 *      reg -- phy register offset
 * OUTPUT:
 *      p_val -- phy register value
 * RETURN:
 *      0 -- read success
 *      -1 -- read failure
 * NOTES:
 *      Attention!! Customer should implement mii mutex
 *      lock in this func
 */
int
an8855_phy_read(u32 port_num, u32 reg, u32 *p_val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    g_mii_read(phy, reg, &data);
    *p_val = data & 0x0000FFFF;

    return 0;
}

/* FUNCTION NAME:   an8855_phy_write
 * PURPOSE:
 *      This API is used write an8855 phy registers.
 * INPUT:
 *      port_num -- port number, 0~4
 *      reg -- phy register offset
 *      val -- phy register value
 * OUTPUT:
 * RETURN:
 *      0 -- write success
 *      -1 -- write failure
 * NOTES:
 *      Attention!! Customer should implement mii mutex
 *      lock in this func
 */
int
an8855_phy_write(u32 port_num, u32 reg, u32 val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    data = val & 0x0000FFFF;
    g_mii_write(phy, reg, data);

    return 0;
}

/* FUNCTION NAME:   an8855_phy_read_cl45
 * PURPOSE:
 *      This API is used read an8855 phy registers.
 * INPUT:
 *      port_num -- port number, 0~4
 *      dev_addr -- phy device type
 *      reg_addr -- phy register offset
 * OUTPUT:
 *      p_val -- phy register value
 * RETURN:
 *      0 -- read success
 *      -1 -- read failure
 * NOTES:
 *      Attention!! Customer should implement mii mutex
 *      lock in this func or before/after calling this func
 */
u32
an8855_phy_read_cl45(u32 port_num, u32 dev_addr, u32 reg_addr, u32 *p_val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    g_mii_c45_read(phy, dev_addr, reg_addr, &data);
    *p_val = data & 0x0000FFFF;

    return 0;
}

/* FUNCTION NAME:   an8855_phy_write_cl45
 * PURPOSE:
 *      This API is used write an8855 phy registers.
 * INPUT:
 *      port_num -- port number, 0~4
 *      dev_addr -- phy device type
 *      reg_addr -- phy register offset
 *      val -- phy register value
 * OUTPUT:
 * RETURN:
 *      0 -- write success
 *      -1 -- write failure
 * NOTES:
 *      Attention!! Customer should implement mii mutex
 *      lock in this func or before/after calling this func
 */
int
an8855_phy_write_cl45(u32 port_num, u32 dev_addr, u32 reg_addr, u32 val)
{
    u32 phy = 0, data = 0;

    if (port_num >= AN8855_PHY_NUM)
        return -1;

    phy = g_smi_addr + port_num;
    data = val & 0x0000FFFF;
    g_mii_c45_write(phy, dev_addr, reg_addr, data);

    return 0;
}
