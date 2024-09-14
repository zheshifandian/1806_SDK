/*
 * Include Files
 */
#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "osal_math.h"
#include "fal_tiger_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "hal_mem.h"
#include "yt_rate.h"
#include "fal_tiger_rate.h"

uint8_t g_cycle_time[YT_UNIT_NUM];

static uint32_t rate_tran_usr2reg(uint8_t rate_mode, uint8_t token_level, uint32_t time_slot, uint32_t rate)
{
    uint32_t retRate;
    uint32_t divisor;
    uint64 dividend;

    divisor = 1000000000;
    if (rate_mode == RATE_MODE_BYTE)
    {
        if (0 < (11 - 2 * token_level))
        {
            dividend = ((uint64)rate * time_slot) << (11 - 2 * token_level);
        }
        else
        {
            dividend = ((uint64)rate * time_slot) >> (2 * token_level - 11);
        }
        
    }
    else
    {
        if (0 < (21 - 2 * token_level))
        {
            dividend = ((uint64)rate * time_slot) << (21 - 2 * token_level);
        }
        else
        {
            dividend = ((uint64)rate * time_slot) >> (2 * token_level - 21);
        }
    }
    do_div64(dividend, divisor);
    retRate = (uint32_t)dividend;
    
    return retRate;
}

static uint32_t rate_tran_reg2usr(uint8_t rate_mode, uint8_t token_level, uint32_t time_slot, uint32_t rate)
{
    uint32_t retRate;
    uint32_t divisor;
    uint64 dividend;

    divisor = time_slot;
    if (rate_mode == RATE_MODE_BYTE)
    {
        if (0 < (11 - 2 * token_level))
        {
            dividend = ((uint64)rate * 1000000000) >> (11 - 2 * token_level);
        }
        else
        {
            dividend = ((uint64)rate * 1000000000) << (2 * token_level - 11);
        }
        
    }
    else
    {
        if (0 < (21 - 2 * token_level))
        {
            dividend = ((uint64)rate * 1000000000) >> (21 - 2 * token_level);
        }
        else
        {
            dividend = ((uint64)rate * 1000000000) << (2 * token_level - 21);
        }
    }
    do_div64(dividend, divisor);
    retRate = (uint32_t)dividend;
    
    return retRate;
}

    
yt_ret_t fal_tiger_rate_init(yt_unit_t unit)
{
    global_ctrl1_t global_ctrl_tbl;
    meter_timeslot_t meter_timeslot;
    psch_shp_slot_time_cfg_t psch_time_slot;
    qsch_shp_slot_time_cfg_t qsch_time_slot;
    yt_macid_t macid;
    meter_config_tbl_t meter_config_tbl;
    psch_shp_cfg_tbl_t psch_config_tbl;
    qsch_shp_cfg_tbl_t qsch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t i, ebs, cbs;
    uint32_t clksel;
    uint32_t devId;

    /* get cycle time*/
    /* TODO:for 9215,should be different according to different chip id(e.g. 9218) */
    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, FAL_SYS_CLK_REG, &clksel), ret);
    switch(clksel)
    {
        case 0:/*125M for 9215*/
            CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, FAL_CHIP_DEVICE_REG, &devId), ret);
            if (YT_TIGER_DEVICE_FPGA == (devId>>16))
            {
                FAL_CHIP_CYCLE_TIME(unit) = 7;
            }
            else
            {
                FAL_CHIP_CYCLE_TIME(unit) = 8;
            }
            break;
        case 1:/*143M for 9218*/
            FAL_CHIP_CYCLE_TIME(unit) = 7;
            break;
        default:
            FAL_CHIP_CYCLE_TIME(unit) = 8;
            break;
    }

    /* set meter global state enable */
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_METER_ENf, &global_ctrl_tbl, YT_ENABLE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);

    /* meter time_slot */
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_SET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, FAL_TIGER_DEFAULT_METER_TIME_SLOT);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    
    /* port shaping time_slot */
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &psch_time_slot), ret);
    HAL_FIELD_SET(PSCH_SHP_SLOT_TIME_CFGm, PSCH_SHP_SLOT_TIME_CFG_PSCH_SHP_SLOT_TIMEf, &psch_time_slot, FAL_TIGER_DEFAULT_PSHAP_TIME_SLOT);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &psch_time_slot), ret);


    /* queue shaping time_slot */
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &qsch_time_slot), ret);
    HAL_FIELD_SET(QSCH_SHP_SLOT_TIME_CFGm, QSCH_SHP_SLOT_TIME_CFG_QSCH_SHP_SLOT_TIMEf, &qsch_time_slot, FAL_TIGER_DEFAULT_QSHAP_TIME_SLOT);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &qsch_time_slot), ret);

    /* vlan policy and acl policy*/
    for (i = 0; i < CAL_MAX_METER_ENTRY_NUM(unit); ++i) {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, i, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, RATE_MODE_BYTE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_0f, &meter_config_tbl, (cbs & 0xfff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_1f, &meter_config_tbl, (cbs >> 12));
        ebs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_0f, &meter_config_tbl, (ebs & 0x3fff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_1f, &meter_config_tbl, (ebs >> 14));
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, i, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    }
    
    /* port policy*/
    for (i = 0; i < YT_PORT_NUM; ++i) {
        macid = CAL_YTP_TO_MAC(unit,i);
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, CAL_MAX_METER_ENTRY_NUM(unit) + macid, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, RATE_MODE_BYTE);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_ENf, &meter_config_tbl, YT_ENABLE);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_MODEf, &meter_config_tbl, METER_MODE_RFC4115);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_COLOR_MODEf, &meter_config_tbl, COLOR_BLIND);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_DROP_COLORf, &meter_config_tbl, DROP_COLOR_YR);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CFf, &meter_config_tbl, CF_MODE_NONE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_0f, &meter_config_tbl, (cbs & 0xfff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_1f, &meter_config_tbl, (cbs >> 12));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EIRf, &meter_config_tbl, 0);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_0f, &meter_config_tbl, 0);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_1f, &meter_config_tbl, 0);
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, CAL_MAX_METER_ENTRY_NUM(unit) + macid, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    }

    /* port shaping*/
    for (i = 0; i < YT_PORT_NUM; ++i) {
        macid = CAL_YTP_TO_MAC(unit,i);
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_TOKEN_LEVELf, &psch_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &psch_config_tbl, RATE_MODE_BYTE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_CBSf, &psch_config_tbl, cbs);
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
    }

    /**queue shaping*/
    for (i = 0; i < FAL_MAX_PORT_NUM * (CAL_MAX_UCAST_QUEUE_NUM(unit) + CAL_MAX_MCAST_QUEUE_NUM(unit)); ++i) {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, i, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_TOKEN_LEVELf, &qsch_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &qsch_config_tbl, RATE_MODE_BYTE);
        //cbs = 0;
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_CBSf, &qsch_config_tbl, cbs);
        ebs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_EBSf, &qsch_config_tbl, ebs);
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_CFG_TBLm, i, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_igrBandwidthCtrlEnable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    port_meter_ctrln_t port_meter_ctrl;
    yt_meterid_t meter_id;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    meter_id = macid;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_METER_CTRLNm, macid, sizeof(port_meter_ctrln_t), &port_meter_ctrl), ret);
    HAL_FIELD_SET(PORT_METER_CTRLNm, PORT_METER_CTRLN_METER_IDf, &port_meter_ctrl, meter_id);
    HAL_FIELD_SET(PORT_METER_CTRLNm, PORT_METER_CTRLN_METER_ENf, &port_meter_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PORT_METER_CTRLNm, macid, sizeof(port_meter_ctrln_t), &port_meter_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_igrBandwidthCtrlEnable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    port_meter_ctrln_t port_meter_ctrl;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_METER_CTRLNm, macid, sizeof(port_meter_ctrln_t), &port_meter_ctrl), ret);
    HAL_FIELD_GET(PORT_METER_CTRLNm, PORT_METER_CTRLN_METER_ENf, &port_meter_ctrl, pEnable);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_igrBandwidthCtrlMode_set(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t port_rate_mode)
{
    meter_config_tbl_t meter_config_tbl;
    yt_meterid_t meter_id;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    meter_id = macid + CAL_MAX_METER_ENTRY_NUM(unit);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, port_rate_mode.rate_mode);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_BYTE_RATE_MODEf, &meter_config_tbl, port_rate_mode.inc_gap);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_igrBandwidthCtrlMode_get(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t *pPort_rate_mode)
{
    meter_config_tbl_t meter_config_tbl;
    yt_meterid_t meter_id;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint8_t rate_mode = 0;
    uint8_t byte_rate_mode = 0;

    meter_id = macid + CAL_MAX_METER_ENTRY_NUM(unit);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, &rate_mode);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_BYTE_RATE_MODEf, &meter_config_tbl, &byte_rate_mode);
    pPort_rate_mode->rate_mode = rate_mode;
    pPort_rate_mode->inc_gap = byte_rate_mode;

    return CMM_ERR_OK;
}
yt_ret_t fal_tiger_rate_igrBandwidthCtrlRate_set(yt_unit_t unit, yt_port_t port, uint32_t rate)
{
    meter_timeslot_t meter_timeslot;
    meter_config_tbl_t meter_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, rate_mode;
    uint32_t time_slot;
    uint32_t cir, meter_id;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    meter_id = macid + CAL_MAX_METER_ENTRY_NUM(unit);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_GET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, &token_level);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, &rate_mode);
    cir = rate_tran_usr2reg(rate_mode, token_level, time_slot, rate);

    if (cir > (uint32_t)(0x40000 - 1))
    {
        osal_printf("Exceed CIR max tokens: time_slot=%d, token_level=%d, cir=0x%x.\r\n", time_slot, token_level, cir);
        return CMM_ERR_NOT_SUPPORT;
    }

    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CIRf, &meter_config_tbl, cir);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_igrBandwidthCtrlRate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate)
{
    meter_timeslot_t meter_timeslot;
    meter_config_tbl_t meter_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, rate_mode;
    uint32_t time_slot;
    uint32_t cir, meter_id;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    meter_id = macid + CAL_MAX_METER_ENTRY_NUM(unit);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_GET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, &token_level);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CIRf, &meter_config_tbl, &cir);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, &rate_mode);
    *pRate = rate_tran_reg2usr(rate_mode, token_level, time_slot, cir);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_vlan_enable_set(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t meter_id, yt_enable_t enable)
{
    l2_vlan_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VLAN_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_VLAN_TBLm, vid,sizeof(l2_vlan_tbl_t), &entry), ret);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_METER_IDf, &entry, meter_id);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_METER_ENf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_VLAN_TBLm, vid, sizeof(l2_vlan_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_vlan_enable_get(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t *pMeter_id, yt_enable_t *pEnable)
{
    l2_vlan_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_VLAN_TBLm, vid,sizeof(l2_vlan_tbl_t), &entry), ret);
    HAL_FIELD_GET(L2_VLAN_TBLm, L2_VLAN_TBL_METER_IDf, &entry, pMeter_id);
    HAL_FIELD_GET(L2_VLAN_TBLm, L2_VLAN_TBL_METER_ENf, &entry, pEnable);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_enable_set(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t enable)
{
    meter_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_ENf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_enable_get(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t *pEnable)
{
    meter_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_ENf, &entry, pEnable);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_mode_set(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t mode)
{
    meter_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_MODEf, &entry, mode.meter_mode);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &entry, mode.rate_mode);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_COLOR_MODEf, &entry, mode.color_mode);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_DROP_COLORf, &entry, mode.drop_color);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_BYTE_RATE_MODEf, &entry, mode.inc_gap);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CFf, &entry, mode.cf_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_mode_get(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t *pMode)
{
    meter_config_tbl_t entry;
    uint8_t meter_mode;
    uint8_t color_mode;
    uint8_t drop_color;
    uint8_t rate_mode;
    uint8_t byte_rate_mode;
    uint8_t cf;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &entry), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_MODEf, &entry, &meter_mode);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &entry, &rate_mode);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_DROP_COLORf, &entry, &drop_color);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_COLOR_MODEf, &entry, &color_mode);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_BYTE_RATE_MODEf, &entry, &byte_rate_mode);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CFf, &entry, &cf);
    
    pMode->meter_mode = meter_mode;
    pMode->rate_mode = rate_mode;
    pMode->color_mode = color_mode;
    pMode->drop_color = drop_color;
    pMode->inc_gap = byte_rate_mode;
    pMode->cf_mode = cf;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_rate_set(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t rate)
{
    meter_timeslot_t meter_timeslot;
    meter_config_tbl_t meter_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, rate_mode, meter_mode;
    uint32_t time_slot;
    uint32_t eir, cir;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_GET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, &token_level);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, &rate_mode);
    eir = rate_tran_usr2reg(rate_mode, token_level, time_slot, rate.eir);
    cir = rate_tran_usr2reg(rate_mode, token_level, time_slot, rate.cir);

    if (eir > (uint32_t)(0x40000 - 1) || cir > (uint32_t)(0x40000 - 1))
    {
        osal_printf("Exceed CIR/EIR max tokens: time_slot=%d, token_level=%d, eir=0x%x, cir=0x%x.\r\n", time_slot, token_level, eir, cir);
        return CMM_ERR_NOT_SUPPORT;
    }

    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_MODEf, &meter_config_tbl, &meter_mode);
    if (meter_mode == METER_MODE_RFC2698)
    {
        if ((eir < cir))
        {
            osal_printf("EIR should be larger than CIR in RFC2698 mode.\r\n");
            return CMM_ERR_NOT_SUPPORT;
        }
    }

    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EIRf, &meter_config_tbl, eir);
    HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CIRf, &meter_config_tbl, cir);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_meter_rate_get(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t *pRate)
{
    meter_timeslot_t meter_timeslot;
    meter_config_tbl_t meter_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, rate_mode;
    uint32_t time_slot;
    uint32_t eir, cir;

    CMM_PARAM_CHK(CAL_MAX_METER_ENTRY_NUM(unit) <= meter_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_GET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, METER_CONFIG_TBLm, meter_id, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, &token_level);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EIRf, &meter_config_tbl, &eir);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CIRf, &meter_config_tbl, &cir);
    HAL_FIELD_GET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, &rate_mode);
    pRate->cir = rate_tran_reg2usr(rate_mode, token_level, time_slot, cir);
    pRate->eir = rate_tran_reg2usr(rate_mode, token_level, time_slot, eir);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    psch_shp_cfg_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &entry), ret);
    HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_C_SHAPER_ENf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    psch_shp_cfg_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &entry), ret);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_C_SHAPER_ENf, &entry, pEnable);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_mode_set(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t shaping_mode)
{
    psch_shp_cfg_tbl_t shp_entry;
    psch_meter_cfg_tbl_t meter_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &shp_entry), ret);
    HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &shp_entry, shaping_mode.shp_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &shp_entry), ret);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_METER_CFG_TBLm, macid, sizeof(psch_meter_cfg_tbl_t), &meter_entry), ret);
    HAL_FIELD_SET(PSCH_METER_CFG_TBLm, PSCH_METER_CFG_TBL_METER_LENGTHf, &meter_entry, BYTE_RATE_GAP_INCLUDE - shaping_mode.sch_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_METER_CFG_TBLm, macid, sizeof(psch_meter_cfg_tbl_t), &meter_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_mode_get(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t *pShaping_mode)
{
    psch_shp_cfg_tbl_t shp_entry;
    psch_meter_cfg_tbl_t meter_entry;
    uint8_t shp_mode, meter_length;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &shp_entry), ret);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &shp_entry, &shp_mode);
    pShaping_mode->shp_mode = shp_mode;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_METER_CFG_TBLm, macid, sizeof(psch_meter_cfg_tbl_t), &meter_entry), ret);
    HAL_FIELD_GET(PSCH_METER_CFG_TBLm, PSCH_METER_CFG_TBL_METER_LENGTHf, &meter_entry, &meter_length);
    pShaping_mode->sch_mode = BYTE_RATE_GAP_INCLUDE - meter_length;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_rate_set(yt_unit_t unit, yt_port_t port, uint32_t rate)
{
    psch_shp_slot_time_cfg_t meter_timeslot;
    psch_shp_cfg_tbl_t psch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, shaper_mode;
    uint32_t time_slot;
    uint32_t cir;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &meter_timeslot), ret);
    HAL_FIELD_GET(PSCH_SHP_SLOT_TIME_CFGm, PSCH_SHP_SLOT_TIME_CFG_PSCH_SHP_SLOT_TIMEf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_TOKEN_LEVELf, &psch_config_tbl, &token_level);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &psch_config_tbl, &shaper_mode);
    cir = rate_tran_usr2reg(shaper_mode, token_level, time_slot, rate);

    if (cir > (uint32_t)(0x40000 - 1))
    {
        osal_printf("Exceed CIR max tokens: time_slot=%d, token_level=%d, cir=0x%x.\r\n", time_slot, token_level, cir);
        return CMM_ERR_NOT_SUPPORT;
    }
    HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_CIRf, &psch_config_tbl, cir);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_port_rate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate)
{
    psch_shp_slot_time_cfg_t meter_timeslot;
    psch_shp_cfg_tbl_t psch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, shaper_mode;
    uint32_t time_slot;
    uint32_t cir;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &meter_timeslot), ret);
    HAL_FIELD_GET(PSCH_SHP_SLOT_TIME_CFGm, PSCH_SHP_SLOT_TIME_CFG_PSCH_SHP_SLOT_TIMEf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_TOKEN_LEVELf, &psch_config_tbl, &token_level);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &psch_config_tbl, &shaper_mode);
    HAL_FIELD_GET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_CIRf, &psch_config_tbl, &cir);
    *pRate = rate_tran_reg2usr(shaper_mode, token_level, time_slot, cir);

    return CMM_ERR_OK;
}


yt_ret_t fal_tiger_rate_shaping_queue_enable_set(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t cshap_en, yt_enable_t eshap_en)
{
    qsch_shp_cfg_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &entry), ret);
    HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_C_SHAPER_ENf, &entry, cshap_en);
    HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_E_SHAPER_ENf, &entry, eshap_en);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_queue_enable_get(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t *pCshap_en, yt_enable_t *pEshap_en)
{
    qsch_shp_cfg_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &entry), ret);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_C_SHAPER_ENf, &entry, pCshap_en);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_E_SHAPER_ENf, &entry, pEshap_en);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_queue_mode_set(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t shaping_mode)
{
    qsch_shp_cfg_tbl_t shp_entry;
    qsch_meter_cfg_tbl_t meter_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &shp_entry), ret);
    HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &shp_entry, shaping_mode.shp_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &shp_entry), ret);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_METER_CFG_TBLm, idx, sizeof(qsch_meter_cfg_tbl_t), &meter_entry), ret);
    HAL_FIELD_SET(QSCH_METER_CFG_TBLm, QSCH_METER_CFG_TBL_METER_LENGTHf, &meter_entry, BYTE_RATE_GAP_INCLUDE - shaping_mode.sch_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_METER_CFG_TBLm, idx, sizeof(qsch_meter_cfg_tbl_t), &meter_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_queue_mode_get(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t *pShaping_mode)
{
    qsch_shp_cfg_tbl_t shp_entry;
    qsch_meter_cfg_tbl_t meter_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    uint8_t shaper_mode, sch_mode;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &shp_entry), ret);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &shp_entry, &shaper_mode);
    pShaping_mode->shp_mode = shaper_mode;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_METER_CFG_TBLm, idx, sizeof(qsch_meter_cfg_tbl_t), &meter_entry), ret);
    HAL_FIELD_GET(QSCH_METER_CFG_TBLm, QSCH_METER_CFG_TBL_METER_LENGTHf, &meter_entry, &sch_mode);
    pShaping_mode->sch_mode = BYTE_RATE_GAP_INCLUDE - sch_mode;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_queue_rate_set(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t rate)
{
    qsch_shp_slot_time_cfg_t meter_timeslot;
    qsch_shp_cfg_tbl_t qsch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, shaper_mode;
    uint32_t time_slot;
    uint32_t eir, cir;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &meter_timeslot), ret);
    HAL_FIELD_GET(QSCH_SHP_SLOT_TIME_CFGm, QSCH_SHP_SLOT_TIME_CFG_QSCH_SHP_SLOT_TIMEf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_TOKEN_LEVELf, &qsch_config_tbl, &token_level);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &qsch_config_tbl, &shaper_mode);
    eir = rate_tran_usr2reg(shaper_mode, token_level, time_slot, rate.eir);
    cir = rate_tran_usr2reg(shaper_mode, token_level, time_slot, rate.cir);

    if (eir > (uint32_t)(0x40000 - 1) || cir > (uint32_t)(0x40000 - 1))
    {
        osal_printf("Exceed CIR/EIR max tokens: time_slot=%d, token_level=%d.\r\n", time_slot, token_level);
        return CMM_ERR_NOT_SUPPORT;
    }

    HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_EIRf, &qsch_config_tbl, eir);
    HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_CIRf, &qsch_config_tbl, cir);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_shaping_queue_rate_get(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t *pRate)
{
    qsch_shp_slot_time_cfg_t meter_timeslot;
    qsch_shp_cfg_tbl_t qsch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t token_level, shaper_mode;
    uint32_t time_slot;
    uint32_t eir, cir;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &meter_timeslot), ret);
    HAL_FIELD_GET(QSCH_SHP_SLOT_TIME_CFGm, QSCH_SHP_SLOT_TIME_CFG_QSCH_SHP_SLOT_TIMEf, &meter_timeslot, &time_slot);
    time_slot = time_slot * 8 * FAL_CHIP_CYCLE_TIME(unit); //ns
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;

    }
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_SHP_CFG_TBLm, idx, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_EIRf, &qsch_config_tbl, &eir);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_CIRf, &qsch_config_tbl, &cir);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_TOKEN_LEVELf, &qsch_config_tbl, &token_level);
    HAL_FIELD_GET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &qsch_config_tbl, &shaper_mode);
    pRate->cir = rate_tran_reg2usr(shaper_mode, token_level, time_slot, cir);
    pRate->eir = rate_tran_reg2usr(shaper_mode, token_level, time_slot, eir);

    return CMM_ERR_OK;
}
