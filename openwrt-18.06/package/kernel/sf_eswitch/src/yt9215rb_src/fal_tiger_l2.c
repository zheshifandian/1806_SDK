/*
 * Include Files
 */
#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "fal_cmm.h"
#include "fal_tiger_l2.h"
#include "fal_tiger_lag.h"
#include "yt_l2.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"

/*
 * Symbol Definition
 */

/*
 * Macro Declaration
 */

/*
 * Data Declaration
 */

typedef l2_fdb_tbl_bin0_t l2_fdb_tbl_t;

static uint32_t  fal_tiger_l2_fdb_addr_flush(yt_unit_t unit, yt_l2_tbl_flush_ctrl_t op_flush);
static uint32_t  fal_tiger_l2_fdb_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, 
                                                     yt_l2_fdb_info_t *pfdb_info, uint16_t *plookup_index);
static uint32_t  fal_tiger_l2_fdb_op_add(yt_unit_t unit,  yt_l2_fdb_op_mode_t op_mode,
                     uint16_t fid, yt_mac_addr_t mac_addr, yt_port_mask_t port_mask,
                     yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_add_cfg(yt_unit_t unit, yt_l2_fdb_op_mode_t op_mode, 
                                                                 l2_fdb_tbl_t l2_fdb, yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_flush(yt_unit_t unit, yt_l2_tbl_flush_ctrl_t op_flush, yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_get_one(yt_unit_t unit,uint16_t fid, yt_mac_addr_t mac_addr, 
                                                                 yt_l2_fdb_info_t *pfdb_info, yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_get_one_withidx(yt_unit_t unit, uint16_t idx, 
                                                                             yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info,
                                                                             yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_fdb_op_get_next_withidx(yt_unit_t unit, uint16_t idx, 
                                                                    yt_mac_addr_t *pmac_addr, uint16_t *pfid, uint16_t *pnext_index, 
                                                                    yt_l2_fdb_info_t *pfdb_info, yt_l2_fdb_op_result_t *pop_result);
static uint32_t  fal_tiger_l2_op_info_get(yt_unit_t unit,  yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info);
static  uint32_t  fal_tiger_l2_op_result_get(yt_unit_t unit,  yt_l2_fdb_op_result_t *pop_result);
static uint32_t fal_tiger_l2_tbl_read(yt_unit_t unit, uint16_t lookup_index, l2_fdb_tbl_t *pl2_fdb);
static uint32_t fal_tiger_l2_tbl_write(yt_unit_t unit, uint16_t lookup_index, l2_fdb_tbl_t *pl2_fdb);

static uint32_t fal_tiger_l2_tbl_write(yt_unit_t unit, uint16_t lookup_index, l2_fdb_tbl_t *pl2_fdb)
{
    cmm_err_t ret = CMM_ERR_OK;
    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_op_mode_t op_mode;

    if(NULL == pl2_fdb)
    {
        return CMM_ERR_NULL_POINT;
    }

    op_mode.l2_fdb_op_mode = L2_FDB_OP_MODE_INDEX;
    op_mode.entry_idx = lookup_index;
    
    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_add_cfg(unit, op_mode, *pl2_fdb, &op_result),ret);
        
        if(op_result.lookup_fail)
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }
    }

    return CMM_ERR_OK;
}

static uint32_t fal_tiger_l2_tbl_read(yt_unit_t unit, uint16_t lookup_index, l2_fdb_tbl_t *pl2_fdb)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_info_t fdb_info;
    yt_mac_addr_t mac_addr;
    uint16_t fid;

    if(NULL == pl2_fdb) {return CMM_ERR_NULL_POINT;}
    
    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);
    
    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_one_withidx(unit, lookup_index, &mac_addr, &fid, &fdb_info, &op_result),ret);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_0f, pl2_fdb, ((mac_addr.addr[0] << 24) | (mac_addr.addr[1] << 16) | (mac_addr.addr[2] << 8) | mac_addr.addr[3]));
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_1f, pl2_fdb, ((mac_addr.addr[4] << 8) | mac_addr.addr[5]));
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_FIDf, pl2_fdb, fid);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_STATUSf, pl2_fdb, fdb_info.STATUS);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_INT_PRI_ENf, pl2_fdb, fdb_info.DMAC_INT_PRI_EN);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_NEW_VIDf, pl2_fdb, fdb_info.NEW_VID);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_INT_PRIf, pl2_fdb, fdb_info.INT_PRI);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_INT_PRI_ENf, pl2_fdb, fdb_info.SMAC_INT_PRI_EN);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_COPY_TO_CPUf, pl2_fdb, fdb_info.COPY_TO_CPU);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_DROPf, pl2_fdb, fdb_info.DMAC_DROP);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DST_PORT_MASKf, pl2_fdb, fdb_info.DST_PORT_MASK);
        HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_DROPf, pl2_fdb, fdb_info.SMAC_DROP);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_init(yt_unit_t unit)
{
    uint8_t id;

    fal_tiger_l2_system_learnlimit_cnt_set(unit, L2_FDB_ENTRY_NUM);
    for(id = 0; id < CAL_PORT_NUM_ON_UNIT(unit); id++)
    {
        fal_tiger_l2_port_learnlimit_cnt_set(unit, id, L2_FDB_ENTRY_NUM);
    }
    for(id = 0; id < FAL_MAX_LAG_NUM; id++)
    {
        fal_tiger_l2_lag_learnlimit_cnt_set(unit, id, L2_FDB_ENTRY_NUM);
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_mcast_addr_add(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_port_mask_t port_mask)
{
    cmm_err_t ret           = CMM_ERR_OK;
    
    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_op_mode_t op_mode;
    yt_port_mask_t  macmask;

    CAL_YTPLIST_TO_MLIST(unit,port_mask, macmask);

    op_mode.l2_fdb_op_mode = L2_FDB_OP_MODE_HASH;
    op_mode.entry_idx = 0;
    
    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_add(unit, op_mode, vid, mac_addr, macmask, &op_result),ret);
        if(op_result.lookup_fail)
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_ucast_addr_add(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_port_t port, yt_bool_t isLag)
{
    yt_port_mask_t portmask;
#ifdef LAG_INCLUDED
    yt_link_agg_group_t lagInfo;
    yt_port_mask_t lagportmask;
    cmm_err_t ret = CMM_ERR_OK;
#endif

    if(isLag == FALSE)/* phy port */
    {
        CMM_CLEAR_MEMBER_PORT(portmask);
        CMM_SET_MEMBER_PORT(portmask, port);
    }
#ifdef LAG_INCLUDED
    else if(isLag == TRUE) /* check lag port is valid or not */
    {
        CMM_PARAM_CHK((FAL_MAX_LAG_NUM <= port), CMM_ERR_INPUT);
        CMM_ERR_CHK(fal_tiger_lag_group_info_get(unit, port, &lagInfo), ret);
        if(lagInfo.member_num == 0)
        {
            return CMM_ERR_NOT_INIT;
        }
        lagportmask.portbits[0] = lagInfo.member_portmask;
        CAL_YTPLIST_TO_MLIST(unit, lagportmask, portmask);
    }
#endif

    return fal_tiger_l2_fdb_mcast_addr_add(unit, vid, mac_addr, portmask);
}

static uint32_t  fal_tiger_l2_fdb_addr_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_del(unit,  vid,  mac_addr, &op_result), ret);
        if(op_result.lookup_fail)
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_ucast_addr_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr)
{
    return fal_tiger_l2_fdb_addr_del(unit, vid, mac_addr);
}

yt_ret_t  fal_tiger_l2_fdb_mcast_addr_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr)
{
    return fal_tiger_l2_fdb_addr_del(unit, vid, mac_addr);
}

yt_ret_t  fal_tiger_l2_fdb_linkdownFlush_en_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_hw_flush_ctrl_t flush_op;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_HW_FLUSH_CTRLm, 0, sizeof(l2_fdb_hw_flush_ctrl_t), &flush_op), ret);
    HAL_FIELD_SET(L2_FDB_HW_FLUSH_CTRLm, L2_FDB_HW_FLUSH_CTRL_GLOBAL_ENf, &flush_op, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_HW_FLUSH_CTRLm, 0, sizeof(l2_fdb_hw_flush_ctrl_t), &flush_op), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_linkdownFlush_en_get(yt_unit_t unit, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_hw_flush_ctrl_t flush_op;
    uint32 enable;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_HW_FLUSH_CTRLm, 0, sizeof(l2_fdb_hw_flush_ctrl_t), &flush_op), ret);

    HAL_FIELD_GET(L2_FDB_HW_FLUSH_CTRLm, L2_FDB_HW_FLUSH_CTRL_GLOBAL_ENf, &flush_op, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_fdb_all_ucast_flush(yt_unit_t unit)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));
    
    flush_op.mode = L2_FDB_FLUSH_MODE_UFDB_ALL;

    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

yt_ret_t fal_tiger_l2_fdb_port_ucast_flush(yt_unit_t unit, yt_port_t port)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));

    CMM_CLEAR_MEMBER_PORT(flush_op.port_mask);
    CMM_SET_MEMBER_PORT(flush_op.port_mask, CAL_YTP_TO_MAC(unit,port));

    flush_op.mode = L2_FDB_FLUSH_MODE_UFDB_PORT;
        
    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

yt_ret_t fal_tiger_l2_fdb_vlan_ucast_flush(yt_unit_t unit, yt_vlan_t vid)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));

    flush_op.fid = vid;

    flush_op.mode = L2_FDB_FLUSH_MODE_UFDB_FID;
        
    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

yt_ret_t fal_tiger_l2_fdb_all_mcast_flush(yt_unit_t unit)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));
    
    flush_op.mode = L2_FDB_FLUSH_MODE_MFDB_ALL;
        
    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

yt_ret_t fal_tiger_l2_fdb_port_mcast_flush(yt_unit_t unit, yt_port_t port)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));

    CMM_CLEAR_MEMBER_PORT(flush_op.port_mask);
    CMM_SET_MEMBER_PORT(flush_op.port_mask, CAL_YTP_TO_MAC(unit,port));

    flush_op.mode = L2_FDB_FLUSH_MODE_MFDB_PORT;

    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

yt_ret_t fal_tiger_l2_fdb_vlan_mcast_flush(yt_unit_t unit, yt_vlan_t vid)
{
    yt_l2_tbl_flush_ctrl_t flush_op;

    osal_memset(&flush_op, 0, sizeof(yt_l2_tbl_flush_ctrl_t));

    flush_op.fid = vid;

    flush_op.mode = L2_FDB_FLUSH_MODE_MFDB_FID;
        
    return fal_tiger_l2_fdb_addr_flush(unit, flush_op);
}

static uint32_t  fal_tiger_l2_fdb_addr_flush(yt_unit_t unit, yt_l2_tbl_flush_ctrl_t op_flush)
{
    cmm_err_t ret           = CMM_ERR_OK;
    hal_reg_tbl_mode_t mode;
    yt_l2_fdb_op_result_t op_result;
    uint32 enable;

    CMM_ERR_CHK(fal_tiger_l2_fdb_linkdownFlush_en_get(unit, &enable), ret);
    if (enable == YT_ENABLE)
    {
        return CMM_ERR_FORBIDDEN;
    }

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_flush(unit, op_flush, &op_result), ret);
    }

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, 
                                            yt_l2_fdb_info_t *pfdb_info, uint16_t *plookup_index)
{
    cmm_err_t ret           = CMM_ERR_OK;
    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == plookup_index), CMM_ERR_NULL_POINT);
        
    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    osal_memset(&op_result, 0, sizeof(op_result)); 

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_one(unit, vid, mac_addr, pfdb_info, &op_result), ret);
        
        if(op_result.lookup_fail)
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }
        
        *plookup_index = op_result.entry_idx;
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_type_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_l2_fdb_type_t *ptype)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr, &fdb_info, &lookup_index), ret);

    *ptype = (fdb_info.STATUS == FDB_STATUS_STATIC) ? FDB_TYPE_STATIC : FDB_TYPE_DYNAMIC;
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_ENf, &l2_learn_per_port_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;
    uint32_t enable;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_ENf, &l2_learn_per_port_ctrl, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_cnt_set(yt_unit_t unit, yt_port_t port, uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_NUMf, &l2_learn_per_port_ctrl, maxcnt);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_cnt_get(yt_unit_t unit, yt_port_t port, uint32_t *pmaxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;
    uint32_t maxcnt;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_NUMf, &l2_learn_per_port_ctrl, &maxcnt);
    *pmaxcnt = maxcnt;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_exceed_drop_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_DROPf, &l2_learn_per_port_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_exceed_drop_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;
    uint32_t enable;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_DROPf, &l2_learn_per_port_ctrl, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_en_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_ENf, &l2_learn_global_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_en_get(yt_unit_t unit, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;
    uint32_t enable;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_ENf, &l2_learn_global_ctrl, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_cnt_set(yt_unit_t unit, uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_NUMf, &l2_learn_global_ctrl, maxcnt);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_cnt_get(yt_unit_t unit, uint32_t *pmaxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;
    uint32_t maxcnt;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_NUMf, &l2_learn_global_ctrl, &maxcnt);
    *pmaxcnt = maxcnt;
   
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_exceed_drop_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_DROPf, &l2_learn_global_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_exceed_drop_get(yt_unit_t unit, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;
    uint32_t enable;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_DROPf, &l2_learn_global_ctrl, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_drop_sa_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));
       
    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_DROPf, &l2_fdb_tbl, enable);
    CMM_ERR_CHK(fal_tiger_l2_tbl_write(unit,lookup_index, &l2_fdb_tbl),ret);
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_drop_sa_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr,  yt_enable_t *pEnable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;
    uint32_t enable;

    CMM_PARAM_CHK((NULL == pEnable), CMM_ERR_NULL_POINT);

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));

    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_DROPf, &l2_fdb_tbl, &enable);
    *pEnable        = enable ? YT_ENABLE : YT_DISABLE;
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_drop_da_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));
       
    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_DROPf, &l2_fdb_tbl, enable);
    CMM_ERR_CHK(fal_tiger_l2_tbl_write(unit,lookup_index, &l2_fdb_tbl),ret);
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_drop_da_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr,  yt_enable_t *pEnable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;
    uint32_t enable;

    CMM_PARAM_CHK((NULL == pEnable), CMM_ERR_NULL_POINT);

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));

    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_DROPf, &l2_fdb_tbl, &enable);
    *pEnable        = enable ? YT_ENABLE : YT_DISABLE;
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_copy2cpu_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));
       
    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_COPY_TO_CPUf, &l2_fdb_tbl, enable);
    CMM_ERR_CHK(fal_tiger_l2_tbl_write(unit,lookup_index, &l2_fdb_tbl),ret);
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_copy2cpu_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t *pEnable)
{
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    l2_fdb_tbl_t l2_fdb_tbl;
    uint32_t enable;

    osal_memset(&fdb_info, 0, sizeof(fdb_info));
    osal_memset(&l2_fdb_tbl, 0, sizeof(l2_fdb_tbl));
       
    #ifdef SDK_DATABASE_HW_EN
    CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr,  &fdb_info, &lookup_index), ret);
    CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit,lookup_index, &l2_fdb_tbl),ret);
    #endif

    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_COPY_TO_CPUf, &l2_fdb_tbl, &enable);
    *pEnable = enable ? YT_ENABLE : YT_DISABLE;
    
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_mcast_set(yt_unit_t unit, yt_port_mask_t port_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_mcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;

    CAL_YTPLIST_TO_MLIST(unit,port_mask, macmask);

#ifndef INTER_MCU
    macmask.portbits[0] |= (1<<FAL_INTERNAL_CPU_MACID);
#endif
    HAL_FIELD_SET(L2_MCAST_FILTER_MASKm, L2_MCAST_FILTER_MASK_MCAST_FILTER_MASKf, &filter_mask, macmask.portbits[0]);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_MCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_mcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask)
{
    cmm_err_t ret       = CMM_ERR_OK;
    l2_mcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;
    uint32_t port_mask;
    
    CMM_PARAM_CHK((NULL == pport_mask), CMM_ERR_NULL_POINT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    HAL_FIELD_GET(L2_MCAST_FILTER_MASKm, L2_MCAST_FILTER_MASK_MCAST_FILTER_MASKf, &filter_mask, &port_mask);
    macmask.portbits[0] = port_mask;

    CAL_MLIST_TO_YTPLIST(unit, macmask, (*pport_mask));

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_bcast_set(yt_unit_t unit, yt_port_mask_t port_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_bcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;

    CAL_YTPLIST_TO_MLIST(unit,port_mask, macmask);

#ifndef INTER_MCU
    macmask.portbits[0] |= (1<<FAL_INTERNAL_CPU_MACID);
#endif
    HAL_FIELD_SET(L2_BCAST_FILTER_MASKm, L2_BCAST_FILTER_MASK_BCAST_FILTER_MASKf, &filter_mask, macmask.portbits[0]);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_BCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_bcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_bcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;
    uint32_t port_mask;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_BCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    HAL_FIELD_GET(L2_BCAST_FILTER_MASKm, L2_BCAST_FILTER_MASK_BCAST_FILTER_MASKf, &filter_mask, &port_mask);
    macmask.portbits[0] = port_mask;
    
    CAL_MLIST_TO_YTPLIST(unit, macmask, (*pport_mask));

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_unknown_ucast_set(yt_unit_t unit, yt_port_mask_t port_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_unknown_ucast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;

    CAL_YTPLIST_TO_MLIST(unit,port_mask, macmask);

#ifndef INTER_MCU
    macmask.portbits[0] |= (1<<FAL_INTERNAL_CPU_MACID);
#endif
    HAL_FIELD_SET(L2_UNKNOWN_UCAST_FILTER_MASKm, L2_UNKNOWN_UCAST_FILTER_MASK_UNKNOWN_UCAST_FILTER_MASKf, &filter_mask, macmask.portbits[0]);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_UNKNOWN_UCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_unknown_ucast_get(yt_unit_t unit, yt_port_mask_t *pport_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_unknown_ucast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;
    uint32_t port_mask;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_UNKNOWN_UCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    HAL_FIELD_GET(L2_UNKNOWN_UCAST_FILTER_MASKm, L2_UNKNOWN_UCAST_FILTER_MASK_UNKNOWN_UCAST_FILTER_MASKf, &filter_mask, &port_mask);
    macmask.portbits[0] = port_mask;
    
    CAL_MLIST_TO_YTPLIST(unit, macmask, (*pport_mask));

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_unknown_mcast_set(yt_unit_t unit, yt_port_mask_t port_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_unknown_mcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;

    CAL_YTPLIST_TO_MLIST(unit,port_mask, macmask);

#ifndef INTER_MCU
    macmask.portbits[0] |= (1<<FAL_INTERNAL_CPU_MACID);
#endif
    HAL_FIELD_SET(L2_UNKNOWN_MCAST_FILTER_MASKm, L2_UNKNOWN_MCAST_FILTER_MASK_UNKNOWN_MCAST_FILTER_MASKf, &filter_mask, macmask.portbits[0]);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_UNKNOWN_MCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_filter_unknown_mcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_unknown_mcast_filter_mask_t filter_mask;
    yt_port_mask_t  macmask;
    uint32_t port_mask;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_UNKNOWN_MCAST_FILTER_MASKm, 0, sizeof(filter_mask), &filter_mask), ret);

    HAL_FIELD_GET(L2_UNKNOWN_MCAST_FILTER_MASKm, L2_UNKNOWN_MCAST_FILTER_MASK_UNKNOWN_MCAST_FILTER_MASKf, &filter_mask, &port_mask);
    macmask.portbits[0] = port_mask;
    
    CAL_MLIST_TO_YTPLIST(unit, macmask, (*pport_mask));

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_rma_bypass_unknown_mcast_filter_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret       = CMM_ERR_OK;
    l2_mc_unknown_act_ctrl_t mc_act;
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &mc_act), ret);

    HAL_FIELD_SET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_DROP_RMABYPASSf, &mc_act, enable);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &mc_act), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_rma_bypass_unknown_mcast_filter_get(yt_unit_t unit, yt_enable_t *pEnable)
{
    cmm_err_t ret       = CMM_ERR_OK;
    l2_mc_unknown_act_ctrl_t mc_act;
    uint32_t enable;
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &mc_act), ret);
    HAL_FIELD_GET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_DROP_RMABYPASSf, &mc_act, &enable);
    *pEnable = enable;

    return CMM_ERR_OK;
}

uint32_t  fal_tiger_l2_igmp_bypass_unknown_mcast_filter_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret       = CMM_ERR_OK;
    l2_mc_unknown_act_ctrl_t mc_act;
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &mc_act), ret);

    HAL_FIELD_SET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_DROP_IGMPBYPASSf, &mc_act, enable);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &mc_act), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_port_uc_cnt_get(yt_unit_t unit, yt_port_t port, uint32 *pcnt)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_port_learn_mac_cntn_t l2_port_learn_mac_cntn;
    yt_macid_t macid;
    uint32_t mac_cnt;

    CMM_PARAM_CHK((NULL == pcnt), CMM_ERR_NULL_POINT);

    macid = CAL_YTP_TO_MAC(unit,port);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_PORT_LEARN_MAC_CNTNm, macid, sizeof(l2_port_learn_mac_cntn), &l2_port_learn_mac_cntn), ret);
    HAL_FIELD_GET(L2_PORT_LEARN_MAC_CNTNm, L2_PORT_LEARN_MAC_CNTN_MAC_CNTf, &l2_port_learn_mac_cntn, &mac_cnt);
    *pcnt = mac_cnt;

    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_fdb_lag_uc_cnt_get(yt_unit_t unit, uint8_t groupid, uint32_t *pcnt)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_port_learn_mac_cntn_t l2_port_learn_mac_cntn;
    uint32_t mac_cnt;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_PARAM_CHK((NULL == pcnt), CMM_ERR_NULL_POINT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_PORT_LEARN_MAC_CNTNm, FAL_MAX_PORT_NUM + groupid, sizeof(l2_port_learn_mac_cntn), &l2_port_learn_mac_cntn), ret);
    HAL_FIELD_GET(L2_PORT_LEARN_MAC_CNTNm, L2_PORT_LEARN_MAC_CNTN_MAC_CNTf, &l2_port_learn_mac_cntn, &mac_cnt);
    *pcnt = mac_cnt;

    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_en_set(yt_unit_t unit, uint8_t groupid,  yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_SET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_ENf, &l2_lag_learn_limit_ctrl, enable);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_en_get(yt_unit_t unit, uint8_t groupid,  yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;
    uint32_t enable;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);
    HAL_FIELD_GET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_ENf, &l2_lag_learn_limit_ctrl, &enable);
    *pEnable = enable;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_cnt_set(yt_unit_t unit, uint8_t groupid,  uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_SET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_NUMf, &l2_lag_learn_limit_ctrl, maxcnt);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_cnt_get(yt_unit_t unit, uint8_t groupid,  uint32_t *pmaxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;
    uint32_t maxcnt;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_GET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_NUMf, &l2_lag_learn_limit_ctrl, &maxcnt);
    *pmaxcnt = maxcnt;
    

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_exceed_drop_set(yt_unit_t unit, uint8_t groupid, yt_enable_t enable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_SET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_DROPf, &l2_lag_learn_limit_ctrl, enable);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_lag_learnlimit_exceed_drop_get(yt_unit_t unit, uint8_t groupid, yt_enable_t *pEnable)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;
    uint32_t enable;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_GET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_DROPf, &l2_lag_learn_limit_ctrl, &enable);
    *pEnable = enable;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_uc_cnt_get(yt_unit_t unit,  uint32 *pcnt)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_learn_mac_cnt_t l2_learn_mac_cntn;
    uint32_t mac_cnt;
    
    CMM_PARAM_CHK((NULL == pcnt), CMM_ERR_NULL_POINT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_MAC_CNTm, 0, sizeof(l2_learn_mac_cntn), &l2_learn_mac_cntn), ret);
    HAL_FIELD_GET(L2_LEARN_MAC_CNTm, L2_LEARN_MAC_CNT_MAC_CNTf, &l2_learn_mac_cntn, &mac_cnt);
    *pcnt = mac_cnt;

    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_fdb_mc_cnt_get(yt_unit_t unit,  uint32 *pcnt)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_igmp_learn_group_cnt_t l2_igmp_learn_group_cnt;
    uint32_t group_cnt;
    
    CMM_PARAM_CHK((NULL == pcnt), CMM_ERR_NULL_POINT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_IGMP_LEARN_GROUP_CNTm, 0, sizeof(l2_igmp_learn_group_cnt), &l2_igmp_learn_group_cnt), ret);
    HAL_FIELD_GET(L2_IGMP_LEARN_GROUP_CNTm, L2_IGMP_LEARN_GROUP_CNT_CNTf, &l2_igmp_learn_group_cnt, &group_cnt);
    *pcnt = group_cnt;


    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_fdb_aging_port_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_aging_per_port_ctrl_t l2_aging_per_port_ctrl;
    yt_macid_t macid;
    uint32_t portmask;
    
    macid = CAL_YTP_TO_MAC(unit, port);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_AGING_PER_PORT_CTRLm, 0, sizeof(l2_aging_per_port_ctrl), &l2_aging_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_AGING_PER_PORT_CTRLm, L2_AGING_PER_PORT_CTRL_AGING_ENf, &l2_aging_per_port_ctrl, &portmask);
    if (enable == YT_ENABLE)
    {
        SET_BIT(portmask, macid);
    }
    else if (enable == YT_DISABLE)
    {
        CLEAR_BIT(portmask, macid);
    }
    else
    {
        return CMM_ERR_INPUT;
    }
    HAL_FIELD_SET(L2_AGING_PER_PORT_CTRLm, L2_AGING_PER_PORT_CTRL_AGING_ENf, &l2_aging_per_port_ctrl, portmask);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_AGING_PER_PORT_CTRLm, 0, sizeof(l2_aging_per_port_ctrl), &l2_aging_per_port_ctrl), ret);

    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_fdb_aging_port_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_aging_per_port_ctrl_t l2_aging_per_port_ctrl;
    yt_macid_t macid;
    uint32_t port_mask;

    macid = CAL_YTP_TO_MAC(unit,port);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_AGING_PER_PORT_CTRLm, 0, sizeof(l2_aging_per_port_ctrl), &l2_aging_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_AGING_PER_PORT_CTRLm, L2_AGING_PER_PORT_CTRL_AGING_ENf, &l2_aging_per_port_ctrl, &port_mask);
    *pEnable = IS_BIT_SET(port_mask, macid) ? YT_ENABLE : YT_DISABLE;
    
    return CMM_ERR_OK; 
}


yt_ret_t  fal_tiger_l2_fdb_aging_time_set(yt_unit_t unit,  uint32_t sec)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_aging_ctrl_t l2_aging_ctrl;
    uint32_t age_time;

    osal_memset(&l2_aging_ctrl, 0, sizeof(l2_aging_ctrl));

    if(sec > 5*65535)
        return CMM_ERR_INPUT;

    if(FDB_STATUS_MAX_TIME > sec)
    {
        age_time = 1;
    }
    else
    {
        age_time  = (sec / FDB_STATUS_MAX_TIME);
    }
    HAL_FIELD_SET(L2_AGING_CTRLm, L2_AGING_CTRL_AGING_INTERVALf, &l2_aging_ctrl, age_time);
       
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_AGING_CTRLm, 0, sizeof(l2_aging_ctrl), &l2_aging_ctrl), ret);

    return CMM_ERR_OK; 
}

yt_ret_t  fal_tiger_l2_fdb_aging_time_get(yt_unit_t unit,  uint32_t *psec)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_aging_ctrl_t l2_aging_ctrl;
    uint32_t age_time;

    osal_memset(&l2_aging_ctrl, 0, sizeof(l2_aging_ctrl));

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_AGING_CTRLm, 0, sizeof(l2_aging_ctrl), &l2_aging_ctrl), ret);
    HAL_FIELD_GET(L2_AGING_CTRLm, L2_AGING_CTRL_AGING_INTERVALf, &l2_aging_ctrl, &age_time);
    *psec = age_time * FDB_STATUS_MAX_TIME;

    return CMM_ERR_OK; 
}

static  uint32_t  fal_tiger_l2_op_result_get(yt_unit_t unit,  yt_l2_fdb_op_result_t *pop_result)
{
    l2_fdb_tbl_op_result_t l2_fdb_tbl_op_result;
    uint32_t op_node;
    uint32_t lookup_fail;
    uint16_t l2_fdb_tbl_op_busy_cnt = FDB_BUSY_CHECK_NUMBER;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    while (l2_fdb_tbl_op_busy_cnt)
    {
        osal_memset(&l2_fdb_tbl_op_result, 0, sizeof(l2_fdb_tbl_op_result));
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_RESULTm, 0, sizeof(l2_fdb_tbl_op_result), &l2_fdb_tbl_op_result), ret);
        HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_OP_DONEf, &l2_fdb_tbl_op_result, &op_node);
        if(TRUE == op_node)
        {
            HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_LOOKUP_FAILf, &l2_fdb_tbl_op_result, &lookup_fail);
            HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_ENTRY_INDEXf, &l2_fdb_tbl_op_result, &pop_result->entry_idx);
            pop_result->lookup_fail = lookup_fail;
            pop_result->op_done     = TRUE;
            break;
        }
        
        l2_fdb_tbl_op_busy_cnt--;
        
        if(0 == l2_fdb_tbl_op_busy_cnt)
        {
            pop_result->lookup_fail = TRUE;
            pop_result->op_done     = FALSE;
                
            return CMM_ERR_FDB_OP_BUSY;
        }
        
    }
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_op_info_get(yt_unit_t unit,  yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info)
{
  
    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;
    cmm_err_t ret           = CMM_ERR_OK;
    uint32_t macAddr0;
    uint32_t macAddr1;
    uint32_t status;
    uint32_t dmac_int_pri_en;
    uint32_t fid;
    uint32_t new_vid;
    uint32_t int_pri;
    uint32_t smac_int_pri_en;
    uint32_t copy_to_cpu;
    uint32_t dmac_drop;
    uint32_t dst_port_mask;
    uint32_t smac_drop;
    
    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);
    
    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_0_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_1_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_2_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);
    
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_STATUSf, &l2_fdb_tbl_op_data_1, &status);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_DMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_1, &dmac_int_pri_en);
    pfdb_info->STATUS           = status;
    pfdb_info->DMAC_INT_PRI_EN  = dmac_int_pri_en;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_NEW_VIDf, &l2_fdb_tbl_op_data_2, &new_vid);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_INT_PRIf, &l2_fdb_tbl_op_data_2, &int_pri);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_2, &smac_int_pri_en);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_COPY_TO_CPUf, &l2_fdb_tbl_op_data_2, &copy_to_cpu);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_DMAC_DROPf, &l2_fdb_tbl_op_data_2, &dmac_drop);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_DST_PORT_MASKf, &l2_fdb_tbl_op_data_2, &dst_port_mask);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_DROPf, &l2_fdb_tbl_op_data_2, &smac_drop);
    pfdb_info->NEW_VID         = new_vid;
    pfdb_info->INT_PRI          = int_pri;
    pfdb_info->SMAC_INT_PRI_EN  = smac_int_pri_en;
    pfdb_info->COPY_TO_CPU      = copy_to_cpu;
    pfdb_info->DMAC_DROP        = dmac_drop;
    pfdb_info->DST_PORT_MASK    = dst_port_mask;
    pfdb_info->SMAC_DROP        = smac_drop;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_FIDf, &l2_fdb_tbl_op_data_1, &fid);
    *pfid = fid;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_0_DUMMYm, L2_FDB_TBL_OP_DATA_0_DUMMY_MAC_DA_0f, &l2_fdb_tbl_op_data_0, &macAddr0);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_MAC_DA_1f, &l2_fdb_tbl_op_data_1, &macAddr1);
    pmac_addr->addr[0] = ((macAddr0 & 0xFF000000) >> 24);
    pmac_addr->addr[1] = ((macAddr0 & 0xFF0000) >> 16) ;
    pmac_addr->addr[2] = ((macAddr0 & 0xFF00) >> 8);
    pmac_addr->addr[3] = (macAddr0 & 0xFF);
    pmac_addr->addr[4] = ((macAddr1 & 0xFF00) >> 8);
    pmac_addr->addr[5] = (macAddr1 & 0xFF);
    
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_add_cfg(yt_unit_t unit, yt_l2_fdb_op_mode_t op_mode, 
                                                        l2_fdb_tbl_t l2_fdb, yt_l2_fdb_op_result_t *pop_result)
{
    l2_fdb_tbl_op_t l2_fdb_tbl_op;
    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;
    cmm_err_t ret           = CMM_ERR_OK;
    uint32_t macAddr0;
    uint32_t macAddr1;
    uint32_t status;
    uint32_t dmac_int_pri_en;
    uint32_t fid;
    uint32_t new_vid;
    uint32_t int_pri;
    uint32_t smac_int_pri_en;
    uint32_t copy_to_cpu;
    uint32_t dmac_drop;
    uint32_t dst_port_mask;
    uint32_t smac_drop;
    uint32_t move_aging_status;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));

    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_0f, &l2_fdb, &macAddr0);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_1f, &l2_fdb, &macAddr1);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_FIDf, &l2_fdb, &fid);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_STATUSf, &l2_fdb, &status);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_INT_PRI_ENf, &l2_fdb, &dmac_int_pri_en);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_NEW_VIDf, &l2_fdb, &new_vid);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_INT_PRIf, &l2_fdb, &int_pri);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_INT_PRI_ENf, &l2_fdb, &smac_int_pri_en);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_COPY_TO_CPUf, &l2_fdb, &copy_to_cpu);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DMAC_DROPf, &l2_fdb, &dmac_drop);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DST_PORT_MASKf, &l2_fdb, &dst_port_mask);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_SMAC_DROPf, &l2_fdb, &smac_drop);
    HAL_FIELD_GET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MOVE_AGING_STATUSf, &l2_fdb, &move_aging_status);

    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_0m, L2_FDB_TBL_OP_DATA_0_MAC_DA_0f, &l2_fdb_tbl_op_data_0, macAddr0);
    
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_MAC_DA_1f, &l2_fdb_tbl_op_data_1, macAddr1);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_FIDf, &l2_fdb_tbl_op_data_1, fid);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_STATUSf, &l2_fdb_tbl_op_data_1, status);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_DMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_1, dmac_int_pri_en);

    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_NEW_VIDf, &l2_fdb_tbl_op_data_2, new_vid);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_INT_PRIf, &l2_fdb_tbl_op_data_2, int_pri);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_SMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_2, smac_int_pri_en);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_COPY_TO_CPUf, &l2_fdb_tbl_op_data_2, copy_to_cpu);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_DMAC_DROPf, &l2_fdb_tbl_op_data_2, dmac_drop);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_DST_PORT_MASKf, &l2_fdb_tbl_op_data_2, dst_port_mask);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_SMAC_DROPf, &l2_fdb_tbl_op_data_2, smac_drop);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_MOVE_AGING_STATUSf, &l2_fdb_tbl_op_data_2, move_aging_status);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_0m, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_1m, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_2m, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);
    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_ADD);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, op_mode.l2_fdb_op_mode);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_ENTRY_INDEXf, &l2_fdb_tbl_op, op_mode.entry_idx);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);
    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_add(yt_unit_t unit,  yt_l2_fdb_op_mode_t op_mode,
            uint16_t fid, yt_mac_addr_t mac_addr, yt_port_mask_t port_mask,
            yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_t l2_fdb;
    yt_l2_fdb_info_t fdb_info;
    uint16_t lookup_index = 0;
    
    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb, 0, sizeof(l2_fdb));
    osal_memset(&fdb_info, 0, sizeof(yt_l2_fdb_info_t));

    if (fal_tiger_l2_fdb_get(unit, fid,  mac_addr,  &fdb_info, &lookup_index) == CMM_ERR_OK)
        CMM_ERR_CHK(fal_tiger_l2_tbl_read(unit, lookup_index, &l2_fdb),ret);

    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_0f, &l2_fdb, ((mac_addr.addr[0] << 24) | (mac_addr.addr[1] << 16) | (mac_addr.addr[2] << 8) | mac_addr.addr[3]));
    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_MAC_DA_1f, &l2_fdb, ((mac_addr.addr[4] << 8) | mac_addr.addr[5]));
    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_FIDf, &l2_fdb, fid);
    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_STATUSf, &l2_fdb, FDB_STATUS_STATIC);
    HAL_FIELD_SET(L2_FDB_TBL_BIN0m, L2_FDB_TBL_BIN0_DST_PORT_MASKf, &l2_fdb, port_mask.portbits[0]);
       
    CMM_ERR_CHK(fal_tiger_l2_fdb_op_add_cfg(unit, op_mode, l2_fdb, pop_result), ret);

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_result_t l2_fdb_tbl_op_result;
    l2_fdb_tbl_op_t l2_fdb_tbl_op;
    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));

    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_0m, L2_FDB_TBL_OP_DATA_0_MAC_DA_0f, &l2_fdb_tbl_op_data_0, ((mac_addr.addr[0] << 24) | (mac_addr.addr[1] << 16) | (mac_addr.addr[2] << 8) | mac_addr.addr[3]));
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_MAC_DA_1f, &l2_fdb_tbl_op_data_1, ((mac_addr.addr[4] << 8) | mac_addr.addr[5]));
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_FIDf, &l2_fdb_tbl_op_data_1, vid);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OP_DATA_0m, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OP_DATA_1m, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OP_DATA_2m, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);

    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_result));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_DEL);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_HASH);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);
    
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_get_one_withidx(yt_unit_t unit, uint16_t idx, 
                                                                    yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info,
                                                                    yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_t));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_GET_ONE);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_INDEX);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_ENTRY_INDEXf, &l2_fdb_tbl_op, idx);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_info_get(unit, pmac_addr, pfid,  pfdb_info), ret);
    
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_get_next_withidx(yt_unit_t unit, uint16_t idx, 
                                                                    yt_mac_addr_t *pmac_addr, uint16_t *pfid, uint16_t *pnext_index, 
                                                                    yt_l2_fdb_info_t *pfdb_info, yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pnext_index), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_t));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_GET_NEXT);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_GET_NEXT_TYPEf, &l2_fdb_tbl_op, FAL_TIGER_FDB_GET_NEXT_UCAST_ONE);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_INDEX);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_ENTRY_INDEXf, &l2_fdb_tbl_op, idx);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    if (pop_result->lookup_fail == TRUE)
        return CMM_ERR_ENTRY_NOT_FOUND;

    *pnext_index = pop_result->entry_idx;

    CMM_ERR_CHK(fal_tiger_l2_op_info_get(unit, pmac_addr, pfid,  pfdb_info), ret);
    
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_get_one(yt_unit_t unit,uint16_t fid, yt_mac_addr_t mac_addr, 
                                                        yt_l2_fdb_info_t *pfdb_info, yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_result_t l2_fdb_tbl_op_result;

    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));

    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_0m, L2_FDB_TBL_OP_DATA_0_MAC_DA_0f, &l2_fdb_tbl_op_data_0, ((mac_addr.addr[0] << 24) | (mac_addr.addr[1] << 16) | (mac_addr.addr[2] << 8) | mac_addr.addr[3]));
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_MAC_DA_1f, &l2_fdb_tbl_op_data_1, ((mac_addr.addr[4] << 8) | mac_addr.addr[5]));
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_FIDf, &l2_fdb_tbl_op_data_1, fid);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_0m, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_1m, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_2m, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);


    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_result));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_GET_ONE);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_HASH);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);
    
    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_info_get(unit, &mac_addr, &fid, pfdb_info), ret);

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_flush(yt_unit_t unit, yt_l2_tbl_flush_ctrl_t op_flush, yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_result_t l2_fdb_tbl_op_result;

    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));

    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_1m, L2_FDB_TBL_OP_DATA_1_FIDf, &l2_fdb_tbl_op_data_1, op_flush.fid);
    HAL_FIELD_SET(L2_FDB_TBL_OP_DATA_2m, L2_FDB_TBL_OP_DATA_2_DST_PORT_MASKf, &l2_fdb_tbl_op_data_2, op_flush.port_mask.portbits[0]);
      
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_0m, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_1m, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit,L2_FDB_TBL_OP_DATA_2m, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);


    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_result));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_FLUSH);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_FLUSH_MODEf, &l2_fdb_tbl_op, op_flush.mode);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_FLUSH_STATIC_ENf, &l2_fdb_tbl_op, op_flush.flush_static_en);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_fdb_uc_withindex_get(yt_unit_t unit, uint16_t index, l2_ucastMacAddr_info_t *pUcastMac)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_info_t fdb_info;
    yt_mac_addr_t mac_addr;
    yt_port_t port;
    yt_macid_t macid;
    uint16_t fid;

    if(CAL_L2_FDB_NUM_MAX <= index)
    {
        return CMM_ERR_EXCEED_RANGE;
    }

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_one_withidx(unit, index, &mac_addr, &fid, &fdb_info, &op_result),ret);

        if(FDB_STATUS_INVALID == fdb_info.STATUS ||
            IS_MCAST_ADDR(mac_addr.addr))
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }

        for(port =0; port < CAL_MAX_PORT_NUM; port++)
        {
            macid = CAL_YTP_TO_MAC(unit,port);
            if(IS_BIT_SET(fdb_info.DST_PORT_MASK, macid))
                break;
        }
        pUcastMac->port = port;
        
        pUcastMac->vid = fid;
        osal_memcpy(pUcastMac->macaddr.addr, mac_addr.addr, MAC_ADDR_LEN);
        switch(fdb_info.STATUS)
        {
            case FDB_STATUS_PENDING:
                pUcastMac->type = FDB_TYPE_PENDING;
                break;
            case FDB_STATUS_STATIC:
                pUcastMac->type = FDB_TYPE_STATIC;
                break;
            default:
                pUcastMac->type = FDB_TYPE_DYNAMIC;
                break;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_fdb_uc_withMacAndVid_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, l2_ucastMacAddr_info_t *pUcastMac)
{
    hal_reg_tbl_mode_t table_reg_mode;
    uint16_t lookup_index   = 0;
    cmm_err_t ret           = CMM_ERR_OK;
    yt_l2_fdb_info_t fdb_info;
    yt_port_t port;
    yt_macid_t macid;

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);
    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_get(unit, vid,  mac_addr, &fdb_info, &lookup_index), ret);

        if(FDB_STATUS_INVALID == fdb_info.STATUS ||
            IS_MCAST_ADDR(mac_addr.addr))
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }

        switch(fdb_info.STATUS)
        {
            case FDB_STATUS_PENDING:
                pUcastMac->type = FDB_TYPE_PENDING;
                break;
            case FDB_STATUS_STATIC:
                pUcastMac->type = FDB_TYPE_STATIC;
                break;
            default:
                pUcastMac->type = FDB_TYPE_DYNAMIC;
                break;
        }

        for(port =0; port < CAL_MAX_PORT_NUM; port++)
        {
            macid = CAL_YTP_TO_MAC(unit,port);
            if(IS_BIT_SET(fdb_info.DST_PORT_MASK, macid))
                break;
        }
        pUcastMac->port = port;
   }
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_fdb_uc_withindex_getnext(yt_unit_t unit, uint16_t index, uint16_t *pNext_index, l2_ucastMacAddr_info_t *pUcastMac)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_info_t fdb_info;
    yt_mac_addr_t mac_addr;
    yt_port_t port;
    yt_macid_t macid;
    uint16_t fid;

    if(CAL_L2_FDB_NUM_MAX - 1 <= index)
    {
        return CMM_ERR_EXCEED_RANGE;
    }

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_next_withidx(unit, index, &mac_addr, &fid, pNext_index, &fdb_info, &op_result),ret);

        if(FDB_STATUS_INVALID == fdb_info.STATUS ||
            IS_MCAST_ADDR(mac_addr.addr))
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }

        for(port =0; port < CAL_MAX_PORT_NUM; port++)
        {
            macid = CAL_YTP_TO_MAC(unit,port);
            if(IS_BIT_SET(fdb_info.DST_PORT_MASK, macid))
                break;
        }
        pUcastMac->port = port;

        pUcastMac->vid = fid;
        osal_memcpy(pUcastMac->macaddr.addr, mac_addr.addr, MAC_ADDR_LEN);
        switch(fdb_info.STATUS)
        {
            case FDB_STATUS_PENDING:
                pUcastMac->type = FDB_TYPE_PENDING;
                break;
            case FDB_STATUS_STATIC:
                pUcastMac->type = FDB_TYPE_STATIC;
                break;
            default:
                pUcastMac->type = FDB_TYPE_DYNAMIC;
                break;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_port_learn_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    /* disable learn fdb*/
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_DISABLEf, &l2_learn_per_port_ctrl, enable ? YT_DISABLE : YT_ENABLE);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_port_learn_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    uint32 enable;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_GET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_DISABLEf, &l2_learn_per_port_ctrl, &enable);
    *pEnable = enable ? YT_DISABLE : YT_ENABLE;

    return CMM_ERR_OK;
}
