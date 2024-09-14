/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_dos.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_DOS_H
#define __YT_DOS_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_dos_type_e {
    DOS_TYPE_DAEQSA,
    DOS_TYPE_LAND,
    DOS_TYPE_ICMP_PRAG,
    DOS_TYPE_TCP_FRAG_OFFSET,
    DOS_TYPE_LARGE_ICMPV4,
    DOS_TYPE_LARGE_ICMPV6,
    DOS_TYPE_TCP_HEADER_PARTIAL,
    DOS_TYPE_CFI_MISMATCH_DROP,
    DOS_TYPE_SA_MC_DROP,
    DOS_TYPE_SA_BC_DROP,
    DOS_TYPE_SA_ZERO_DROP,
    DOS_TYPE_DA_ZERO_DROP,
    DOS_TYPE_TCP_SYNRST_SCAN,   /*SYN and RST bits are set*/
    DOS_TYPE_TCP_SYNFIN_SCAN,   /*SYN and FIN bits are set*/
    DOS_TYPE_TCP_XMAS_SCAN, /*TCP sequence number is zero and the FIN, URG and PSH bits are set*/
    DOS_TYPE_TCP_NULL_SCAN, /*TCP sequence number is zero and all control bits are zeros*/
    DOS_TYPE_TCP_SYN_PORTLESS1024,  /*SYN bit is 1, ACK bit is 0, and Source-Port is 0~1023*/
    DOS_TYPE_TCP_ALLFLAGS_SCAN, /*All control bits are set*/
}yt_dos_type_t;

typedef enum yt_dos_action_e {
    DOS_ACTION_DROP,
    DOS_ACTION_COPY,
    DOS_ACTION_TRAP,
    DOS_ACTION_NONE,
}yt_dos_action_t;

typedef enum yt_dos_icmp_version_e {
    DOS_ICMP_VERSION_4,
    DOS_ICMP_VERSION_6,
}yt_dos_icmp_version_t;


/**
 * @internal      yt_dos_init
 * @endinternal
 *
 * @brief         init dos function module
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_dos_init(yt_unit_t unit);

/**
 * @internal      yt_dos_port_en_set
 * @endinternal
 *
 * @brief         enable or disable dos on specific port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_dos_port_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_dos_port_en_get
 * @endinternal
 *
 * @brief         get enable state of dos on specific port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_dos_port_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_dos_drop_en_set
 * @endinternal
 *
 * @brief         enable drop for specific dos
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -type defined of dos
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_dos_drop_en_set(yt_unit_t unit, yt_dos_type_t type, yt_enable_t enable);


/**
 * @internal      yt_dos_drop_en_get
 * @endinternal
 *
 * @brief         get drop state of specific dos
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -type defined of dos
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_dos_drop_en_get(yt_unit_t unit, yt_dos_type_t type, yt_enable_t *pEnable);


/**
 * @internal      yt_dos_large_icmp_size_set
 * @endinternal
 *
 * @brief         set largest size of icmp packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     ver                 -icmp version
 * @param[in]     size                -largest size,maximum 8191
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_dos_large_icmp_size_set(yt_unit_t unit, yt_dos_icmp_version_t ver, uint16_t size);


/**
 * @internal      yt_dos_large_icmp_size_get
 * @endinternal
 *
 * @brief         get largest size of icmp packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     ver                 -icmp version
 * @param[out]    psize               -largest size
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_dos_large_icmp_size_get(yt_unit_t unit, yt_dos_icmp_version_t ver, uint16_t *psize);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
