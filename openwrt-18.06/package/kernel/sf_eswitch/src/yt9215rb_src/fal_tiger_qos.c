/*
 * Include Files
 */
#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "fal_tiger_qos.h"
#include "fal_cmm.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"

static yt_ret_t fal_tiger_qos_ac_queue_flush(yt_unit_t unit, yt_qid_t qinfo);
static yt_ret_t fal_tiger_qos_ac_enqueue_disable(yt_unit_t unit, yt_qid_t qinfo, uint8_t disable);

yt_ret_t fal_tiger_qos_intPri_map_weight_set(yt_unit_t unit, yt_port_t port, yt_qos_intPri_map_weight_t pri_tbl)
{
    qos_merge_precedence_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.mac_sa_pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.mac_da_pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.vlan_pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.acl_pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.dscp_pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.cpri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.spri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri_tbl.port_pri, CMM_ERR_INPUT);
    
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_MAC_SA_INT_PRI_PRECEDENCEf, &entry, pri_tbl.mac_sa_pri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_MAC_DA_INT_PRI_PRECEDENCEf, &entry, pri_tbl.mac_da_pri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_VLAN_INT_PRI_PRECEDENCEf, &entry, pri_tbl.vlan_pri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_ACL_INT_PRI_PRECEDENCEf, &entry, pri_tbl.acl_pri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_DSCP2INT_PRI_PRECEDENCEf, &entry, pri_tbl.dscp_pri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_CPRI2INT_PRI_PRECEDENCEf, &entry, pri_tbl.cpri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_SPRI2INT_PRI_PRECEDENCEf, &entry, pri_tbl.spri);
    HAL_FIELD_SET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_PORT_PRI_PRECEDENCEf, &entry, pri_tbl.port_pri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_MERGE_PRECEDENCE_CTRLNm, macid,sizeof(qos_merge_precedence_ctrln_t), &entry), ret);
    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.src_port;

 */
yt_ret_t fal_tiger_qos_intPri_map_weight_get(yt_unit_t unit, yt_port_t port, yt_qos_intPri_map_weight_t *pri_tbl)
{
    qos_merge_precedence_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    uint32_t mac_sa_pri;
    uint32_t mac_da_pri;
    uint32_t vlan_pri;
    uint32_t acl_pri;
    uint32_t dscp_pri;
    uint32_t cpri;
    uint32_t spri;
    uint32_t port_pri;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_MERGE_PRECEDENCE_CTRLNm, macid, sizeof(qos_merge_precedence_ctrln_t), &entry), ret);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_MAC_SA_INT_PRI_PRECEDENCEf, &entry, &mac_sa_pri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_MAC_DA_INT_PRI_PRECEDENCEf, &entry, &mac_da_pri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_VLAN_INT_PRI_PRECEDENCEf, &entry, &vlan_pri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_ACL_INT_PRI_PRECEDENCEf, &entry, &acl_pri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_DSCP2INT_PRI_PRECEDENCEf, &entry, &dscp_pri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_CPRI2INT_PRI_PRECEDENCEf, &entry, &cpri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_SPRI2INT_PRI_PRECEDENCEf, &entry, &spri);
    HAL_FIELD_GET(QOS_MERGE_PRECEDENCE_CTRLNm, QOS_MERGE_PRECEDENCE_CTRLN_PORT_PRI_PRECEDENCEf, &entry, &port_pri);

    pri_tbl->mac_sa_pri = mac_sa_pri;
    pri_tbl->mac_da_pri = mac_da_pri;
    pri_tbl->vlan_pri = vlan_pri;
    pri_tbl->acl_pri = acl_pri;
    pri_tbl->dscp_pri = dscp_pri;
    pri_tbl->cpri = cpri;
    pri_tbl->spri = spri;
    pri_tbl->port_pri = port_pri;

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.src_port;

 */
yt_ret_t fal_tiger_qos_intPri_portDefPri_set (yt_unit_t unit, yt_port_t port, yt_enable_t enable, yt_pri_t pri)
{
    qos_port_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &entry), ret);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_PORT_INTPRIf, &entry, pri);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_PORT_INTPRI_ENf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &entry), ret);

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note];

 */
yt_ret_t  fal_tiger_qos_intPri_portDefPri_get (yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable, yt_pri_t *pPri)
{
    qos_port_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    uint32_t enable;
    uint32_t pri;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &entry), ret);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_PORT_INTPRIf, &entry, &pri);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_PORT_INTPRI_ENf, &entry, &enable);
    *pEnable = enable?YT_ENABLE:YT_DISABLE;
    *pPri = pri;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_cpri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_pri_t pri)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < cpri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < cpri.prio, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_CPRI_MAP_SELf, &port_entry, CPRI_MAP_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);

    index = (CPRI_MAP_INDEX << 4) + (cpri.dei<<3) + cpri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_SET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_PRIOf, &pri_entry, pri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_cpri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_pri_t *pPri)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_pri_index_t pri_map_sel;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    uint32_t pri;

    CMM_PARAM_CHK(TRUE < cpri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < cpri.prio, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_CPRI_MAP_SELf, &port_entry, &index);

    pri_map_sel = index;
    index = (pri_map_sel << 4) + (cpri.dei<<3) + cpri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_GET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_PRIOf, &pri_entry, &pri);
    *pPri = pri;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_spri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_pri_t pri)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < spri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < spri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_SPRI_MAP_SELf, &port_entry, SPRI_MAP_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);

    index = (SPRI_MAP_INDEX << 4) + (spri.dei<<3) + spri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_SET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_PRIOf, &pri_entry, pri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_spri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_pri_t *pPri)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_pri_index_t pri_map_sel;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t pri;

    CMM_PARAM_CHK(TRUE < spri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < spri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_SPRI_MAP_SELf, &port_entry, &index);
    pri_map_sel = index;
    
    index = (pri_map_sel << 4) + (spri.dei<<3) + spri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_GET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_PRIOf, &pri_entry, &pri);
    *pPri = pri;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_dscp_map_set(yt_unit_t unit, yt_dscp_t dscp, yt_pri_t pri)
{
    dscp_to_int_prio_map_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VALUE_DSCP(unit) < dscp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);
    HAL_FIELD_SET(DSCP_TO_INT_PRIO_MAPm, DSCP_TO_INT_PRIO_MAP_INT_PRIOf, &entry, pri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_dscp_map_get(yt_unit_t unit, yt_dscp_t dscp, yt_pri_t *pPri)
{
    dscp_to_int_prio_map_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t pri;

    CMM_PARAM_CHK(CAL_MAX_VALUE_DSCP(unit) < dscp, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);
    HAL_FIELD_GET(DSCP_TO_INT_PRIO_MAPm, DSCP_TO_INT_PRIO_MAP_INT_PRIOf, &entry, &pri);
    *pPri = pri;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_qos_intPri_vlan_map_set(yt_unit_t unit, yt_vlan_t vid, yt_enable_t enable, yt_pri_t pri)
{
    l2_vlan_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_VLAN_TBLm, vid,sizeof(l2_vlan_tbl_t), &entry), ret);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_INT_PRI_VALIDf, &entry, enable);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_INT_PRIf, &entry, pri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_VLAN_TBLm, vid, sizeof(l2_vlan_tbl_t), &entry), ret);
    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_qos_intPri_vlan_map_get(yt_unit_t unit, yt_vlan_t vid, yt_enable_t *pEnable, yt_pri_t *pPri)
{
    l2_vlan_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_VLAN_TBLm, vid,sizeof(l2_vlan_tbl_t), &entry), ret);
    HAL_FIELD_GET(L2_VLAN_TBLm, L2_VLAN_TBL_INT_PRI_VALIDf, &entry, pEnable);
    HAL_FIELD_GET(L2_VLAN_TBLm, L2_VLAN_TBL_INT_PRIf, &entry, pPri);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_map_igrMirror_set(yt_unit_t unit, yt_enable_t enable, yt_pri_t pri)
{
    mirror_qos_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_QOS_CTRLm, 0, sizeof(mirror_qos_ctrl_t), &entry), ret);
    HAL_FIELD_SET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_INGR_MIRROR_INT_PRIOf, &entry, pri);
    HAL_FIELD_SET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_INGR_MIRROR_INT_PRIO_VALIDf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, MIRROR_QOS_CTRLm, 0, sizeof(mirror_qos_ctrl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_map_igrMirror_get(yt_unit_t unit, yt_enable_t *pEnable, yt_pri_t *pPri)
{
    mirror_qos_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t pri;
    uint32_t enable;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_QOS_CTRLm, 0, sizeof(mirror_qos_ctrl_t), &entry), ret);
    HAL_FIELD_GET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_INGR_MIRROR_INT_PRIOf, &entry, &pri);
    HAL_FIELD_GET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_INGR_MIRROR_INT_PRIO_VALIDf, &entry, &enable);
    *pPri = pri;
    *pEnable = enable;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intPri_map_egrMirror_set(yt_unit_t unit, yt_enable_t enable, yt_pri_t pri)
{
    mirror_qos_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < pri, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_QOS_CTRLm, 0,sizeof(mirror_qos_ctrl_t), &entry), ret);
    HAL_FIELD_SET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_EGR_MIRROR_INT_PRIOf, &entry, pri);
    HAL_FIELD_SET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_EGR_MIRROR_INT_PRIO_VALIDf, &entry, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, MIRROR_QOS_CTRLm, 0,sizeof(mirror_qos_ctrl_t), &entry), ret);

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] mirror_process() in qm.c

 */
yt_ret_t fal_tiger_qos_intPri_map_egrMirror_get(yt_unit_t unit, yt_enable_t *pEnable, yt_pri_t *pPri)
{
    mirror_qos_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t pri;
    uint32_t enable;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_QOS_CTRLm, 0,sizeof(mirror_qos_ctrl_t), &entry), ret);
    HAL_FIELD_GET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_EGR_MIRROR_INT_PRIOf, &entry, &pri);
    HAL_FIELD_GET(MIRROR_QOS_CTRLm, MIRROR_QOS_CTRL_EGR_MIRROR_INT_PRIO_VALIDf, &entry, &enable);
    *pPri = pri;
    *pEnable = enable;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_cpri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_dp_t dp)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < dp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < cpri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < cpri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_CPRI_MAP_SELf, &port_entry, CPRI_MAP_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);

    index = (CPRI_MAP_INDEX << 4) + (cpri.dei<<3) + cpri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_SET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_DPf, &pri_entry, dp);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_cpri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_dp_t *pDp)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_pri_index_t pri_map_sel;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    uint32_t dp;

    CMM_PARAM_CHK(TRUE < cpri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < cpri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid, sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_CPRI_MAP_SELf, &port_entry, &index);

    pri_map_sel = index;
    index = (pri_map_sel << 4) + (cpri.dei<<3) + cpri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_GET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_DPf, &pri_entry, &dp);
    *pDp = dp;
        
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_spri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_dp_t dp)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < dp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < spri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < spri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_SET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_SPRI_MAP_SELf, &port_entry, SPRI_MAP_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);

    index = (SPRI_MAP_INDEX << 4) + (spri.dei<<3) + spri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_SET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_DPf, &pri_entry, dp);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_spri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_dp_t *pDp)
{
    qos_port_ctrln_t port_entry;
    pri_to_int_prio_map_t pri_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_pri_index_t pri_map_sel;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t dp;

    CMM_PARAM_CHK(TRUE < spri.dei, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < spri.prio, CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QOS_PORT_CTRLNm, macid,sizeof(qos_port_ctrln_t), &port_entry), ret);
    HAL_FIELD_GET(QOS_PORT_CTRLNm, QOS_PORT_CTRLN_SPRI_MAP_SELf, &port_entry, &index);

    pri_map_sel = index;
    index = (pri_map_sel << 4) + (spri.dei<<3) + spri.prio;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PRI_TO_INT_PRIO_MAPm, index,sizeof(pri_to_int_prio_map_t), &pri_entry), ret);
    HAL_FIELD_GET(PRI_TO_INT_PRIO_MAPm, PRI_TO_INT_PRIO_MAP_INT_DPf, &pri_entry, &dp);
	*pDp = dp;
	
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_dscp_map_set(yt_unit_t unit, yt_dscp_t dscp, yt_dp_t dp)
{
    dscp_to_int_prio_map_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_MAX_VALUE_DSCP(unit) < dscp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < dp, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);
    HAL_FIELD_SET(DSCP_TO_INT_PRIO_MAPm, DSCP_TO_INT_PRIO_MAP_INT_DPf, &entry, dp);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_intDP_dscp_map_get(yt_unit_t unit, yt_dscp_t dscp, yt_dp_t *pDp)
{
    dscp_to_int_prio_map_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t dp;

    CMM_PARAM_CHK(CAL_MAX_VALUE_DSCP(unit) < dscp, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, DSCP_TO_INT_PRIO_MAPm, dscp,sizeof(dscp_to_int_prio_map_t), &entry), ret);
    HAL_FIELD_GET(DSCP_TO_INT_PRIO_MAPm, DSCP_TO_INT_PRIO_MAP_INT_DPf, &entry, &dp);
    *pDp = dp;

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.src_port;

 */
yt_ret_t fal_tiger_qos_que_map_ucast_set(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t qmap_tbl)
{
    int_prio_to_ucast_qid_mapn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, INT_PRIO_TO_UCAST_QID_MAPNm, macid,sizeof(int_prio_to_ucast_qid_mapn_t), &entry), ret);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_0_TO_QIDf, &entry, qmap_tbl.pri0_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_1_TO_QIDf, &entry, qmap_tbl.pri1_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_2_TO_QIDf, &entry, qmap_tbl.pri2_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_3_TO_QIDf, &entry, qmap_tbl.pri3_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_4_TO_QIDf, &entry, qmap_tbl.pri4_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_5_TO_QIDf, &entry, qmap_tbl.pri5_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_6_TO_QIDf, &entry, qmap_tbl.pri6_qid);
    HAL_FIELD_SET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_7_TO_QIDf, &entry, qmap_tbl.pri7_qid);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, INT_PRIO_TO_UCAST_QID_MAPNm, macid,sizeof(int_prio_to_ucast_qid_mapn_t), &entry), ret);

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.src_port;

 */
yt_ret_t fal_tiger_qos_que_map_ucast_get(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t *qmap_tbl)
{
    int_prio_to_ucast_qid_mapn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t pri0_qid;
    uint32_t pri1_qid;
    uint32_t pri2_qid;
    uint32_t pri3_qid;
    uint32_t pri4_qid;
    uint32_t pri5_qid;
    uint32_t pri6_qid;
    uint32_t pri7_qid;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, INT_PRIO_TO_UCAST_QID_MAPNm, macid,sizeof(int_prio_to_ucast_qid_mapn_t), &entry), ret);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_0_TO_QIDf, &entry, &pri0_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_1_TO_QIDf, &entry, &pri1_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_2_TO_QIDf, &entry, &pri2_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_3_TO_QIDf, &entry, &pri3_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_4_TO_QIDf, &entry, &pri4_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_5_TO_QIDf, &entry, &pri5_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_6_TO_QIDf, &entry, &pri6_qid);
    HAL_FIELD_GET(INT_PRIO_TO_UCAST_QID_MAPNm, INT_PRIO_TO_UCAST_QID_MAPN_PRIO_7_TO_QIDf, &entry, &pri7_qid);

    qmap_tbl->pri0_qid = pri0_qid;
    qmap_tbl->pri1_qid = pri1_qid;
    qmap_tbl->pri2_qid = pri2_qid;
    qmap_tbl->pri3_qid = pri3_qid;
    qmap_tbl->pri4_qid = pri4_qid;
    qmap_tbl->pri5_qid = pri5_qid;
    qmap_tbl->pri6_qid = pri6_qid;
    qmap_tbl->pri7_qid = pri7_qid;

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.dest_port;

 */
yt_ret_t fal_tiger_qos_que_map_mcast_set(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t qmap_tbl)
{
    int_prio_to_mcast_qid_mapn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_0_TO_QIDf, &entry, qmap_tbl.pri0_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_1_TO_QIDf, &entry, qmap_tbl.pri1_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_2_TO_QIDf, &entry, qmap_tbl.pri2_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_3_TO_QIDf, &entry, qmap_tbl.pri3_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_4_TO_QIDf, &entry, qmap_tbl.pri4_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_5_TO_QIDf, &entry, qmap_tbl.pri5_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_6_TO_QIDf, &entry, qmap_tbl.pri6_qid);
    HAL_FIELD_SET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_7_TO_QIDf, &entry, qmap_tbl.pri7_qid);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, INT_PRIO_TO_MCAST_QID_MAPNm, macid,sizeof(int_prio_to_mcast_qid_mapn_t), &entry), ret);
    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] l2_resolution: acl_l2_src_pn.dest_port;

 */
yt_ret_t fal_tiger_qos_que_map_mcast_get(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t *qmap_tbl)
{
    int_prio_to_mcast_qid_mapn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t pri0_qid;
    uint32_t pri1_qid;
    uint32_t pri2_qid;
    uint32_t pri3_qid;
    uint32_t pri4_qid;
    uint32_t pri5_qid;
    uint32_t pri6_qid;
    uint32_t pri7_qid;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, INT_PRIO_TO_MCAST_QID_MAPNm, macid,sizeof(int_prio_to_mcast_qid_mapn_t), &entry), ret);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_0_TO_QIDf, &entry, &pri0_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_1_TO_QIDf, &entry, &pri1_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_2_TO_QIDf, &entry, &pri2_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_3_TO_QIDf, &entry, &pri3_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_4_TO_QIDf, &entry, &pri4_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_5_TO_QIDf, &entry, &pri5_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_6_TO_QIDf, &entry, &pri6_qid);
    HAL_FIELD_GET(INT_PRIO_TO_MCAST_QID_MAPNm, INT_PRIO_TO_MCAST_QID_MAPN_PRIO_7_TO_QIDf, &entry, &pri7_qid);

    qmap_tbl->pri0_qid = pri0_qid;
    qmap_tbl->pri1_qid = pri1_qid;
    qmap_tbl->pri2_qid = pri2_qid;
    qmap_tbl->pri3_qid = pri3_qid;
    qmap_tbl->pri4_qid = pri4_qid;
    qmap_tbl->pri5_qid = pri5_qid;
    qmap_tbl->pri6_qid = pri6_qid;
    qmap_tbl->pri7_qid = pri7_qid;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_que_forceDropPerQue_enable_set(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t enable)
{
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, qinfo.port);
    uint32_t regaddr;
    uint32_t regval;
    uint32_t regval2;
    uint32_t bit;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(FAL_MAX_PORT_NUM < macid, CMM_ERR_INPUT);

    if (UNICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_UCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        regaddr = QOS_FORCEAC_UCASTQUE_REG(unit, macid, qinfo.qid);
        CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regaddr, &regval2), ret);
        regaddr += 4;
        bit = 26;
    }
    else if(MULTICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_MCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        regaddr = QOS_FORCEAC_MCASTQUE_REG(unit, macid, qinfo.qid);
        bit = 27;
    }
    else
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regaddr, &regval), ret);
    if (enable)
    {
        SET_BIT(regval, bit);
    }
    else
    {
        CLEAR_BIT(regval, bit);
    }

    if (UNICAST_QUEUE == qinfo.qtype)
    {
        CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regaddr - 4, regval2), ret);
    }
    CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regaddr, regval), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_que_forceDropPerQue_enable_get(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t *pEnable)
{
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, qinfo.port);
    uint32_t regaddr;
    uint32_t regval;
    uint32_t bit;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(FAL_MAX_PORT_NUM < macid, CMM_ERR_INPUT);

    if (UNICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_UCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        regaddr = QOS_FORCEAC_UCASTQUE_REG(unit, macid, qinfo.qid) + 4;
        bit = 26;
    }
    else if(MULTICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_MCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        regaddr = QOS_FORCEAC_MCASTQUE_REG(unit, macid, qinfo.qid);
        bit = 27;
    }
    else
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regaddr, &regval), ret);
    *pEnable = IS_BIT_SET(regval, bit) ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_que_forceDrop_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    yt_qid_t qinfo;
    uint32_t i;
    cmm_err_t ret = CMM_ERR_OK;

    qinfo.port = port;
    qinfo.qtype = UNICAST_QUEUE;
    for (i = 0; i < CAL_MAX_UCAST_QUEUE_NUM(unit); i++)
    {
        qinfo.qid = i;
        CMM_ERR_CHK(fal_tiger_qos_que_forceDropPerQue_enable_set(unit, qinfo, enable), ret);
    }

    qinfo.qtype = MULTICAST_QUEUE;
    for (i = 0; i < CAL_MAX_MCAST_QUEUE_NUM(unit); i++)
    {
        qinfo.qid = i;
        CMM_ERR_CHK(fal_tiger_qos_que_forceDropPerQue_enable_set(unit, qinfo, enable), ret);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_que_forceDrop_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    yt_qid_t qinfo;
    uint32_t ret;

    qinfo.port = port;
    qinfo.qtype = UNICAST_QUEUE;
    qinfo.qid = 0;
    CMM_ERR_CHK(fal_tiger_qos_que_forceDropPerQue_enable_get(unit, qinfo, pEnable), ret);

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] epe: epe_vlan_tag_proc;

 */
yt_ret_t fal_tiger_qos_remark_port_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_en_t rmark_en)
{
    egr_port_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_SPRIO_REMARK_ENf, &entry, rmark_en.spri_en);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_DEI_REMARK_ENf, &entry, rmark_en.sdei_en);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CPRIO_REMARK_ENf, &entry, rmark_en.cpri_en);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CFI_REMARK_ENf, &entry, rmark_en.ccfi_en);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_DSCP_REMARK_ENf, &entry, rmark_en.dscp_en);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_remark_port_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_en_t *pRmark_en)
{
    egr_port_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t spri_en;
    uint32_t sdei_en;
    uint32_t cpri_en;
    uint32_t ccfi_en;
    uint32_t dscp_en;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_SPRIO_REMARK_ENf, &entry, &spri_en);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_DEI_REMARK_ENf, &entry,   &sdei_en);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CPRIO_REMARK_ENf, &entry, &cpri_en);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CFI_REMARK_ENf, &entry,   &ccfi_en);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_DSCP_REMARK_ENf, &entry,  &dscp_en);
    
    pRmark_en->spri_en = spri_en;
    pRmark_en->sdei_en = sdei_en;
    pRmark_en->cpri_en = cpri_en;
    pRmark_en->ccfi_en = ccfi_en;
    pRmark_en->dscp_en = dscp_en;

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] epe: epe_vlan_tag_proc;

 */
yt_ret_t fal_tiger_qos_remark_dscp_set(yt_unit_t unit, yt_qos_remark_info_t srcInfo, yt_dscp_t new_dscp)
{
    egr_dscp_remarkn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t index = 0;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_DSCP(unit) < new_dscp, CMM_ERR_INPUT);
    
    index = (srcInfo.prio << 2)| srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_DSCP_REMARKNm, index, sizeof(egr_dscp_remarkn_t), &entry), ret);
    HAL_FIELD_SET(EGR_DSCP_REMARKNm, EGR_DSCP_REMARKN_INT_PRIO_TO_DSCPf, &entry, new_dscp);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_DSCP_REMARKNm, index, sizeof(egr_dscp_remarkn_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_remark_dscp_get(yt_unit_t unit, yt_qos_remark_info_t srcInfo, yt_dscp_t *pNew_dscp)
{
    egr_dscp_remarkn_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t index = 0;
    uint32_t new_dscp;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    
    index = (srcInfo.prio << 2)| srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_DSCP_REMARKNm, index, sizeof(egr_dscp_remarkn_t), &entry), ret);
    HAL_FIELD_GET(EGR_DSCP_REMARKNm, EGR_DSCP_REMARKN_INT_PRIO_TO_DSCPf, &entry, &new_dscp);
    *pNew_dscp = new_dscp;

    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] epe: epe_vlan_tag_proc;

 */
yt_ret_t fal_tiger_qos_remark_cpri_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t dstInfo)
{
    egr_port_ctrln_t entry;
    egr_prio_remarkn_t remark_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < dstInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < dstInfo.dei, CMM_ERR_INPUT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CPRIO_REMARK_SELf, &entry, CPRI_REMARK_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);

    index = (CPRI_REMARK_INDEX << 5) | (srcInfo.prio << 2) | srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);
    HAL_FIELD_SET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_DEIf, &remark_entry, dstInfo.dei);
    HAL_FIELD_SET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_PRIOf, &remark_entry, dstInfo.prio);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_remark_cpri_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t *pDstInfo)
{
    egr_port_ctrln_t entry;
    egr_prio_remarkn_t remark_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_remark_index_t remark_index;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t new_pri;
    uint32_t new_dei;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CPRIO_REMARK_SELf, &entry, &index);

    remark_index = index;
    index = (remark_index << 5) | (srcInfo.prio << 2) | srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);
    HAL_FIELD_GET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_DEIf, &remark_entry, &new_dei);
    HAL_FIELD_GET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_PRIOf, &remark_entry, &new_pri);
    pDstInfo->prio = new_pri;
    pDstInfo->dei = new_dei;
    
    return CMM_ERR_OK;
}

/*
 [Function Name]

 [Input/Output]

 [Description]

 [Return]

 [Note] epe: epe_vlan_tag_proc;

 */
yt_ret_t fal_tiger_qos_remark_spri_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t dstInfo)
{
    egr_port_ctrln_t entry;
    egr_prio_remarkn_t remark_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < dstInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(TRUE < dstInfo.dei, CMM_ERR_INPUT);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_SPRIO_REMARK_SELf, &entry, SPRI_REMARK_INDEX);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);

    index = (SPRI_REMARK_INDEX << 5) | (srcInfo.prio << 2) | srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);
    HAL_FIELD_SET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_DEIf, &remark_entry, dstInfo.dei);
    HAL_FIELD_SET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_PRIOf, &remark_entry, dstInfo.prio);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_remark_spri_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t *pDstInfo)
{
    egr_port_ctrln_t entry;
    egr_prio_remarkn_t remark_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_remark_index_t remark_index;
    uint32_t index;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    uint32_t new_pri;
    uint32_t new_dei;

    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_PRI(unit) < srcInfo.prio, CMM_ERR_INPUT);
    CMM_PARAM_CHK(CAL_MAX_VALUE_INT_DROP(unit) < srcInfo.dp, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PORT_CTRLNm, macid, sizeof(egr_port_ctrln_t), &entry), ret);
    HAL_FIELD_GET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_SPRIO_REMARK_SELf, &entry, &index);

    remark_index = index;
    index = (remark_index << 5) | (srcInfo.prio << 2) | srcInfo.dp;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_PRIO_REMARKNm, index,sizeof(egr_prio_remarkn_t), &remark_entry), ret);
    HAL_FIELD_GET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_DEIf, &remark_entry, &new_dei);
    HAL_FIELD_GET(EGR_PRIO_REMARKNm, EGR_PRIO_REMARKN_INT_PRIO_TO_PRIOf, &remark_entry, &new_pri);
    pDstInfo->prio = new_pri;
    pDstInfo->dei = new_dei;
    
    return CMM_ERR_OK;
}

uint32_t fal_tiger_qos_sch_tableId_get(yt_unit_t unit, yt_qid_t qinfo, uint8_t *pId)
{
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, qinfo.port);

    CMM_PARAM_CHK(FAL_MAX_PORT_NUM < macid, CMM_ERR_INPUT);

    if (UNICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_UCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        *pId = macid * CAL_MAX_UCAST_QUEUE_NUM(unit) + qinfo.qid;
    }
    else if(MULTICAST_QUEUE == qinfo.qtype)
    {
        CMM_PARAM_CHK(CAL_MAX_MCAST_QUEUE_NUM(unit) < qinfo.qid, CMM_ERR_INPUT);
        *pId = macid  * CAL_MAX_MCAST_QUEUE_NUM(unit) + qinfo.qid + (FAL_MAX_PORT_NUM * CAL_MAX_UCAST_QUEUE_NUM(unit));
    }
    else
    {
        return CMM_ERR_INPUT;
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_sp_set(yt_unit_t unit, yt_qid_t qinfo, yt_queue_pri_t qpri)
{
    qsch_flow_map_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(fal_tiger_qos_ac_enqueue_disable(unit, qinfo, TRUE), ret);
    CMM_ERR_CHK(fal_tiger_qos_ac_queue_flush(unit, qinfo), ret);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);
    HAL_FIELD_SET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_C_PRIf, &entry, qpri);
    HAL_FIELD_SET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_E_PRIf, &entry, qpri);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);

    CMM_ERR_CHK(fal_tiger_qos_ac_enqueue_disable(unit, qinfo, FALSE), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_sp_get(yt_unit_t unit, yt_qid_t qinfo, yt_queue_pri_t *pQpri)
{
    qsch_flow_map_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    uint32_t qpri;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);
    HAL_FIELD_GET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_C_PRIf, &entry, &qpri);
    *pQpri = qpri;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_dwrr_mode_set(yt_unit_t unit, yt_qid_t qinfo, yt_rate_mode_t dwrr_cmode)
{
    qsch_c_dwrr_cfg_tbl_t c_entry;
    qsch_e_dwrr_cfg_tbl_t e_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_queue_pri_t pri = 0;
    uint32_t idx;

    CMM_PARAM_CHK(RATE_MODE_PACKET < dwrr_cmode, CMM_ERR_INPUT);

    if(fal_tiger_qos_schedule_sp_get(unit, qinfo, &pri) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    idx = (CAL_YTP_TO_MAC(unit, qinfo.port) * (CAL_MAX_UCAST_QUEUE_NUM(unit) + CAL_MAX_MCAST_QUEUE_NUM(unit))) + pri;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_C_DWRR_CFG_TBLm, idx, sizeof(qsch_c_dwrr_cfg_tbl_t), &c_entry), ret);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_E_DWRR_CFG_TBLm, idx, sizeof(qsch_e_dwrr_cfg_tbl_t), &e_entry), ret);
    HAL_FIELD_SET(QSCH_C_DWRR_CFG_TBLm, QSCH_C_DWRR_CFG_TBL_DWRR_MODEf, &c_entry, dwrr_cmode);
    HAL_FIELD_SET(QSCH_E_DWRR_CFG_TBLm, QSCH_E_DWRR_CFG_TBL_DWRR_MODEf, &e_entry, dwrr_cmode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_C_DWRR_CFG_TBLm, idx, sizeof(qsch_c_dwrr_cfg_tbl_t), &c_entry), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_E_DWRR_CFG_TBLm, idx, sizeof(qsch_e_dwrr_cfg_tbl_t), &e_entry), ret);


    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_dwrr_mode_get(yt_unit_t unit, yt_qid_t qinfo, yt_rate_mode_t *pDwrr_cmode)
{
    qsch_c_dwrr_cfg_tbl_t c_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_queue_pri_t pri = 0;
    uint32_t idx;
    uint32_t dwrr_cmode;

    if(fal_tiger_qos_schedule_sp_get(unit, qinfo, &pri) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    idx = (CAL_YTP_TO_MAC(unit, qinfo.port) * (CAL_MAX_UCAST_QUEUE_NUM(unit) + CAL_MAX_MCAST_QUEUE_NUM(unit))) + pri;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_C_DWRR_CFG_TBLm, idx, sizeof(qsch_c_dwrr_cfg_tbl_t), &c_entry), ret);
    HAL_FIELD_GET(QSCH_C_DWRR_CFG_TBLm, QSCH_C_DWRR_CFG_TBL_DWRR_MODEf, &c_entry, &dwrr_cmode);
    *pDwrr_cmode = dwrr_cmode;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_dwrr_set(yt_unit_t unit, yt_qid_t qinfo, yt_queue_weight_t qweight)
{
    qsch_flow_map_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);
    HAL_FIELD_SET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_C_DWRR_WEIGHTf, &entry, qweight);
    HAL_FIELD_SET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_E_DWRR_WEIGHTf, &entry, qweight);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_qos_schedule_dwrr_get(yt_unit_t unit, yt_qid_t qinfo, yt_queue_weight_t *pQweight)
{
    qsch_flow_map_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;
    uint32_t qweight;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, QSCH_FLOW_MAP_TBLm, idx, sizeof(qsch_flow_map_tbl_t), &entry), ret);
    HAL_FIELD_GET(QSCH_FLOW_MAP_TBLm, QSCH_FLOW_MAP_TBL_C_DWRR_WEIGHTf, &entry, &qweight);
    *pQweight = qweight;

    return CMM_ERR_OK;
}

static yt_ret_t fal_tiger_qos_ac_queue_flush(yt_unit_t unit, yt_qid_t qinfo)
{
    flush_cfg_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t flush_qid = 0;
    uint32_t flush_done = 0;
    uint16_t busy_cnt = DONE_CHECK_NUMBER;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &flush_qid) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    HAL_FIELD_SET(FLUSH_CFGm, FLUSH_CFG_FLUSH_DONEf, &entry, 0);
    HAL_FIELD_SET(FLUSH_CFGm, FLUSH_CFG_FLUSH_REQf, &entry, 1);
    HAL_FIELD_SET(FLUSH_CFGm, FLUSH_CFG_FLUSH_QIDf, &entry, flush_qid);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, FLUSH_CFGm, 0, sizeof(flush_cfg_t), &entry), ret);
 
    while (busy_cnt)
    {
        flush_done = 0;
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, FLUSH_CFGm, 0, sizeof(flush_cfg_t), &entry), ret);
        HAL_FIELD_GET(FLUSH_CFGm, FLUSH_CFG_FLUSH_DONEf, &entry, &flush_done);
        if(flush_done)
        {
            break;
        }        
        
        busy_cnt--;
        
        if(0 == busy_cnt)
        {
            return CMM_ERR_FAIL;
        }
    }

    return CMM_ERR_OK;
}

static yt_ret_t fal_tiger_qos_ac_enqueue_disable(yt_unit_t unit, yt_qid_t qinfo, uint8_t disable)
{
    oq_enq_dis_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t idx = 0;

    if(fal_tiger_qos_sch_tableId_get(unit, qinfo, &idx) != CMM_ERR_OK)
    {
        return CMM_ERR_INPUT;
    }

    HAL_FIELD_SET(OQ_ENQ_DIS_TBLm, OQ_ENQ_DIS_TBL_ENQ_DISf, &entry, disable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, OQ_ENQ_DIS_TBLm, idx, sizeof(oq_enq_dis_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}
