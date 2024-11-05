#ifndef __SF_JL51XX_API_H__
#define __SF_JL51XX_API_H__

#include "sfa18_gmac.h"

#define APB_REG_BUSRT_SIZE_PG0_MAX  6
#define APB_REG_BUSRT_SIZE_MAX  8

#define REG_NO_OPR_CTRL_0	16
#define REG_NO_OPR_CTRL_1	17
#define REG_NO_OPR_CTRL_2	18
#define REG_NO_OPR_DATA0_L	19
#define REG_NO_OPR_DATA6_L	16

#define PHY_DATA_MASK               ((u32)0x0000FFFFU)
#define PHY_REG_MASK                ((u8)0x1FU)

#define MAC0_USER_CTRL_OFFSET					((u32)0x00000070U)
#define MAC_UCTRL_PAGE_ID						(MAC0_USER_CTRL_OFFSET >> 5)
#define MAC_UCTRL_REG_ID(id)					((MAC0_USER_CTRL_OFFSET & 0x1F) + id)

#define APB_FRONTEND_BASE                        ((u32)0x00500000U)
#define SWCORE_BASE                              ((u32)0x00100000U)
#define CPU_RESERVED0_OFFSET                     ((u32)0x00000006U)
#define DISABLE_CPU_TAG_ON_CPU_PORT_OFFSET       ((u32)0x00002D8CU)
#define VID_TO_VLAN_TABLE_TCAM_ANSWER_OFFSET     ((u32)0x00002ACCU)
#define TIME_TO_AGE_OFFSET                       ((u32)0x00000078U)
#define PINMUX_BASE                              ((u32)0x01200000U)
#define PIN_MUX_0_OFFSET                         ((u32)0x00000000U)
#define PIN_MUX_1_OFFSET                         ((u32)0x00000001U)
#define CPU_RESERVED1_OFFSET                     ((u32)0x00000007U)
#define CLKGEN_BASE                              ((u32)0x00800000U)
#define CLKGEN_CTL_0_OFFSET                      ((u32)0x00000000U)
#define LLDP_CONFIGURATION_OFFSET                ((u32)0x00002AEEU)
#define SEND_TO_CPU_OFFSET                       ((u32)0x0000294EU)
#define APB_TOP_BASE                             ((u32)0x00400000U)
#define LED_CFG_0_OFFSET                         ((u32)0x00000000U)
#define LED_CFG_1_OFFSET                         ((u32)0x00000001U)

#define DEFAULT_TICK_NUM        0x7735940UL /**< Number of ticks between aging interval */
#define DEFAULT_TICK_ID         0UL /**< DEFAULT_TICK_ID */
#define VLAN_IDX_DEFAULT        0UL /**< VLAN_IDX_DEFAULT */
#define VLAN_IDX_DROP           15UL /**< VLAN_IDX_DROP */

#define BITMASK_MAX                         0xFFFFFFFFUL /**< BITMASK_MAX */
#define BITOFS_MAX                          31 /**< BITOFS_MAX */

#define BITS(start, end)                    ((BITMASK_MAX) << (start) & (BITMASK_MAX) >> (BITOFS_MAX - (end))) /**< BITS(start, end) */
#define GET_BIT(regval, bitofs)             (((regval) >> (bitofs)) & 0x1) /**< GET_BIT(regval, bitofs) */
#define SET_BIT(regval, bitofs)             ((regval) |= BIT(bitofs)) /**< SET_BIT(regval, bitofs) */
#define CLR_BIT(regval, bitofs)             ((regval) &= (~BIT(bitofs))) /**< CLR_BIT(regval, bitofs) */
#define FLIP_BIT(regval, bitofs)            ((regval) ^= BIT(bitofs)) /**< FLIP_BIT(regval, bitofs)  */

#define GET_BITS(regval, start, end)        (((regval) & BITS((start), (end))) >> (start)) /**< GET_BITS(regval, start, end) */
#define SET_BITS(regval, start, end)        ((regval) |= BITS((start), (end))) /**< SET_BITS(regval, start, end) */
#define CLR_BITS(regval, start, end)        ((regval) &= (~BITS((start), (end)))) /**< CLR_BITS(regval, start, end) */
#define FLIP_BITS(regval, start, end)       ((regval) ^= BITS((start), (end))) /**< FLIP_BITS(regval, start, end) */

typedef enum jl_led_group_e {
    LED_GROUP0 = 0,/**< 0 */
    LED_GROUP1,/**< 1 */
    LED_GROUP_END,/**< Invalid */
} jl_led_group_t;

typedef enum jl_port_e {
	UTP_PORT0 = 0,
	UTP_PORT1,
	UTP_PORT2,
	UTP_PORT3,
	UTP_PORT4,
	UTP_PORT5,
	UTP_PORT6,
	UTP_PORT7,

	EXT_PORT0 = 8,
	EXT_PORT1,

	JL_PORT_MAX = 10,
	UNDEF_PORT = 0xff
} jl_port_t;

enum chip_id_e {
	CHIP_ID_JL5104 = 0,/**< JL5104 */
	CHIP_ID_JL5105,/**< JL5105 */
	CHIP_ID_JL5106,/**< JL5106 */
	CHIP_ID_JL5108,/**< JL5108 */
	CHIP_ID_JL5109,/**< JL5109 */
	CHIP_ID_JL5110,/**< JL5110 */
	CHIP_ID_END/**< Invalid */
};

int jl_get_chip_id(void);
int jl_switch_init(void);

#endif // __SF_JL51XX_API_H__
