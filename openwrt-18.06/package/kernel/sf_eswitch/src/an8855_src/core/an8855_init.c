/* FILE NAME:  an8855_init.c
 * PURPOSE:
 *    It provides an8855 switch intialize flow.
 *
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */
#include "an8855_mdio.h"
#include "an8855_phy.h"

/* NAMING CONSTANT DECLARATIONS
*/
#define PAG_SEL_AN8855H  (2)
#define PAG_SEL_AN8852R  (5)
#define MAX_NUM_OF_GIGA_PORTS      (5)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
*/

/* GLOBAL VARIABLE DECLARATIONS
*/

/* LOCAL SUBPROGRAM DECLARATIONS
*/

/* STATIC VARIABLE DECLARATIONS
*/

/* EXPORTED SUBPROGRAM BODIES
*/

/* FUNCTION NAME:   an8855_hw_reset
 * PURPOSE:
 *      This API is used to reset an8855 hw.
 * INPUT:
 * OUTPUT:
 * RETURN:
 * NOTES:
 *      Attention!! Customer should implement this func
 */
void
an8855_hw_reset(void)
{
    //dbg_print(">>>>> an8855_hw_reset\n");
    /* set an8855 reset pin to 0 */

    /* delay 100ms */

    /* set an8855 reset pin to 1 */

    /* delay 600ms */

}

/* FUNCTION NAME:   an8855_sw_reset
 * PURPOSE:
 *      This API is used to reset an8855 system.
 * INPUT:
 * OUTPUT:
 * RETURN:
 * NOTES:
 */
void
an8855_sw_reset(void)
{
    //dbg_print(">>>>> an8855_sw_reset\n");
    an8855_reg_write(0x100050c0, 0x80000000);
    an8855_udelay(100000);
}

/* FUNCTION NAME:   an8855_phy_calibration_setting
 * PURPOSE:
 *      This API is used to set an8855 phy calibration.
 * INPUT:
 * OUTPUT:
 * RETURN:
 * NOTES:
 *      None
 */
void
an8855_phy_calibration_setting(void)
{
    int i = 0;

    //dbg_print("\nSMI IOMUX initial ...");
    an8855_reg_write(0x10000070, 0x2);
    an8855_udelay(10000);
    //dbg_print("\nGPHY initial ...");
    an8855_reg_write(0x1028C840, 0x0);
    for(i = 0; i <= 4; i++)
    {
        an8855_phy_write(i, 0, 0x1040);
    }
    an8855_udelay(10000);
    //dbg_print("Done");
    //dbg_print("\nSw calibration ... ");
extern void gphy_calibration(uint8_t phy_base);
    gphy_calibration(g_smi_addr);
    //dbg_print("\nDone");
}

void an8855_run_sram_code(void)
{
	an8855_reg_write(0x10010000, 0x846); //sram code
	an8855_reg_write(0x10010004, 0x04a); //sram code
	an8855_reg_write(0x10005034, 0x00f); //cpu enable
	an8855_udelay(10000);
	an8855_reg_write(0x1000a100, 0x023); //disable watch
	an8855_udelay(10000);
	an8855_reg_write(0x1028c840, 0x10);
}

void an8855_gbe_1g_setting(void)
{
	u32 port = 0, reg = 0x200, val = 0;

	for (port = 0; port < MAX_NUM_OF_GIGA_PORTS; port++)
	{
		an8855_phy_write_cl45(port, 0x1e, 0x11, 0x0f00);
	}

	while (reg < 0x230)
	{
		an8855_phy_read_cl45(0, 0x1f, reg, &val);
		val = (val & (~0x3f)) | 0x20;
		an8855_phy_write_cl45(0, 0x1f, reg, val);
		reg += 2;
	}

	/* port phy power up */
	for (port = 0; port < MAX_NUM_OF_GIGA_PORTS; port++)
	{
		an8855_phy_write(port, 0x0, 0x1240);
	}
}

/* FUNCTION NAME:   an8855_init
 * PURPOSE:
 *      This API is used to init an8855.
 * INPUT:
 * OUTPUT:
 * RETURN:
 *      0 -- init success
 *      -1 -- init failure
 * NOTES:
 *      Attention!! Customer should implement part of this func
 */
int
an8855_init(void)
{
    u32 data = 0;

    /* an8855 hw reset */
    //an8855_hw_reset();

    /* an8855 system reset */
    //an8855_sw_reset();

    /* Keep the clock ticking when all ports link down */
    data = an8855_reg_read(0x10213e1c);
    data &= ~(0x3);
    an8855_reg_write(0x10213e1c, data);

	an8855_run_sram_code();
	an8855_gbe_1g_setting();
    /* internal phy calibration */
    /* please comment out this func after calibration data loaded from ROM code */
    //an8855_phy_calibration_setting();

    return 0;
}
