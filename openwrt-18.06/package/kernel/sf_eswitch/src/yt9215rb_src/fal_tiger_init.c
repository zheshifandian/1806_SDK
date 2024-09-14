/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

/*
 * Include Files
 */
#include "fal_cmm.h"
#include "fal_tiger_init.h"
#include "fal_tiger_port.h"
#include "fal_tiger_led.h"
#include "osal_print.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"
#include "fal_tiger_qos.h"

static yt_ret_t fal_tiger_patch_init(yt_unit_t unit);
static yt_ret_t fal_tiger_port_interface_init(yt_unit_t unit);
static yt_ret_t fal_tiger_led_init(yt_unit_t unit);

yt_ret_t fal_tiger_init(yt_unit_t unit)
{
    fal_tiger_patch_init(unit);
    fal_tiger_led_init(unit);
    fal_tiger_port_interface_init(unit);
    return CMM_ERR_OK;
}

static yt_ret_t fal_tiger_patch_qos_init(yt_unit_t unit)
{
    uint32_t port;
    uint32_t qid;
    uint32_t regAddr;
    int32_t ret = 0;

    for (port = 0; port < FAL_MAX_PORT_NUM; port++)
    {
        for (qid = 0; qid < CAL_MAX_UCAST_QUEUE_NUM(unit); qid++)
        {
            regAddr = QOS_FORCEAC_UCASTQUE_REG(unit, port, qid);
            CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr, 0x80c060c), ret);
            CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr + 4, 0x9000000), ret);
        }

        for (qid = 0; qid < CAL_MAX_MCAST_QUEUE_NUM(unit); qid++)
        {
            regAddr = QOS_FORCEAC_MCASTQUE_REG(unit, port, qid);
            CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr, 0x1400060c), ret);
        }
    }

    return CMM_ERR_OK;
}

static yt_ret_t fal_tiger_patch_init(yt_unit_t unit)
{
    uint8_t i;
    uint32_t regData;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    global_ctrl1_t gEntry;

    uint8_t intTuneVal[4] = {0x07, 0x00, 0x00, 0x00};

#ifdef MEM_MODE_CMODEL
    return CMM_ERR_OK;
#endif
    /* flow control */
    for(i = 0; i < FAL_MAX_PORT_NUM; i++)
    {
        HAL_MEM_DIRECT_WRITE(unit, 0x281000+i*8, 0x8040284B);
        HAL_MEM_DIRECT_WRITE(unit, 0x281004+i*8, 0x26F1B);
    }
    for(i = 0; i < 4; i++)
    {
        HAL_MEM_DIRECT_WRITE(unit, 0x2801D0+i*4, 0x14A);
        HAL_MEM_DIRECT_WRITE(unit, 0x180904+i*4, intTuneVal[i]);
    }

#if defined(BOARD_YT9215RB_YT8531_DEMO) || defined(BOARD_YT9215S_YT8531_FIB_DEMO)
    /*select mdio grp1 pin*/
    HAL_MEM_DIRECT_READ(unit, 0x80358, &regData);
    regData |= (1 << 2);
    HAL_MEM_DIRECT_WRITE(unit, 0x80358, regData);
#endif
    /*for xmii on low temp*/
    HAL_MEM_DIRECT_READ(unit, 0x80400, &regData);
    regData |= (1 << 11);
    HAL_MEM_DIRECT_WRITE(unit, 0x80400, regData);

    /*disable internal cpu port learn fdb*/
    HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, FAL_INTERNAL_CPU_MACID, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_DISABLEf, &l2_learn_per_port_ctrl, YT_ENABLE);
    HAL_TBL_REG_WRITE(unit, L2_LEARN_PER_PORT_CTRLNm, FAL_INTERNAL_CPU_MACID, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl);

    HAL_TBL_REG_READ(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &gEntry);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_ACL_ENf, &gEntry, YT_ENABLE);
    HAL_TBL_REG_WRITE(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &gEntry);

    /*
    * disable sds intf,will be enabled when config sds mode
    * set to mdio access mode
    */
    HAL_MEM_DIRECT_READ(unit, CHIP_INTERFACE_CTRL_REG, &regData);
    regData &= ~(0x43<<0);
    HAL_MEM_DIRECT_WRITE(unit, CHIP_INTERFACE_CTRL_REG, regData);

    fal_tiger_patch_qos_init(unit);
#ifdef PORT_INCLUDED
    fal_tiger_port_init(unit);
#endif

    return CMM_ERR_OK;
}

/* config interfaces according to board profile */
static yt_ret_t fal_tiger_port_interface_init(yt_unit_t unit)
{
#ifdef PORT_INCLUDED
    yt_port_t port;
    yt_port_attri_t attr;
    yt_serdes_mode_t sdsMode;
    yt_extif_mode_t extifMode;
    uint8_t extif_id = 0;
#endif

#ifdef PORT_INCLUDED
    for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
    {
        attr = CAL_PORT_ATTRIBUTE(unit, port);
        switch(attr)
        {
            case PORT_ATTR_EXT_RGMII:
                fal_tiger_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RGMII);
                break;
            case PORT_ATTR_EXT_RMII_MAC:
                fal_tiger_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RMII_MAC);
                break;
            case PORT_ATTR_EXT_RMII_PHY:
                fal_tiger_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RMII_PHY);
                break;
            case PORT_ATTR_EXT_MII:
                fal_tiger_port_extif_mode_set(unit, port, YT_EXTIF_MODE_MII);
                break;
            case PORT_ATTR_ETH:
                /*ext port*/
                if(CAL_PORT_TYPE_EXT == CAL_YTP_PORT_TYPE(unit, port))
                {
                    /*config serdes*/
                    if(CAL_IS_SERDES(unit, port))
                    {
                        sdsMode = CAL_SERDES_MODE(unit, port);
                        if(CMM_ERR_OK == CMM_CALSdsMode_to_YTExtMode(sdsMode, &extifMode))
                        {
                            fal_tiger_port_extif_mode_set(unit, port, extifMode);
                        }
                        /*TODO:check if sg phy,config phy if necessary*/
                    }

                    if(CAL_IS_PHY_PORT(unit, port))
                    {
                        fal_tiger_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RGMII);
                        if(CAL_IS_YTPHY(unit, port))
                        {
                            extif_id = CAL_YTP_TO_EXTPORT(unit, port);
                            HAL_MEM_DIRECT_WRITE(unit, 0x8035C+4*extif_id, TRUE);/*enable rgmii AN*/
                        }
                        else
                        {
                            /*TODO:set mac force and start polling*/
                        }
                    }
                }
                /*config combo port*/
                if(CAL_IS_COMBO_PORT(unit, port))
                {
                    /*phy combo mode*/
                    if(CAL_IS_PHY_PORT(unit, port))
                    {
                        if(PORT_MEDI_COMBO_FIBER == CAL_PORT_MEDIUM(unit, port))
                        {
                            fal_tiger_port_phyCombo_mode_set(unit, port, COMBO_MODE_FIBER_FIRST);
                        }
                        else if(PORT_MEDI_COMBO_COPPER == CAL_PORT_MEDIUM(unit, port))
                        {
                            fal_tiger_port_phyCombo_mode_set(unit, port, COMBO_MODE_COPPER_FIRST);
                        }
                    }
                    /*TODO:other*/
                }
                /*enable port*/
                if(CAL_IS_PHY_PORT(unit, port))
                {
                    fal_tiger_port_enable_set(unit, port, YT_ENABLE);
                }
                break;
            case PORT_ATTR_INT_CPU:
            case PORT_ATTR_CASCADE:
                break;
            default:
                break;
        }
    }
#else
    CMM_UNUSED_PARAM(unit);
#endif
    return CMM_ERR_OK;
}

yt_ret_t CMM_CALSdsMode_to_YTExtMode(yt_serdes_mode_t sdsMode, yt_extif_mode_t *pExtifMode)
{
    switch(sdsMode)
    {
        case SERDES_MODE_DISABLE:
            *pExtifMode = YT_EXTIF_MODE_SG_DISABLE;
            break;
        case SERDES_MODE_SGMII_MAC:
            *pExtifMode = YT_EXTIF_MODE_SG_MAC;
            break;
        case SERDES_MODE_SGMII_PHY:
            *pExtifMode = YT_EXTIF_MODE_SG_PHY;
            break;
        case SERDES_MODE_1000BX:
            *pExtifMode = YT_EXTIF_MODE_FIB_1000;
            break;
        case SERDES_MODE_100B_FX:
            *pExtifMode = YT_EXTIF_MODE_FIB_100;
            break;
        case SERDES_MODE_2500BX:
            *pExtifMode = YT_EXTIF_MODE_BX2500;
            break;
        case SERDES_MODE_SGMII_AS:
            *pExtifMode = YT_EXTIF_MODE_SGFIB_AS;
            break;
        default:
            return CMM_ERR_FAIL;
    }

    return CMM_ERR_OK;
}

/**
 * @internal      fal_tiger_led_init
 * @endinternal
 *
 * @brief         init LED
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
static yt_ret_t  fal_tiger_led_init(yt_unit_t unit)
{
#ifdef LED_INCLUDED
    uint32_t ret;
    uint32_t regAddr;
    uint32_t regVal;
    uint32_t port;
    uint8_t index;
    uint8_t portNum;
    uint8_t ledNum;
    yt_sled_dataNum_t dataNum;
    yt_led_remapping_t dstInfo;
    const yt_sled_remapInfo_t *sInfo = SLED_PARAM(unit)->pRemapInfo;

    if (NULL == LEDDSCP_ON_UNIT(unit))
    {
        return CMM_ERR_NULL_POINT;
    }

    fal_tiger_led_mode_set(unit, LED_MODE(unit));

    /* serial mode */
    if (LED_MODE_SERIAL == LED_MODE(unit))
    {
        if (NULL == SLED_PARAM(unit))
        {
            return CMM_ERR_NULL_POINT;
        }

        /* active mode */
        fal_tiger_led_serial_activeMode_set(unit, SLED_PARAM(unit)->activeMode);

        dataNum = SLED_PARAM(unit)->dataNum;
        switch(dataNum)
        {
            case SLED_DATANUM_5:
                portNum = 5;
                ledNum = 1;
                break;

            case SLED_DATANUM_7:
                portNum = 7;
                ledNum = 1;
                break;

           case SLED_DATANUM_10:
                portNum = 5;
                ledNum = 2;
                break;

            case SLED_DATANUM_14:
                portNum = 7;
                ledNum = 2;
                break;

            case SLED_DATANUM_15:
                portNum = 5;
                ledNum = 3;
                break;

            case SLED_DATANUM_21:
                portNum = 7;
                ledNum = 3;
                break;

            default:
                return CMM_ERR_NOT_SUPPORT;
        }

        /* serial port num-- start */
        ret = HAL_MEM_DIRECT_READ(unit, LED_GLB_CTRL, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }
        regVal &= 0xfffe1fff;
        regVal |= ((portNum & 0xf) << 13);
        HAL_MEM_DIRECT_WRITE(unit, LED_GLB_CTRL, regVal);
        /* serial port num-- end */

        /* serial pin num-- start */
        ret = HAL_MEM_DIRECT_READ(unit, LED_SERIAL_CTRL, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }
        regVal &= 0xfffffffc;
        regVal |= ((ledNum - 1) & 0x3);
        HAL_MEM_DIRECT_WRITE(unit, LED_SERIAL_CTRL, regVal);
        /* serial pin num-- end */

        /* remaping */
        for (index = 0; index < SLED_PARAM(unit)->remapInfoNum; index++)
        {
            sInfo = SLED_PARAM(unit)->pRemapInfo + index;
            dstInfo.port = sInfo->port;
            dstInfo.ledId = sInfo->ledId;
            fal_tiger_led_serial_remapping_set(unit, index, dstInfo);
        }
    }

    /* preset disable_link_try bit of LED0 action(exclude CPU_PORT) */
    for (port = 0; port < FAL_MAX_PORT_NUM - 1; port++)
    {
        regAddr = LED_CTRL_0_BASE + port * 4;
        ret = HAL_MEM_DIRECT_READ(unit, regAddr, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }

        regVal |= 0x20000;
        HAL_MEM_DIRECT_WRITE(unit, regAddr, regVal);
    }
#else
    CMM_UNUSED_PARAM(unit);
#endif

    return CMM_ERR_OK;
}
