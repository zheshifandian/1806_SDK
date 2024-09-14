/*
 * Include Files
 */
#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "fal_tiger_port_isolation.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"


yt_ret_t fal_tiger_port_isolation_set(yt_unit_t unit, yt_port_t port, yt_port_mask_t iso_portmask)
{
    l2_port_isolation_ctrln_t  entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid;
    yt_port_mask_t macmask;

    macid = CAL_YTP_TO_MAC(unit,port);
    CAL_YTPLIST_TO_MLIST(unit,iso_portmask, macmask);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_PORT_ISOLATION_CTRLNm, macid, sizeof(l2_port_isolation_ctrln_t), &entry), ret); 
    HAL_FIELD_SET(L2_PORT_ISOLATION_CTRLNm, L2_PORT_ISOLATION_CTRLN_ISOLATED_PORT_MASKf, &entry, macmask.portbits[0]);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_PORT_ISOLATION_CTRLNm, macid, sizeof(l2_port_isolation_ctrln_t), &entry), ret); 
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_port_isolation_get(yt_unit_t unit, yt_port_t port, yt_port_mask_t *pIso_portmask)
{
    l2_port_isolation_ctrln_t  entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid;
    yt_port_mask_t macmask;
    uint32_t portmask;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_CLEAR_MEMBER_PORT(macmask);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_PORT_ISOLATION_CTRLNm, macid, sizeof(l2_port_isolation_ctrln_t), &entry), ret); 
    HAL_FIELD_GET(L2_PORT_ISOLATION_CTRLNm, L2_PORT_ISOLATION_CTRLN_ISOLATED_PORT_MASKf, &entry, &portmask);
    macmask.portbits[0] = portmask;

    CAL_MLIST_TO_YTPLIST(unit, macmask, (*pIso_portmask));
    
    return CMM_ERR_OK;
}
