/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#ifndef __CAL_BOARD_PROFILE_H__
#define __CAL_BOARD_PROFILE_H__

/*
 * Include Files
 */
#include "yt_types.h"
#include "chipdef.h"
#include "phy_chipdef.h"
/*
 * Symbol Definition
 */

/* profile identifier */
#define BOARD_MAX_PROFILE				5	/* how many profiles the code would use */
#define BOARD_PROFILE_NAME_MAXLEN		32	/* profile name max length */
#define BOARD_NONE						(0xFF)
#define INVALID_ID						(0xFF)

/*
 * Data Type Definition
 */
typedef enum yt_board_profile_id_e
{
    BOARD_ID_YT9215RB	= 92153,
    BOARD_ID_YT9215S	= 92152,
    BOARD_ID_YT9215SC	= 92151,
    BOARD_ID_YT9218NB	= 92181,
    BOARD_ID_YT9218MB	= 92180,
}yt_board_profile_id_t;

/* port attribute */
typedef enum yt_port_attri_e
{
    PORT_ATTR_ETH		= 1,	/* Ethernet port */
    PORT_ATTR_CASCADE	= 2,	/* Cadcade port (the interconnection between two chips cadcaded on a board) */
    PORT_ATTR_INT_CPU,			/* Internal CPU port */
/*    PORT_ATTR_EXT_CPU,	*/		/* EXT CPU port through XMII or SGMII */
    PORT_ATTR_EXT_MII,
    PORT_ATTR_EXT_RMII_MAC,
    PORT_ATTR_EXT_RMII_PHY,
    PORT_ATTR_EXT_RGMII,
    PORT_ATTR_EXT_SERDES,
}yt_port_attri_t;

/* port ethernet type */
typedef enum yt_port_ethType_e
{
    ETH_TYPE_FE		= 0x01,/* Fast ethernet port */
    ETH_TYPE_GE		= 0x02,/* Giga ethernet port */
    ETH_TYPE_GE25	= 0x04,/* 2.5 Giga ethernet port */
    ETH_TYPE_GE50	= 0x08,/* 5 Giga ethernet port */
}yt_port_ethType_t;

/* switch core register access method (from main CPU) */
typedef enum yt_swAccMethod_e
{
    SWCHIP_ACC_NONE = 0,            /* not able to access register */
    SWCHIP_ACC_SPI,                     /* the switch core registers are accessed through SPI interface by main CPU */
    SWCHIP_ACC_I2C,                     /* the switch core registers are accessed through I2C interface by main CPU */
    SWCHIP_ACC_SMI,                     /* the switch core registers are accessed through SMI interface by main CPU */
}yt_swAccMethod_t;

/* serdes mode */
typedef enum yt_serdes_mode_e
{
    SERDES_MODE_NONE = 0,
    SERDES_MODE_DISABLE,
    SERDES_MODE_SGMII_MAC,
    SERDES_MODE_SGMII_PHY,
    SERDES_MODE_1000BX,
    SERDES_MODE_100B_FX,
    SERDES_MODE_2500BX,
    SERDES_MODE_SGMII_AS,
} yt_serdes_mode_t;

/* smi id */
typedef enum yt_smi_type_e
{
	YT_SMI_NONE = 0xFF,	/*invaild*/
	YT_SMI_INT = 0,	/* map to sw internal smi */
	YT_SMI_EXT,	/* map to sw ext smi */
	YT_SMI_EXT_CPU,	/* ext cpu smi */
}yt_smi_type_t;

typedef enum yt_i2c_mode_e
{
    YT_I2C_STD_MSB = 1,
    YT_I2C_STD_LSB,
    YT_I2C_SIMPLE,
}yt_i2c_mode_t;

/* Control Interface (CRTLIF)--start */
/* SMI */
typedef uint32_t (*smi_cl22_write)(uint8_t phyAddr, uint8_t regAddr, uint16_t regValue);
typedef uint32_t (*smi_cl22_read)(uint8_t phyAddr, uint8_t regAddr, uint16_t *pRegValue);
/* SPI */
typedef int32_t (*spi_cb_write)(uint8_t *regAddr, uint32_t addrLen, uint8_t *regValue, uint32_t valueLen);
typedef int32_t (*spi_cb_read)(uint8_t *regAddr, uint32_t addrLen, uint8_t *regValue, uint32_t valueLen);
/* I2C */
typedef int32_t (*i2c_cb_write)(uint8_t *regAddr, uint32_t addrLen, uint8_t *regValue, uint32_t valueLen);
typedef int32_t (*i2c_cb_read)(uint8_t *regAddr, uint32_t addrLen, uint8_t *regValue, uint32_t valueLen);

typedef struct yt_smi_desc_s
{
    uint8_t switchId;
    uint8_t phyAddr;
    smi_cl22_write smi_write;
    smi_cl22_read smi_read;
}yt_smi_desc_t;

typedef struct yt_spi_desc_s
{
    spi_cb_write spi_write;
    spi_cb_read spi_read;
}yt_spi_desc_t;

typedef struct yt_i2c_desc_s
{
    yt_i2c_mode_t i2c_mode;
    i2c_cb_write i2c_write;
    i2c_cb_read i2c_read;
}yt_i2c_desc_t;

typedef struct yt_switch_access_s
{
    yt_swAccMethod_t swreg_acc_method;
    union
    {
        yt_smi_desc_t smi_controller;
        yt_spi_desc_t spi_controller;
        yt_i2c_desc_t i2c_controller;
    }controller;
}yt_switch_access_t;
/* Control Interface (CRTLIF)--end */

typedef struct board_profile_identify_s
{
    uint32_t	id;
    char		name[BOARD_PROFILE_NAME_MAXLEN];
}board_profile_identify_t;

/* port description */
typedef struct yt_portDescp_s
{
    uint8_t   mac_id;               /* Physical MAC ID */
    yt_port_attri_t   attribute;            /* port attribute,yt_port_attri_t */
    uint8_t   phy_index;          /* phy index number or BOARD_NONE */
    uint8_t   phy_addr;           /* phy address */
    uint8_t   serdes_index;     /* serdes index number, or BOARD_NONE */
    yt_port_ethType_t   ethtype;              /* port ethernet type,yt_port_ethType_t */
    yt_port_medium_t   medium;              /* port media type, yt_port_medium_t*/
    yt_smi_type_t   smi_index;          /* index of SMI interface */
}yt_portDescp_t;

/* serdes description */
typedef struct yt_serdesDescp_s
{
    uint8_t   serdes_id;	/* Physical Serdes ID */
    yt_serdes_mode_t   mode;			/* yt_serdesMode_t */
}yt_serdesDescp_t;

/* phy description */
typedef struct yt_phyDescp_s
{
    uint8_t phy_index;
    uint8_t chip_model;	/* yt_phy_chip_model_t */
    uint8_t start_mac_id;
    uint8_t phy_max;
}yt_phyDescp_t;

/* LED description */
typedef enum yt_led_mode_e
{
    LED_MODE_PARALLEL = 0,
    LED_MODE_SCAN,          /* 9215 no support */
    LED_MODE_SERIAL,
    LED_MODE_NUM
}yt_led_mode_t;

/* bit num on LED_DATA */
typedef enum yt_sled_dataNum_e
{
    SLED_DATANUM_5 = 0,
    SLED_DATANUM_7,
    SLED_DATANUM_10,
    SLED_DATANUM_14,
    SLED_DATANUM_15,
    SLED_DATANUM_21,
    SLED_DATANUM_MAX
}yt_sled_dataNum_t;

typedef enum yt_sled_activeMode_e {
    LED_SERIAL_ACTIVE_MODE_HIGH = 0,
    LED_SERIAL_ACTIVE_MODE_LOW
}yt_sled_activeMode_t;

/* valid only for serial mode */
typedef struct yt_sled_remapInfo_s {
    uint8_t port    :6;
    uint8_t ledId   :2;
}yt_sled_remapInfo_t;

typedef struct yt_sled_param_s
{
    yt_sled_activeMode_t activeMode;
    yt_sled_dataNum_t dataNum;
    uint8_t remapInfoNum;
    const yt_sled_remapInfo_t *pRemapInfo;
}yt_sled_param_t;

typedef struct yt_ledDescp_s
{
    yt_led_mode_t ledMode;
    yt_sled_param_t *pSledParam;
}yt_ledDescp_t;

/* Switch chip description structure */
typedef struct yt_swDescp_s
{
    yt_switch_chip_id_t chip_id;
    yt_switch_chip_model_t chip_model;	/* chip model */
    yt_switch_access_t sw_access;   /* the switch registers accessor,only for chip mode */
    uint8_t port_num;
    const yt_portDescp_t *pPortDescp[YT_MAX_PORT_PER_UNIT];  /* port */
    const yt_serdesDescp_t *pSerdesDescp[YT_MAX_SERDES_PER_UNIT];  /* serdes */
    const yt_phyDescp_t *pPhyDescp[YT_MAX_PHY_PER_UNIT];    /* extend phy */
    const yt_ledDescp_t *pLEDDescp; /* LED */
    const yt_swchip_cap_t *pChipCap;  /*capability of every unit*/
    const yt_swchip_if_info_t *pChipIfInfo; /* interface info */
}yt_swDescp_t;

typedef struct yt_socDescp_s
{
    uint8_t	swDescp_index;      /* which swDescp the MCU belong to */
}yt_socDescp_t;

/* board profile structure */
typedef struct yt_hwProfile_info_s
{
    const board_profile_identify_t	*pIdentifier;	/* identifier of this board profile */
    const yt_socDescp_t	*pSoc;		/* SoC (CPU/Peripheral) description */
    uint8_t				switch_count; /* number of switch chip on board */
    yt_swDescp_t	*pSwDescp[YT_MAX_UNIT];    /* switch description */
    yt_port_mask_t		portmask[YT_MAX_UNIT];  /*yt port mask of every unit*/
    yt_port_mask_t		macmask[YT_MAX_UNIT];  /*mac id mask of every unit*/
    yt_port_mask_t		allportmask[YT_MAX_UNIT];  /*yt port mask of every unit, include CPUPORT*/
}yt_hwProfile_info_t;

/* profile identify and init data */
typedef struct yt_hwProfile_s
{
    const board_profile_identify_t		*pIdentifier;	/* identifier of this board profile */
    uint32_t (*profile_init)(yt_hwProfile_info_t *);	/* user defined api to init profile data according to user defined */
}yt_hwProfile_t;


extern yt_hwProfile_info_t	gBoardInfo;
extern const yt_swDescp_t	*gpSwitchUnit[YT_MAX_UNIT];

/*
 * Function Declaration
 */
uint32_t cal_board_profile_init(void);


#endif /*end of __CAL_BOARD_PROFILE_H__*/
