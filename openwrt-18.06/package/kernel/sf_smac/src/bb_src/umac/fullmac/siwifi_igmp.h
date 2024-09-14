/**
 ******************************************************************************
 *
 * @file siwifi_igmp.h
 *
 * @content IGMP (Internet Group Management Protocol) in Multicast
 *
 * Copyright (C) Siflower
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_IGMP_H_
#define _SIWIFI_IGMP_H_

//the max number of member in one multicast group
#define MAX_MEMBER_IN_MULTICAST 16
//the max number of multicast group
#define MAX_MC_GROUP_NUMBER 255

#define NIPQUAD(addr)\
    ((unsigned char *)&addr)[0],\
    ((unsigned char *)&addr)[1],\
    ((unsigned char *)&addr)[2],\
    ((unsigned char *)&addr)[3]

#define NMACQUAD(mac)\
    ((unsigned char *)&mac)[0],\
    ((unsigned char *)&mac)[1],\
    ((unsigned char *)&mac)[2],\
    ((unsigned char *)&mac)[3],\
    ((unsigned char *)&mac)[4],\
    ((unsigned char *)&mac)[5]

enum handle_sa_mcg
{
    HANDLE_MCG_SEARCH = 0,
    HANDLE_MCG_ADD,
    HANDLE_MCG_DELETE,
    HANDLE_MCG_GET_MEMBER_NUMBER
};

enum free_mcg
{
    FREE_MCG_SINGLE = 0,
    FREE_MCG_ALL
};

struct mcg_member {
    __be32 member_addr;
    unsigned char member_source[ETH_ALEN];
};

struct multicast_group {
    __be32 multicast_addr;
    struct mcg_member member[MAX_MEMBER_IN_MULTICAST];
    struct multicast_group * next_mcg;
};

int siwifi_print_mcg_info(struct siwifi_hw *siwifi_hw);
int siwifi_get_mc_group_number(struct siwifi_hw *siwifi_hw);
struct multicast_group * siwifi_search_muladdr(struct siwifi_hw *siwifi_hw,
                                                __be32 igmpv3_muladdr);
int siwifi_mcg_free(struct multicast_group * free_mul_group, struct siwifi_hw * siwifi_hw, enum free_mcg fmcg);
int siwifi_check_igmp(struct siwifi_hw *siwifi_hw, struct sk_buff *skb);
unsigned char * siwifi_multicast_to_unicast(struct siwifi_hw *siwifi_hw, struct sk_buff *skb,
                                    struct net_device *dev);
netdev_tx_t _siwifi_start_xmit(struct sk_buff *skb, struct net_device *dev,
                                unsigned char *_unicast_addr);
int siwifi_del_mcgmember(struct siwifi_hw *siwifi_hw, u8 *del_macaddr);
#endif /* _SIWIFI_LED_H_ */
