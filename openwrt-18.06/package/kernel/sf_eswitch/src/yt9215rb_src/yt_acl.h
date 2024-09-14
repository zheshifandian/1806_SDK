/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_acl.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_ACL_H__
#define __YT_ACL_H__


#include "yt_cmm.h"
#include "yt_vlan_translate.h"
/**************************************************
 *      Constants or macros Declaration                          *
 **************************************************/
#define ACL_UDF_DATA_LEN_MAX    4
#define ACL_UDF_MAX_NUM   16
/**************************************************
 *      Global variables                                                   *
 **************************************************/
typedef enum yt_igrAcl_key_type_e
{
    YT_IGRACL_TEMPLATE_MAC_DA=1,
    YT_IGRACL_TEMPLATE_MAC_SA,
    YT_IGRACL_TEMPLATE_L2_TYPE,
    YT_IGRACL_TEMPLATE_L3_TYPE,
    YT_IGRACL_TEMPLATE_CDEI,

    YT_IGRACL_TEMPLATE_CPRI=6,
    YT_IGRACL_TEMPLATE_CTAG_FMT,
    YT_IGRACL_TEMPLATE_SDEI,
    YT_IGRACL_TEMPLATE_SPRI,
    YT_IGRACL_TEMPLATE_STAG_FMT,

    YT_IGRACL_TEMPLATE_SVID=11,
    YT_IGRACL_TEMPLATE_CVID,
    YT_IGRACL_TEMPLATE_IPV4_DA,
    YT_IGRACL_TEMPLATE_IPV4_SA,
    YT_IGRACL_TEMPLATE_L4_DPORT,

    YT_IGRACL_TEMPLATE_L4_SPORT=16,
    YT_IGRACL_TEMPLATE_L4_TYPE,
    YT_IGRACL_TEMPLATE_IP_FRAGMENT,
    YT_IGRACL_TEMPLATE_IP_1ST_FRAGMENT,
    YT_IGRACL_TEMPLATE_IPV6_DA,

    YT_IGRACL_TEMPLATE_IPV6_SA=21,
    YT_IGRACL_TEMPLATE_IP_OPTION,
    YT_IGRACL_TEMPLATE_PPPOE_FLAG,
    YT_IGRACL_TEMPLATE_TCP_FLAGS,
    YT_IGRACL_TEMPLATE_IP_PROTOCOL,

    YT_IGRACL_TEMPLATE_TOS=26,
    YT_IGRACL_TEMPLATE_IS_IGMP,
    YT_IGRACL_TEMPLATE_UDF_0,
    YT_IGRACL_TEMPLATE_UDF_1,
    YT_IGRACL_TEMPLATE_UDF_2,

    YT_IGRACL_TEMPLATE_UDF_3=31,
    YT_IGRACL_TEMPLATE_UDF_4,
    YT_IGRACL_TEMPLATE_UDF_5,
    YT_IGRACL_TEMPLATE_UDF_6,
    YT_IGRACL_TEMPLATE_UDF_7,

    YT_IGRACL_TEMPLATE_INNER_CVID=36,
    YT_IGRACL_TEMPLATE_INNER_SVID,
    YT_IGRACL_TEMPLATE_INNER_SPRI,
    YT_IGRACL_TEMPLATE_INNER_CPRI,
    YT_IGRACL_TEMPLATE_INNER_SDEI,

    YT_IGRACL_TEMPLATE_INNER_CDEI=41,
    YT_IGRACL_TEMPLATE_ETHER_TYPE,
    YT_IGRACL_TEMPLATE_SRC_PORTMASK,

    YT_IGRACL_TEMPLATE_MAX,
}yt_igrAcl_key_type_t;

typedef enum yt_l3_type_e
{
    L3_NONE, /*except all below*/
    L3_IPV4,
    L3_IPV6,
    L3_IPV4ARP,
    L3_LLDP,
    L3_EAPOL,
    L3_ERP,
    L3_SLOW_PROTOCOL,
}yt_l3_type_t;

typedef enum yt_l4_type_e
{
    L4_NONE,/*except all below*/
    L4_TCP,
    L4_UDP,
    L4_UDP_LITE,
    L4_ICMP,
    L4_IGMP,
    L4_MLD,
    L4_ND,
}yt_l4_type_t;

typedef enum yt_acl_udf_type_e
{
    ACL_UDF_TYPE_RAW,
    ACL_UDF_TYPE_L3,
    ACL_UDF_TYPE_L4,
}yt_acl_udf_type_t;

typedef enum yt_acl_fwd_type_e
{
    ACL_FWD_TYPE_FWD,
    ACL_FWD_TYPE_COPY,
    ACL_FWD_TYPE_REDIRECT,
    ACL_FWD_TYPE_TRAP,
    ACL_FWD_TYPE_DROP,
}yt_acl_fwd_type_t;

typedef struct yt_acl_key_mac_s
{
    yt_mac_addr_t   mac_data;
    yt_mac_addr_t   mac_mask;
}yt_acl_key_mac_t;/*for s_mac, d_mac*/

typedef struct yt_acl_key_etherType_s
{
    uint16_t    type_data_min;
    uint16_t    type_mask_max;
    yt_bool_t   range_en;
}yt_acl_key_etherType_t;

typedef struct yt_acl_key_l2Type_s
{
    yt_l2_type_t    type_data;
    uint8_t			type_mask;
}yt_acl_key_l2Type_t;

typedef struct yt_acl_key_l3Type_s
{
    yt_l3_type_t    type_data;
    uint8_t			type_mask;
}yt_acl_key_l3Type_t;

typedef struct yt_acl_key_l4Type_s
{
    yt_l4_type_t    type_data;
    uint8_t			type_mask;
}yt_acl_key_l4Type_t;

typedef struct yt_acl_key_dei_s
{
    yt_bool_t    dei_data;
    yt_bool_t    dei_mask;
}yt_acl_key_dei_t;

typedef struct yt_acl_key_pri_s
{
    yt_pri_t    pri_data;
    uint8_t     pri_mask;
}yt_acl_key_pri_t;

typedef struct yt_acl_key_vlanFmt_s
{
    yt_vlan_format_t    tagfmt_data;
    uint8_t				tagfmt_mask;
}yt_acl_key_vlanFmt_t;

typedef struct yt_acl_key_vid_s
{
    uint16_t    vid_data_min;/*as min value if range_en*/
    uint16_t    vid_mask_max;/*as max value if range_en*/
    yt_bool_t	range_en;
}yt_acl_key_vid_t;

typedef struct yt_acl_key_ipv4_s
{
    uint32_t    ip_data_min;/*as min value if range_en*/
    uint32_t    ip_mask_max;/*as max value if range_en*/
    yt_bool_t range_en;
}yt_acl_key_ipv4_t;

typedef struct yt_acl_key_l4Port_s
{
    uint16_t    port_data_min;
    uint16_t    port_mask_max;
    yt_bool_t range_en;
}yt_acl_key_l4Port_t;

typedef struct yt_acl_key_isPppoe_s
{
    yt_bool_t    is_pppoe_data;
    yt_bool_t    is_pppoe_mask;
}yt_acl_key_isPppoe_t;

typedef struct yt_acl_key_tcpFlag_s
{
    uint8_t    flag_data;
    uint8_t    flag_mask;
}yt_acl_key_tcpFlag_t;

typedef struct yt_acl_key_isIgmp_s
{
    yt_bool_t    is_igmp_data;
    yt_bool_t    is_igmp_mask;
}yt_acl_key_isIgmp_t;

typedef struct yt_acl_key_ipFrag_s
{
    yt_bool_t    is_frag_data;
    yt_bool_t    is_frag_mask;
}yt_acl_key_ipFrag_t;

typedef struct yt_acl_key_1stIpFrag_s
{
    yt_bool_t    is_1stFrag_data;
    yt_bool_t    is_1stFrag_mask;
}yt_acl_key_1stIpFrag_t;

typedef struct yt_acl_key_ipOption_s
{
    yt_bool_t    is_option_data;
    yt_bool_t    is_option_mask;
}yt_acl_key_ipOption_t;

typedef struct yt_acl_key_ipProtocol_s
{
    uint8_t    protocol_data;
    uint8_t    protocol_mask;
}yt_acl_key_ipProtocol_t;

typedef struct yt_acl_key_ipTOS_s
{
    uint8_t    tos_data;
    uint8_t    tos_mask;
}yt_acl_key_ipTOS_t;

typedef struct yt_acl_key_udf_s
{
    uint8_t    udf_data[ACL_UDF_DATA_LEN_MAX];
    uint8_t    udf_mask[ACL_UDF_DATA_LEN_MAX];
}yt_acl_key_udf_t;

typedef struct yt_acl_key_ipv6_s
{
    uint8_t    ipv6_data_min[IP6_ADDR_LEN];
    uint8_t    ipv6_mask_max[IP6_ADDR_LEN];
    yt_bool_t range_en;
}yt_acl_key_ipv6_t;

typedef yt_port_mask_t yt_acl_key_srcPortmask_t;
typedef void yt_comm_key_t;

typedef void yt_comm_act_t;

typedef enum yt_acl_vlan_assign_mode_e
{
	ACL_VLANASSIGN_MODE_INVAILD,
	ACL_VLANASSIGN_MODE_UNTAG,
	ACL_VLANASSIGN_MODE_TAG,
	ACL_VLANASSIGN_MODE_KEEP,
}yt_acl_vlan_assign_mode_t;

typedef enum yt_acl_action_type_e
{
	ACL_ACT_TYPE_FWD,
	ACL_ACT_TYPE_INTPRI_MAP,
	ACL_ACT_TYPE_VID_REPLACE,
	ACL_ACT_TYPE_PRI_REPLACE,
	ACL_ACT_TYPE_VLAN_ASSIGN,
	ACL_ACT_TYPE_DSCP_REPLACE,
	ACL_ACT_TYPE_METER_ASSIGN,
	ACL_ACT_TYPE_FLOWSTAT,
	ACL_ACT_TYPE_MIRROR_ENABLE,
	ACL_ACT_TYPE_MAX
}yt_acl_action_type_t;

typedef struct yt_acl_action_fwd_s
{
    yt_bool_t   fwd_en;
    yt_acl_fwd_type_t   fwd_type;
    yt_port_mask_t  dst_portmask;/*used for FWD and REDIRECT,all zero for DROP type*/
}yt_acl_action_fwd_t;

typedef struct yt_acl_action_internalPriMap_s
{
    yt_bool_t    int_dp_en; /*enable internal drop priority map*/
    yt_bool_t    int_pri_en; /*enable internal priority map*/
    uint8    int_dp; /*internal drop priority,0~3*/
    uint8    int_pri; /*internal priority,0~7*/
}yt_acl_action_internalPriMap_t;

typedef struct yt_acl_action_vid_replace_s
{
	yt_bool_t	cvid_replace_en;
	yt_bool_t	svid_replace_en;
	yt_vlan_t	cvid;
	yt_vlan_t	svid;
}yt_acl_action_vid_replace_t;

typedef struct yt_acl_action_pri_replace_s
{
    yt_bool_t   cpri_replace_en;
    yt_bool_t   cdei_replace_en;
    yt_pri_t    cpri;
    yt_bool_t   cdei;
    yt_bool_t   spri_replace_en;
    yt_bool_t   sdei_replace_en;
    yt_pri_t    spri;
    yt_bool_t   sdei;
}yt_acl_action_pri_replace_t;

typedef struct yt_acl_action_vlan_assign_s
{
	yt_acl_vlan_assign_mode_t	ctag_assign_mode;
	yt_acl_vlan_assign_mode_t	stag_assign_mode;
}yt_acl_action_vlan_assign_t;

typedef struct yt_acl_action_dscp_replace_s
{
    yt_bool_t    dscp_replace_en;
    yt_dscp_t    dscp;
}yt_acl_action_dscp_replace_t;

typedef struct yt_acl_action_meter_assign_s
{
    yt_bool_t   meter_en;
    yt_meterid_t    meter_id;
}yt_acl_action_meter_assign_t;

typedef struct yt_acl_action_flowStats_s
{
    yt_bool_t	flow_stats_en;
    uint8_t		flow_stats_id; /*flow stat table id*/
}yt_acl_action_flowStats_t;

typedef struct yt_acl_action_mirrorEn_s
{
    yt_bool_t	mirror_en;
}yt_acl_action_mirrorEn_t;

/**
 * @internal      yt_acl_init
 * @endinternal
 *
 * @brief         acl module init,enable unmatch permit on port by default
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_init(yt_unit_t unit);


/**
 * @internal      yt_acl_port_en_set
 * @endinternal
 *
 * @brief         enable or disable acl function on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_acl_port_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_acl_port_en_get
 * @endinternal
 *
 * @brief         get acl enable state on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_acl_port_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_acl_unmatch_permit_en_set
 * @endinternal
 *
 * @brief         enable or disable forward permit when unmatch acl rule on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_acl_unmatch_permit_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_acl_unmatch_permit_en_get
 * @endinternal
 *
 * @brief         get enable state of forward permit when unmatch acl rule on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_acl_unmatch_permit_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_acl_udf_rule_set
 * @endinternal
 *
 * @brief         set user define rule to specific acl udf table.
 * @brief         every table will take 2 bytes data,X*2 and X*2+1 udf table used for YT_IGRACL_TEMPLATE_UDF_X.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     index               -acl udf table index,range (0~15).
 * @param[in]     type                -acl user define data type
 * @param[in]     offset              -offset start from specific type,range (0~127)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_udf_rule_set (yt_unit_t unit, uint8_t  index, yt_acl_udf_type_t type, uint8_t offset);


/**
 * @internal      yt_acl_rule_init
 * @endinternal
 *
 * @brief         start to init rule,must init before add rule key
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_rule_init(yt_unit_t unit);


/**
 * @internal      yt_acl_rule_reset
 * @endinternal
 *
 * @brief         clear acl rule key and action,used to recreate rule before rule active
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_rule_reset(yt_unit_t unit);

/**
 * @internal      yt_acl_rule_key_add
 * @endinternal
 *
 * @brief         add key to list,prepare to create rule.max 16 keys
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -acl key type,refer to yt_igrAcl_key_type_t define
 * @param[in]     pKey_data           -point to different key data,must according to key type
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_NOT_INIT        -did not init rule yet
 * @retval        CMM_ERR_NOT_SUPPORT        -unknown key type
 * @retval        CMM_ERR_ENTRY_FULL        -reach to max num of keys
 * @retval        CMM_ERR_SAMEENTRY_EXIST        -already has same key
 */
extern yt_ret_t yt_acl_rule_key_add(yt_unit_t unit, yt_igrAcl_key_type_t type, yt_comm_key_t *pKey_data);


/**
 * @internal      yt_acl_rule_action_add
 * @endinternal
 *
 * @brief         add acl action before acl rule active
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                  -acl action type,refer to yt_acl_action_type_t define
 * @param[in]     pAction             -point to acl action settings,must according to action type
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_NOT_INIT        -did not init rule yet
 * @retval        CMM_ERR_NOT_SUPPORT        -unknown action type
 */
extern yt_ret_t yt_acl_rule_action_add(yt_unit_t unit, yt_acl_action_type_t type, yt_comm_act_t *pAction);


/**
 * @internal      yt_acl_rule_create
 * @endinternal
 *
 * @brief         create acl rule after add rule key data
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     rulePri             -acl rule priority,range 0~511
 * @param[in]     ruleReverse         -if reverse rule logic
 * @param[out]    pId                 -acl rule id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_TOO_LESS_INFO        -on too less info in the rule
 */
extern yt_ret_t yt_acl_rule_create(yt_unit_t unit, uint16_t rulePri, yt_bool_t ruleReverse, uint32_t *pId);


/**
 * @internal      yt_acl_rule_active
 * @endinternal
 *
 * @brief         active acl rule and related action
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     id                  -acl rule id created by yt_acl_rule_create
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_rule_active(yt_unit_t unit, uint32_t id);


/**
 * @internal      yt_acl_rule_del
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     id                  -acl rule id created by yt_acl_rule_create
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_acl_rule_del(yt_unit_t unit, uint32_t id);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
