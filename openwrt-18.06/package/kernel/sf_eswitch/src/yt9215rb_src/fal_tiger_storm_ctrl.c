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
#include "yt_storm_ctrl.h"
#include "fal_tiger_storm_ctrl.h"

/*
 * Symbol Definition
 */

/*
 * Macro Declaration
 */

/*
 * Data Declaration
 */

/*
 * Function Declaration
 */
static uint32_t fal_tiger_get_regidx(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint8_t *pidx)
{
    yt_macid_t macid;

    macid = CAL_YTP_TO_MAC(unit,port);

    switch(storm_type)
    {
        case STORM_TYPE_BCAST:
            *pidx = macid;
            break;
        case STORM_TYPE_MCAST:
        case STORM_TYPE_UNKNOWN_MCAST:
            *pidx = FAL_MAX_PORT_NUM + macid;
            break;
        case STORM_TYPE_UNKNOWN_UCAST:
            *pidx = FAL_MAX_PORT_NUM * 2 + macid;
            break;
        default:
            return CMM_ERR_FAIL;
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_init(yt_unit_t unit)
{
    global_ctrl1_t global_ctrl_tbl;
    storm_ctrl_timeslot_t   storm_ctrl_timeslot;
    storm_ctrl_config_tbl_t storm_ctrl_config_tbl;
    uint8_t idx = 0;
    uint8_t storm_type;
    yt_port_t port;
    cmm_err_t ret = CMM_ERR_OK;

    /* set meter global state enable */
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_METER_ENf, &global_ctrl_tbl, YT_ENABLE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);
    /*init timeslot*/
    HAL_FIELD_SET(STORM_CTRL_TIMESLOTm, STORM_CTRL_TIMESLOT_TIMESLOTf, &storm_ctrl_timeslot, STORM_DEFAULT_TIMESLOT);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_TIMESLOTm, 0, sizeof(storm_ctrl_timeslot_t), &storm_ctrl_timeslot), ret);

    /*init cbs */
    for(storm_type = STORM_TYPE_BCAST; storm_type <= STORM_TYPE_UNKNOWN_UCAST; storm_type++)
    {
        for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
        {
            if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_OK)
            {
                CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &storm_ctrl_config_tbl), ret);
                HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_RATE_MODEf, &storm_ctrl_config_tbl, STORM_RATE_MODE_BYTE);
                HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_CBSf, &storm_ctrl_config_tbl, STORM_DEFAULT_CBS_BYTE);
                CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &storm_ctrl_config_tbl), ret);
            }
        }
    }
    
    return CMM_ERR_OK;    
}

yt_ret_t fal_tiger_storm_ctrl_enable_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t enable)
{
    storm_ctrl_config_tbl_t entry;
    storm_ctrl_mc_type_ctrl_t mcentry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    yt_macid_t macid;
    uint16_t mc_type = 0;

    macid = CAL_YTP_TO_MAC(unit,port);

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret);
    HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_STORM_ENf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret);

    if(storm_type == STORM_TYPE_UNKNOWN_MCAST)
    {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_MC_TYPE_CTRLm, 0, sizeof(storm_ctrl_mc_type_ctrl_t), &mcentry), ret);
        HAL_FIELD_GET(STORM_CTRL_MC_TYPE_CTRLm, STORM_CTRL_MC_TYPE_CTRL_STORM_CTRL_MC_TYPEf, &mcentry, &mc_type);
        if(enable == YT_ENABLE)
        {
        mc_type |= (0x1 << macid);
        }
        else
        {
        mc_type &= ~(0x1 << macid);
        }
        
        HAL_FIELD_SET(STORM_CTRL_MC_TYPE_CTRLm, STORM_CTRL_MC_TYPE_CTRL_STORM_CTRL_MC_TYPEf, &mcentry, mc_type);
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_MC_TYPE_CTRLm, 0, sizeof(storm_ctrl_mc_type_ctrl_t), &mcentry), ret);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_enable_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t *penable)
{
    storm_ctrl_config_tbl_t entry;
    storm_ctrl_mc_type_ctrl_t mcentry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    yt_macid_t macid;
    uint8_t storm_en = 0;
    uint16_t mc_type = 0;

    macid = CAL_YTP_TO_MAC(unit,port);

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret); 
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_STORM_ENf, &entry, &storm_en);
    *penable = storm_en ? YT_ENABLE : YT_DISABLE;

    if(storm_type == STORM_TYPE_UNKNOWN_MCAST)
    {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_MC_TYPE_CTRLm, 0, sizeof(storm_ctrl_mc_type_ctrl_t), &mcentry), ret);
        HAL_FIELD_GET(STORM_CTRL_MC_TYPE_CTRLm, STORM_CTRL_MC_TYPE_CTRL_STORM_CTRL_MC_TYPEf, &mcentry, &mc_type);
        if((mc_type & (0x1 << macid)))
        {
        *penable = YT_ENABLE;
        }
        else
        {
        *penable = YT_DISABLE;
        }
    }    

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_mode_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t rate_mode)
{
    storm_ctrl_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }
        
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret); 
    HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_RATE_MODEf, &entry, rate_mode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_mode_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t *p_rate_mode)
{
    storm_ctrl_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    uint8_t rate_mode = 0;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }
        
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret); 
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_RATE_MODEf, &entry, &rate_mode);
    *p_rate_mode = rate_mode ? STORM_RATE_MODE_PACKET : STORM_RATE_MODE_BYTE;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_include_gap_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t inc_gap)
{
    storm_ctrl_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret); 
    HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_GAP_INCf, &entry, inc_gap);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret); 

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_include_gap_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t *p_inc_gap)
{
    storm_ctrl_config_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    uint8_t inc_gap = 0;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &entry), ret);
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_GAP_INCf, &entry, &inc_gap);
    *p_inc_gap = inc_gap ? STORM_RATE_GAP_INCLUDE : STORM_RATE_GAP_EXCLUDE;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t rate)
{
    storm_ctrl_timeslot_t storm_ctrl_timeslot;
    storm_ctrl_config_tbl_t storm_ctrl_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t time_slot;
    uint32_t cir = 0, cbs = 0;
    uint8_t idx = 0;
    uint8_t rate_mode = 0;
    uint32_t divisor;
    uint64 dividend;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_TIMESLOTm, 0, sizeof(storm_ctrl_timeslot_t), &storm_ctrl_timeslot), ret);
    HAL_FIELD_GET(STORM_CTRL_TIMESLOTm, STORM_CTRL_TIMESLOT_TIMESLOTf, &storm_ctrl_timeslot, &time_slot);
    time_slot = time_slot * 10000;/*10 * 1000,ns*/

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &storm_ctrl_config_tbl), ret);
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_RATE_MODEf, &storm_ctrl_config_tbl, &rate_mode);
    divisor = 1000000000;
    if (rate_mode == STORM_RATE_MODE_BYTE)
    {
        /* 1 token 1 byte */
        dividend = ((uint64)rate * time_slot) >> 3;
        do_div64(dividend, divisor); /*us->s*/
        cir = (uint32_t)dividend;
        cir = (cir == 0) ? 1 : cir;
        
        cbs = STORM_DEFAULT_CBS_BYTE;/*x/512, 1*unit for 2^9 tokens*/
    }
    else 
    {
        /* 1 token 1/1024 packet */
        dividend = (uint64)rate * 1024 * time_slot;
        do_div64(dividend, divisor); /*us->s*/
        cir = (uint32_t)dividend;
        cir = (cir <= 1) ? 2 : cir;
        
        cbs = STORM_DEFAULT_CBS_PACKET; /*x*1024 /512 1*unit for 2^9 tokens */
    }

    cbs = (cbs == 0) ? 1 : cbs;

    if (cir > (0x80000 - 1)) 
    {
        return CMM_ERR_NOT_SUPPORT;
    }

    if (cbs > (0x400 - 1)) 
    {
        return CMM_ERR_NOT_SUPPORT;
    }

    HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_CIRf, &storm_ctrl_config_tbl, cir);
    HAL_FIELD_SET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_CBSf, &storm_ctrl_config_tbl, cbs);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &storm_ctrl_config_tbl), ret);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_storm_ctrl_rate_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t *prate)
{
    storm_ctrl_timeslot_t storm_ctrl_timeslot;
    storm_ctrl_config_tbl_t storm_ctrl_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t time_slot;
    uint8_t idx = 0;
    uint8_t rate_mode = 0;
    uint32_t cir = 0;
    uint32_t divisor;
    uint64 dividend;

    if(fal_tiger_get_regidx(unit, port, storm_type, &idx) == CMM_ERR_FAIL)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_TIMESLOTm, 0, sizeof(storm_ctrl_timeslot_t), &storm_ctrl_timeslot), ret);
    HAL_FIELD_GET(STORM_CTRL_TIMESLOTm, STORM_CTRL_TIMESLOT_TIMESLOTf, &storm_ctrl_timeslot, &time_slot);
    time_slot = time_slot * 10000;/*10 * 1000,ns*/
    if (0 == time_slot)
    {
        return CMM_ERR_NOT_INIT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, STORM_CTRL_CONFIG_TBLm, idx, sizeof(storm_ctrl_config_tbl_t), &storm_ctrl_config_tbl), ret); 
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_RATE_MODEf, &storm_ctrl_config_tbl, &rate_mode);
    HAL_FIELD_GET(STORM_CTRL_CONFIG_TBLm, STORM_CTRL_CONFIG_TBL_CIRf, &storm_ctrl_config_tbl, &cir);
    divisor = time_slot;
    if (rate_mode == STORM_RATE_MODE_BYTE)
    {
        dividend = (uint64)cir * 8 * 1000000000;
    }
    else
    {
        dividend = ((uint64)cir * 1000000000) >> 10;  /* 2^10 */
    }
    do_div64(dividend, divisor); /*us->s*/
    *prate = (uint32_t)dividend;

    return CMM_ERR_OK;
}
