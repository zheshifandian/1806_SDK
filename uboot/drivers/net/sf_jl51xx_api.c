#include "sf_jl51xx_api.h"

extern struct sgmac_priv *g_priv;
extern int sgmac_mdio_read(struct mii_dev *bus, int addr, int devad, int reg);
extern int sgmac_mdio_write(struct mii_dev *bus, int addr, int devad, int reg, u16 val);

u16 jl_smi_read(u8 phy, u8 reg)
{
	u16 val;
	val = sgmac_mdio_read(g_priv->bus, phy, 0, reg);
	return val;
}

void jl_smi_write(u8 phy, u8 reg, u16 val)
{
	sgmac_mdio_write(g_priv->bus, phy, 0, reg, val);
}

u16 jl_phy_reg_read_ext(u32 phy, u32 page, u32 reg)
{
	jl_smi_write(phy, 0x1f, page);
	return jl_smi_read(phy, reg);
}

void jl_phy_reg_write_ext(u32 phy, u32 page, u32 reg, u16 val)
{
	jl_smi_write(phy, 0x1f, page);
	jl_smi_write(phy, reg, val);
}

int __apb_reg_read(u32 reg, u32 *buf, u8 size)
{
	u32 phy_0 = 0;
	u32 phy_reg = 31;
	u32 page_0 = 0;
	u32 page_1 = 1;
	u32 reg_l_14b = 0;
	u32 reg_h_16b = 0;
	u16 try = 1000;
	u32 tmp = 0;
	u8 pg0_burst = 0;
	u8 pg1_burst = 0;
	u16 wide_en = 0;
	u16 burst_ctrl = 0;
	int i, j;

	if (size > APB_REG_BUSRT_SIZE_MAX) {
		printf("[%s:%d] Brust size overflow, max burst size is 8\n", __func__, __LINE__);
		return -1;
	}

	/* set page 0 */
	jl_smi_write(phy_0, phy_reg, page_0);

	/* 1. set burst size to 0(1 word)
	 * '0' is same as '1', but would not enable the wide range
	 */
	wide_en = ((size > 1) ? 1: 0);
	burst_ctrl = (size << 2) | (wide_en << 1);
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_2, burst_ctrl);

	/* 2. set register address && issue read operation */
	reg_l_14b = ((reg << 2) & 0x0000fffc) | 1;
	reg_h_16b = (reg >> 14) & 0x000003fff;
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_1, reg_h_16b);
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_0, reg_l_14b);

	/* 4. check operation done & status */
	while (--try) {
		tmp = jl_smi_read(phy_0, REG_NO_OPR_CTRL_0);
		if (!(tmp & 1))
			break;
	}

	if (!try) {
		printf("[%s:%d] read apb register timeout\n", __func__, __LINE__);
		return -2;
	}

	tmp = jl_smi_read(phy_0, REG_NO_OPR_CTRL_2);
	if ((tmp & 1)) {
		printf("[%s:%d] read apb register fail\n", __func__, __LINE__);
		return -3;
	}

	/* 5. read data */
	pg0_burst = size;
	if (size > APB_REG_BUSRT_SIZE_PG0_MAX) {
		pg0_burst = APB_REG_BUSRT_SIZE_PG0_MAX;
		pg1_burst = size - APB_REG_BUSRT_SIZE_PG0_MAX;
	}
	for (i = 0; i < pg0_burst; i++) {
		buf[i] = jl_smi_read(phy_0, REG_NO_OPR_DATA0_L + (i * 2));
		tmp = jl_smi_read(phy_0, REG_NO_OPR_DATA0_L + (i * 2) + 1);
		buf[i] |= tmp << 16;
	}
	if (size > APB_REG_BUSRT_SIZE_PG0_MAX) {
		/* set page 1 */
		jl_smi_write(phy_0, phy_reg, page_1);
		for (j = 0; j < pg1_burst; j++) {
			buf[i + j] = jl_smi_read(phy_0, REG_NO_OPR_DATA6_L + (j * 2));
			tmp = jl_smi_read(phy_0, REG_NO_OPR_DATA6_L + (j * 2) + 1);
			buf[i + j] |= tmp << 16;
		}
	}

	return 0;
}

int __apb_reg_write(u32 reg, u32 *buf, u8 size)
{
	u32 phy_0 = 0;
	u32 phy_reg = 31;
	u32 page_0 = 0;
	u32 page_1 = 1;
	u32 reg_l_14b = 0;
	u32 reg_h_16b = 0;
	u16 try = 1000;
	u32 tmp = 0;
	u8 pg0_burst = 0;
	u8 pg1_burst = 0;
	u16 wide_en = 0;
	u16 burst_ctrl = 0;
	int i, j;

	if (size > APB_REG_BUSRT_SIZE_MAX) {
		printf("[%s:%d] Brust size overflow, max burst size is 8\n", __func__, __LINE__);
		return -1;
	}

	/* set page 0 */
	jl_smi_write(phy_0, phy_reg, page_0);

	/* 1. write data */
	pg0_burst = size;
	if (size > APB_REG_BUSRT_SIZE_PG0_MAX) {
		pg0_burst = APB_REG_BUSRT_SIZE_PG0_MAX;
		pg1_burst = size - APB_REG_BUSRT_SIZE_PG0_MAX;
	}
	for (i = 0; i < pg0_burst; i++) {
		jl_smi_write(phy_0, REG_NO_OPR_DATA0_L + (i * 2), (buf[i] & 0x0000ffff));
		jl_smi_write(phy_0, REG_NO_OPR_DATA0_L + (i * 2 + 1), ((buf[i] >> 16) & 0x0000ffff));
	}
	if (size > APB_REG_BUSRT_SIZE_PG0_MAX) {
		/* set page 1 */
		jl_smi_write(phy_0, phy_reg, page_1);
		for (j = 0; j < pg1_burst; j++) {
			jl_smi_write(phy_0, REG_NO_OPR_DATA6_L + (j * 2), (buf[i + j] & 0x0000ffff));
			jl_smi_write(phy_0, REG_NO_OPR_DATA6_L + (j * 2 + 1), ((buf[i + j] >> 16) & 0x0000ffff));
		}
		/* set page 0 */
		jl_smi_write(phy_0, phy_reg, page_0);
	}

	/* 2. set burst size to 0(1 word)
	 * '0' is same as '1', but would not enable the wide range
	 */
	wide_en = ((size > 1) ? 1: 0);
	burst_ctrl = (size << 2) | (wide_en << 1);
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_2, burst_ctrl);

	/* 3. set register address && issue read operation */
	reg_l_14b = ((reg << 2) & 0x0000fffc) | 3;
	reg_h_16b = (reg >> 14) & 0x000003fff;
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_1, reg_h_16b);
	jl_smi_write(phy_0, REG_NO_OPR_CTRL_0, reg_l_14b);

	/* 4. check operation done & status */
	while (--try) {
		tmp = jl_smi_read(phy_0, REG_NO_OPR_CTRL_0);
		if (!(tmp & 1))
			break;
	}

	if (!try) {
		printf("[%s:%d] read apb register timeout\n", __func__, __LINE__);
		return -2;
	}

	tmp = jl_smi_read(phy_0, REG_NO_OPR_CTRL_2);
	if ((tmp & 1)) {
		printf("[%s:%d] read apb register fail\n", __func__, __LINE__);
		return -3;
	}

	return 0;
}

int jl_apb_reg_read(u32 reg, u32 *val)
{
	return __apb_reg_read(reg, val, 1);
}

int jl_apb_reg_burst_read(u32 reg, u32 *buf, u32 size)
{
	return __apb_reg_read(reg, buf, size);
}

int jl_apb_reg_write(u32 reg, u32 val)
{
	return __apb_reg_write(reg, &val, 1);
}

int jl_apb_reg_burst_write(u32 reg, u32 *buf, u32 size)
{
	return __apb_reg_write(reg, buf, size);
}

int jl51xx_phy_direct_read_ext(u32 phy, u32 page, u32 reg, u32 *pregval)
{
	if ((page > PHY_REG_MASK) || (reg > PHY_REG_MASK))
		return -1;

	*pregval = jl_phy_reg_read_ext(phy, page, reg);

	return 0;
}

int jl51xx_phy_direct_write_ext(u32 phy, u32 page, u32 reg, u32 regval)
{
	if ((page > PHY_REG_MASK) || (reg > PHY_REG_MASK))
		return -1;

	jl_phy_reg_write_ext(phy, page, reg, (u16)(regval & PHY_DATA_MASK));

	return 0;
}

int jl51xx_mac_uctrl_read(u8 port, u32 *pregval)
{
	u8 p2m_port;
	int ret;

	if (pregval == NULL)
		return -1;

	switch (port) {
		case (UTP_PORT0):
		case (UTP_PORT1):
		case (UTP_PORT2):
		case (UTP_PORT3):
		case (UTP_PORT4):
			p2m_port = port;
			break;
		case (8):
			p2m_port = EXT_PORT1;
			break;
		default:
			p2m_port = UNDEF_PORT;
	}

	if ((ret = jl51xx_phy_direct_read_ext(0, MAC_UCTRL_PAGE_ID,
					MAC_UCTRL_REG_ID(p2m_port),
					pregval)) != 0)
		return ret;

	return ret;
}

int jl51xx_mac_uctrl_write(u8 port, u32 regval)
{
	u8 p2m_port;
	int ret;

	if (port >= JL_PORT_MAX) {
		return -1;
	}

	switch (port) {
		case (UTP_PORT0):
		case (UTP_PORT1):
		case (UTP_PORT2):
		case (UTP_PORT3):
		case (UTP_PORT4):
			p2m_port = port;
			break;
		case (8):
			p2m_port = EXT_PORT1;
			break;
		default:
			p2m_port = UNDEF_PORT;
	}

	if ((ret = jl51xx_phy_direct_write_ext(0, MAC_UCTRL_PAGE_ID,
					MAC_UCTRL_REG_ID(p2m_port),
					regval)) != 0)
		return ret;

	return 0;
}

void jl_read_bits(u8 *bit_buf, u64 *bits, u8 start, u8 count)
{
	u64 i_bits = 0;
	u8 i_count = 0;
	u32 i_index = (start) / 8;
	u8 i_mask = 1 << ((start) % 8);

	while (i_count < count) {
		if (bit_buf[i_index] & i_mask)
			i_bits |= (1ULL << i_count);
		if (i_mask == 0x80) {
			i_index++;
			i_mask = 0x01;
		} else
			i_mask <<= 1;
		i_count++;
	}
	*bits = i_bits;
};

void jl_write_bits(u8 *bit_buf, u64 bits, u8 start, u8 count)
{
	u64 i_bits = bits;
	u8 i_count = 0;
	u32 i_index = (start) / 8;
	u8 i_mask = 1 << ((start) % 8);

	while (i_count < count) {
		if ((i_bits >> i_count) & 0x01)
			bit_buf[i_index] |= i_mask;
		else
			bit_buf[i_index] &= ~i_mask;
		if (i_mask == 0x80) {
			i_index++;
			i_mask = 0x01;
		} else
			i_mask <<= 1;
		i_count++;
	}
};

int _led_enable_set(u8 led_group, u8 enable)
{
	int ret;
	u32 pinmux0_val;
	u32 pinmux1_val;
	u32 pinmux0_addr = PINMUX_BASE + PIN_MUX_0_OFFSET;
	u32 pinmux1_addr = PINMUX_BASE + PIN_MUX_1_OFFSET;

	ret = jl_apb_reg_read(pinmux0_addr, &pinmux0_val);
	if (ret)
		return ret;
	ret = jl_apb_reg_read(pinmux1_addr, &pinmux1_val);
	if (ret)
		return ret;


	if (led_group == 0) {
		if (enable)
			SET_BITS(pinmux0_val, 7, 14);
		else
			CLR_BITS(pinmux0_val, 7, 14);

		ret = jl_apb_reg_write(pinmux0_addr, pinmux0_val);
		if (ret)
			return ret;
	}

	if (led_group == 1) {
		if (enable) {
			SET_BIT(pinmux0_val, 15);
			SET_BITS(pinmux1_val, 0, 5);
		} else {
			CLR_BIT(pinmux0_val, 15);
			CLR_BITS(pinmux1_val, 0, 5);
		}

		ret = jl_apb_reg_write(pinmux0_addr, pinmux0_val);
		if (ret)
			return ret;
		ret = jl_apb_reg_write(pinmux1_addr, pinmux1_val);
		if (ret)
			return ret;
	}

	return 0;
}

int _led_reset_blink_set(u8 led_blink)
{
	int ret;
	u32 cpu_reserved0_val;
	u32 rsvd0_addr = APB_FRONTEND_BASE + CPU_RESERVED0_OFFSET;

	ret = jl_apb_reg_read(rsvd0_addr, &cpu_reserved0_val);
	if (ret)
		return ret;

	/* enable/disable led reset blink */
	if (led_blink)
		CLR_BIT(cpu_reserved0_val, 19);
	else
		SET_BIT(cpu_reserved0_val, 19);

	ret = jl_apb_reg_write(rsvd0_addr, cpu_reserved0_val);
	if (ret)
		return ret;

	return ret;
}

int jl51xx_led_enable_set(jl_led_group_t group, u32 portmask)
{
	int ret;
	u32 reg_addr = 0;
	u64 pmask;
	u32 led_cfg[1];
	u8 *preg_val = NULL;
	//u32 flip_pmask = JL_PORTMASK_P2VP(portmask);
	u32 flip_pmask = 0x1f;

	if (portmask >= 0x100) {
		return 0;
	}

	FLIP_BITS(flip_pmask, 0, 7);
	pmask = flip_pmask;

	reg_addr = APB_TOP_BASE + LED_CFG_0_OFFSET;
	preg_val = (u8 *)&led_cfg[0];
	ret = jl_apb_reg_read(reg_addr, &led_cfg[0]);
	if (ret)
		return ret;

	if (group == LED_GROUP0)
		jl_write_bits(preg_val, pmask, 0, 8); //port0-port7
	else if (group == LED_GROUP1)
		jl_write_bits(preg_val, pmask, 8, 8); //port0-port7
	else
		return -1;
	ret = jl_apb_reg_write(reg_addr, led_cfg[0]);
	if (ret)
		return ret;

	return ret;
}

int jl51xx_led_group_active_high_set(jl_led_group_t group, u32 portmask)
{
	int ret;
	u32 reg_addr = 0;
	u64 pmask;
	u32 polarity_cfg[1];
	u8 *preg_val = NULL;
	//u32 flip_pmask = JL_PORTMASK_P2VP(portmask);
	u32 flip_pmask = 0x1f;

	if (portmask >= 0x100) {
		return 0;
	}

	FLIP_BITS(flip_pmask, 0, 7);
	pmask = flip_pmask;

	reg_addr = APB_TOP_BASE + LED_CFG_1_OFFSET;
	preg_val = (u8 *)&polarity_cfg[0];
	ret = jl_apb_reg_read(reg_addr, &polarity_cfg[0]);
	if (ret)
		return ret;

	if (group == LED_GROUP0)
		jl_write_bits(preg_val, pmask, 0, 8); //port0-port7
	else if (group == LED_GROUP1)
		jl_write_bits(preg_val, pmask, 8, 8); //port0-port7
	else
		return -1;
	ret = jl_apb_reg_write(reg_addr, polarity_cfg[0]);
	if (ret)
		return ret;

	return ret;
}

int _force_soft_reset(u8 with_analog,
				u8 with_swc, u8 with_mag,
				u8 led_blink)
{
	int ret;
	u32 cpu_reserved1_val;
	u32 rsvd1_addr = APB_FRONTEND_BASE + CPU_RESERVED1_OFFSET;
	u32 clkgen0_val;
	u32 clkgen0_addr = CLKGEN_BASE + CLKGEN_CTL_0_OFFSET;
	u32 count = 10;
	u32 port_mask = 0;

	_led_reset_blink_set(led_blink);

	ret = jl_apb_reg_read(rsvd1_addr, &cpu_reserved1_val);
	if (ret)
		return ret;

	ret = jl_apb_reg_read(clkgen0_addr, &clkgen0_val);
	if (ret)
		return ret;


	/* fast soft reset, for led blink conern */
	if (!with_analog && !with_swc && !with_mag) {
		SET_BIT(cpu_reserved1_val, 31);
		ret = jl_apb_reg_write(rsvd1_addr, cpu_reserved1_val);
		if (ret)
			return ret;

		goto exit;
	}

	/* XXX rm this line, if led reset status is fixed in fw */
	_led_enable_set(0, 0);

	/* Set switch_pd = 1, switch_pd_with_ana = 1 */
	/* Set disable_ext_eeprom = 1 */
	if (with_analog)
		cpu_reserved1_val |= 0x30000008;
	else
		cpu_reserved1_val |= 0x20000008;
	ret = jl_apb_reg_write(rsvd1_addr, cpu_reserved1_val);
	if (ret)
		return ret;

	/* Set MAG_RSTN = 0, CORE_6P25 = 0 */
	if (with_swc)
		CLR_BIT(clkgen0_val, 4);
	if (with_mag)
		CLR_BIT(clkgen0_val, 7);
	ret = jl_apb_reg_write(clkgen0_addr, clkgen0_val);
	if (ret)
		return ret;

	udelay(10000);
	/* XXX rm this line, if led reset status is fixed in fw */
	_led_enable_set(0, 1);

	ret = jl_apb_reg_read(rsvd1_addr, &cpu_reserved1_val);
	if (ret)
		return ret;

	/* Set switch_pd = 0, switch_pd_witch_ana = 0 */
	CLR_BITS(cpu_reserved1_val, 28, 29);
	ret = jl_apb_reg_write(rsvd1_addr, cpu_reserved1_val);
	if (ret)
		return ret;

exit:
	if (!led_blink) {
		while (count--) {
			udelay(1000);

			/*read ADDR_LOAD_DATA4,capture signal FLAG_PROCESS for enabling leds */
			ret = jl51xx_phy_direct_read_ext(0, 7, 0x15, &port_mask);
			if (ret)
				return ret;

			if (port_mask == 0xb) {
				ret = jl51xx_led_enable_set(LED_GROUP0, 0xff);
				if (ret)
					return ret;

				break;
			}
		};
	}

	return 0;
}

int jl51xx_cpu_allow_lldp_enable_set(bool enable)
{
	int ret = 0;
	u32 val = 0;
	u32 portmask_offset = 5;
	u32 reg_addr = SWCORE_BASE + LLDP_CONFIGURATION_OFFSET + portmask_offset;

	ret = jl_apb_reg_read(reg_addr, &val);
	if (ret)
		return ret;

	if (enable)
		val |=  0x1ff;
	else if (enable == false)
		val &= ~(0x1ff);
	else
		return -1;

	ret = jl_apb_reg_write(reg_addr, val);
	if (ret)
		return ret;

	return 0;
}

int jl_switch_init(void)
{
	u64 mac_is_link = 1;
	u64 allow_bpdu, disable, answer, tickcnt, tickindex;
	u32 rsvd0_addr = APB_FRONTEND_BASE + CPU_RESERVED0_OFFSET;
	u32 cpu_reserved0_val;
	u32 mac_user_ctrl_val[1];
	u32 send2cpu_val[2];
	u32 disable_cpu_tag_val[1];
	u32 vid2vtab_tcam_answer_val[1];
	u32 time2age_val[2];
	u32 reg_addr;
	u8 *preg_val = NULL;
	int ret;

	_led_enable_set(0, 0);
	_led_reset_blink_set(0);

	ret = jl_apb_reg_read(rsvd0_addr, &cpu_reserved0_val);
	if (ret)
		return ret;

	/* enable mac9 */
	cpu_reserved0_val |= BIT(15);
	/* RMII */
	cpu_reserved0_val |= BIT(12);
	/* PHY_MODE: clock_out=1 */
	cpu_reserved0_val |= BIT(13);

	ret = jl_apb_reg_write(rsvd0_addr, cpu_reserved0_val);
	if (ret)
		return ret;

	ret = _force_soft_reset(0, 0, 0, 0);
	jl51xx_led_group_active_high_set(LED_GROUP0, 0x1f);

	_led_enable_set(0, 1);

	ret = jl51xx_mac_uctrl_read(EXT_PORT0, &mac_user_ctrl_val[0]);
	if (ret)
		return ret;

	preg_val = (u8 *)&mac_user_ctrl_val[0];
	jl_write_bits(preg_val, mac_is_link, 0, 1);

	ret = jl51xx_mac_uctrl_write(EXT_PORT0, mac_user_ctrl_val[0]);
	if (ret)
		return ret;

	ret = jl51xx_cpu_allow_lldp_enable_set(false);
	if (ret)
		return ret;

	do {
		reg_addr  = SWCORE_BASE + SEND_TO_CPU_OFFSET;
		preg_val = (u8 *)&send2cpu_val[0];
		ret = jl_apb_reg_burst_read(reg_addr, &send2cpu_val[0], 2);
		if (ret)
			return ret;

		allow_bpdu = 0;
		jl_write_bits(preg_val, allow_bpdu, 0, 1);

		ret = jl_apb_reg_burst_write(reg_addr, &send2cpu_val[0], 2);
		if (ret)
			return ret;
	} while (0);

	do {
		reg_addr  = SWCORE_BASE + DISABLE_CPU_TAG_ON_CPU_PORT_OFFSET;
		preg_val = (u8 *)&disable_cpu_tag_val[0];
		ret = jl_apb_reg_burst_read(reg_addr, &disable_cpu_tag_val[0], 1);
		if (ret)
			return ret;

		disable = 1;
		jl_write_bits(preg_val, disable, 0, 1);

		ret = jl_apb_reg_burst_write(reg_addr, &disable_cpu_tag_val[0], 1);
		if (ret)
			return ret;
	} while (0);

	/* disable vlan by default */
	do {
		reg_addr = SWCORE_BASE + VID_TO_VLAN_TABLE_TCAM_ANSWER_OFFSET + VLAN_IDX_DROP * 1;
		preg_val = (u8 *)&vid2vtab_tcam_answer_val[0];
		ret = jl_apb_reg_burst_read(reg_addr, &vid2vtab_tcam_answer_val[0], 1);
		if (ret)
			return ret;

		answer = VLAN_IDX_DEFAULT;
		jl_write_bits(preg_val, answer, 0, 4);

		ret = jl_apb_reg_burst_write(reg_addr, &vid2vtab_tcam_answer_val[0], 1);
		if (ret)
			return ret;
	} while (0);

	/* set time to age tickCnt:0x7735940 by default */
	do {
		reg_addr = SWCORE_BASE + TIME_TO_AGE_OFFSET;
		preg_val = (u8 *)&time2age_val[0];
		ret = jl_apb_reg_burst_read(reg_addr, &time2age_val[0], 2);
		if (ret)
			return ret;

		tickcnt = DEFAULT_TICK_NUM;
		tickindex = DEFAULT_TICK_ID;
		jl_write_bits(preg_val, tickcnt, 0, 32);
		jl_write_bits(preg_val, tickindex, 32, 3);
		ret = jl_apb_reg_burst_write(reg_addr, &time2age_val[0], 2);
		if (ret)
			return ret;
	} while (0);

	return 0;
}

int jl_get_chip_id(void)
{
	u32 reg_addr = APB_FRONTEND_BASE + CPU_RESERVED0_OFFSET;
	u32 reg_val[1] = {0};
	u8 *preg_val = (u8 *)&reg_val[0];
	u64 tmp = 0;
	u32 chip_id;
	int ret;

	ret = jl_apb_reg_burst_read(reg_addr, &reg_val[0], 1);
	if (ret)
		return ret;

	jl_read_bits(preg_val, &tmp, 28, 4);
	chip_id = tmp;
	switch (chip_id) {
		case (CHIP_ID_JL5104):
		case (CHIP_ID_JL5105):
		case (CHIP_ID_JL5106):
		case (CHIP_ID_JL5108):
		case (CHIP_ID_JL5109):
		case (CHIP_ID_JL5110):
			break;
		default:
			return -1;
	}

	return chip_id;
}
