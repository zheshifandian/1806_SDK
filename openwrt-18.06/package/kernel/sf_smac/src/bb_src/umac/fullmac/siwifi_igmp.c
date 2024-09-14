#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/igmp.h>
#include "siwifi_defs.h"
#include "siwifi_mem.h"

#define SIZE_OF_IP_OPTION 4

int siwifi_print_mcg_info(struct siwifi_hw *siwifi_hw)
{
    struct multicast_group * tmp_mul_group = NULL;
    int i;
    printk("siwifi_print_mcg_info\n");

    tmp_mul_group = siwifi_hw->mul_group;
    while(tmp_mul_group != NULL) {
        printk("group:%d.%d.%d.%d\n", NIPQUAD(tmp_mul_group->multicast_addr));
        for(i = 0; i < MAX_MEMBER_IN_MULTICAST; i++) {
            if(tmp_mul_group->member[i].member_addr != 0){
                printk("  member:%d.%d.%d.%d\n", NIPQUAD(tmp_mul_group->member[i].member_addr));
                printk("  macaddr:%x:%x:%x:%x:%x:%x\n",NMACQUAD(tmp_mul_group->member[i].member_source[0]));
            }
        }
        tmp_mul_group = tmp_mul_group->next_mcg;
    }

    return 0;
}

int siwifi_get_mc_group_number(struct siwifi_hw *siwifi_hw)
{
    struct multicast_group * tmp_mul_group = NULL;
    int number = 0;

    tmp_mul_group = siwifi_hw->mul_group;
    while(tmp_mul_group != NULL) {
        number++;
        tmp_mul_group = tmp_mul_group->next_mcg;
    }

    return number;
}

int siwifi_handle_mcgroup(__be32 src_addr, unsigned char * eth_src, struct multicast_group * mc_group, enum handle_sa_mcg hsm)
{
    int i;
    int search = -1;
    int first_null_addr = -1;
    int number_of_addr = 0;

    for(i = 0; i < MAX_MEMBER_IN_MULTICAST; i++) {
        if(mc_group->member[i].member_addr == src_addr)
            search = i;

        if(mc_group->member[i].member_addr == 0 && first_null_addr < 0)
            first_null_addr = i;

        if(mc_group->member[i].member_addr != 0)
            number_of_addr++;
    }

    switch(hsm){
        case HANDLE_MCG_SEARCH:
            if(search >= 0)
                return 1;
            else
                return 0;

        case HANDLE_MCG_ADD:
            if(search < 0 && first_null_addr >= 0) {
                mc_group->member[first_null_addr].member_addr = src_addr;
                memcpy(mc_group->member[first_null_addr].member_source, eth_src, ETH_ALEN);
                return 0;
            }
            break;

        case HANDLE_MCG_DELETE:
            if(search >= 0) {
                mc_group->member[search].member_addr = 0;
                memset(mc_group->member[search].member_source,0,ETH_ALEN);
                return 0;
            }
            break;

        case HANDLE_MCG_GET_MEMBER_NUMBER:
            return number_of_addr;

        default:
            return -1;
    }

    return -1;
}

struct multicast_group * siwifi_search_muladdr(struct siwifi_hw *siwifi_hw, __be32 igmpv3_muladdr)
{
    struct multicast_group * search_mcg = siwifi_hw->mul_group;

    while(search_mcg != NULL) {
        //printk("search %d.%d.%d.%d in %d.%d.%d.%d\n",
        //        NIPQUAD(igmpv3_muladdr),NIPQUAD(search_mcg->multicast_addr));
        if(search_mcg->multicast_addr == igmpv3_muladdr)
            return search_mcg;
        else{
            search_mcg = search_mcg->next_mcg;
        }
    }

    return NULL;
}

int in_filter_muladdr(__be32 igmpv3_muladdr)
{
    if(igmpv3_muladdr == 0xfb0000e0) //224. 0 . 0 .251
        return 1;
    return 0;
}

int siwifi_mc_join_group(struct siwifi_hw *siwifi_hw, __be32 src_addr, unsigned char * eth_src, __be32 igmpv3_muladdr)
{
    struct multicast_group * tmp_mul_group = NULL;
    struct multicast_group * new_mul_group = NULL;
    int mc_group_number;

    if(in_filter_muladdr(igmpv3_muladdr))
        return -1;

    //printk("src %d.%d.%d.%d join mc_group %d.%d.%d.%d \n",
            //NIPQUAD(src_addr), NIPQUAD(igmpv3_muladdr));
    tmp_mul_group = siwifi_search_muladdr(siwifi_hw, igmpv3_muladdr);
    if(tmp_mul_group == NULL) {
        mc_group_number = siwifi_get_mc_group_number(siwifi_hw);
        if(mc_group_number >= MAX_MC_GROUP_NUMBER) {
            printk("The number of multicast groups reaches the upper limit\n");
            return -1;
        }

        new_mul_group = (struct multicast_group *)siwifi_kzalloc(sizeof(struct multicast_group), GFP_ATOMIC);
        //printk("malloc %p new-src %d.%d.%d.%d join mc_group %d.%d.%d.%d \n",
        //    new_mul_group, NIPQUAD(src_addr), NIPQUAD(igmpv3_muladdr));
        new_mul_group->multicast_addr = igmpv3_muladdr;
        new_mul_group->member[0].member_addr = src_addr;
        memcpy(new_mul_group->member[0].member_source, eth_src, ETH_ALEN);
        new_mul_group->next_mcg = NULL;

        tmp_mul_group = siwifi_hw->mul_group;
        if(tmp_mul_group == NULL) {
            siwifi_hw->mul_group = new_mul_group;
        } else {
            while((tmp_mul_group->next_mcg) != NULL) {
                tmp_mul_group = tmp_mul_group->next_mcg;
            }
            tmp_mul_group->next_mcg = new_mul_group;
        }
    } else {
        siwifi_handle_mcgroup(src_addr, eth_src, tmp_mul_group, HANDLE_MCG_ADD);
    }

    return 0;
}

int siwifi_mcg_free(struct multicast_group * free_mul_group, struct siwifi_hw * siwifi_hw, enum free_mcg fmcg)
{
    struct multicast_group * previous_mul_group = NULL;
    struct multicast_group * tmp_free_mul_group = NULL;

    previous_mul_group = siwifi_hw->mul_group;
    if(previous_mul_group == NULL)
        return -1;

    switch(fmcg){
        case FREE_MCG_SINGLE:
            if(siwifi_hw->mul_group == free_mul_group){
                siwifi_hw->mul_group = siwifi_hw->mul_group->next_mcg;
                siwifi_kfree(free_mul_group);
                break;
            }

            while(previous_mul_group->next_mcg != free_mul_group && previous_mul_group->next_mcg != NULL){
                previous_mul_group = previous_mul_group->next_mcg;
            }
            previous_mul_group->next_mcg = free_mul_group->next_mcg;
            siwifi_kfree(free_mul_group);
            break;
        case FREE_MCG_ALL:
            while(previous_mul_group != NULL){
                tmp_free_mul_group = previous_mul_group;
                previous_mul_group = previous_mul_group->next_mcg;
                siwifi_kfree(tmp_free_mul_group);
            }
            break;
        default:
            break;
    }

    return 0;
}

int siwifi_mc_leave_group(struct siwifi_hw *siwifi_hw, __be32 src_addr, __be32 igmpv3_muladdr)
{
    struct multicast_group * tmp_mul_group = NULL;
    //printk("src %d.%d.%d.%d leave mc_group %d.%d.%d.%d \n",
            //NIPQUAD(src_addr), NIPQUAD(igmpv3_muladdr));
    tmp_mul_group = siwifi_search_muladdr(siwifi_hw, igmpv3_muladdr);
    if(tmp_mul_group != NULL) {
        siwifi_handle_mcgroup(src_addr, NULL, tmp_mul_group, HANDLE_MCG_DELETE);
        if(siwifi_handle_mcgroup(0, NULL,tmp_mul_group, HANDLE_MCG_GET_MEMBER_NUMBER) == 0){
            siwifi_mcg_free(tmp_mul_group, siwifi_hw, FREE_MCG_SINGLE);
        }
    }

    return 0;
}

int siwifi_check_igmp(struct siwifi_hw *siwifi_hw, struct sk_buff *skb)
{
    struct ethhdr* tmp_ethhdr = NULL;
    struct iphdr* tmp_iphdr = NULL;
    unsigned char  ipv4_proto = 0;
    struct igmphdr * tmp_igmphdr = NULL;
    struct igmpv3_report* igmpv3_hdr = NULL;
    __be32 src_addr;
    __be32 igmpv3_muladdr;
    __u8 igmp_type;
    //group record types
    __u8 igmp_grec_type;
    unsigned char eth_source[ETH_ALEN];
    struct igmpv3_grec *igmpv3grec = NULL;
    int rec_id = 0;
    int src_num = 0;

    tmp_ethhdr = (struct ethhdr *) (skb->head + skb->mac_header);
    if (!is_multicast_ether_addr(tmp_ethhdr->h_dest))
        return -1;
    tmp_iphdr = (struct iphdr *) ((unsigned char*)tmp_ethhdr  + sizeof(struct ethhdr));
    ipv4_proto = tmp_iphdr->protocol;
    if (ipv4_proto != IPPROTO_IGMP)
        return -1;
    src_addr = tmp_iphdr->saddr;

    //todo vlantag
    tmp_igmphdr = (struct igmphdr *) ((unsigned char*)tmp_iphdr + sizeof(struct iphdr) + SIZE_OF_IP_OPTION);
    igmp_type = tmp_igmphdr->type;
    switch(igmp_type) {
        //igmp v1/v2 report type
        case IGMP_HOST_MEMBERSHIP_REPORT:
        case IGMPV2_HOST_MEMBERSHIP_REPORT:
            memcpy(eth_source, tmp_ethhdr->h_source, ETH_ALEN);
            siwifi_mc_join_group(siwifi_hw, src_addr, eth_source, tmp_igmphdr->group);
            break;
        case IGMP_HOST_LEAVE_MESSAGE:
            siwifi_mc_leave_group(siwifi_hw, src_addr, tmp_igmphdr->group);
            break;
        case IGMPV3_HOST_MEMBERSHIP_REPORT:
            igmpv3_hdr = (struct igmpv3_report *) ((unsigned char*)tmp_iphdr + sizeof(struct iphdr) + SIZE_OF_IP_OPTION);
            rec_id = ntohs(igmpv3_hdr->ngrec);
            igmpv3grec = &igmpv3_hdr->grec[0];
            while(rec_id > 0){
                src_num = ntohs(igmpv3grec->grec_nsrcs);
                igmpv3_muladdr = igmpv3grec->grec_mca;
                igmp_grec_type = igmpv3grec->grec_type;
                //printk("---rec_id %d igmpv3_mul_ip %d.%d.%d.%d---\n", rec_id, NIPQUAD(igmpv3_muladdr));

                switch(igmp_grec_type){
                    case IGMPV3_MODE_IS_INCLUDE:
                    case IGMPV3_CHANGE_TO_INCLUDE:
                        if(src_num == 0) {
                            siwifi_mc_leave_group(siwifi_hw,src_addr,igmpv3_muladdr);
                        } else {
                            memcpy(eth_source, tmp_ethhdr->h_source, ETH_ALEN);
                            siwifi_mc_join_group(siwifi_hw,src_addr,eth_source,igmpv3_muladdr);
                        }
                        break;
                    case IGMPV3_MODE_IS_EXCLUDE:
                    case IGMPV3_CHANGE_TO_EXCLUDE:
                    case IGMPV3_ALLOW_NEW_SOURCES:
                        memcpy(eth_source, tmp_ethhdr->h_source, ETH_ALEN);
                        siwifi_mc_join_group(siwifi_hw,src_addr,eth_source,igmpv3_muladdr);
                        break;
                    case IGMPV3_BLOCK_OLD_SOURCES:
                        siwifi_mc_leave_group(siwifi_hw,src_addr,igmpv3_muladdr);
                        break;
                    default:
                        break;
                }
                rec_id--;
                igmpv3grec = (struct igmpv3_grec *)((char*)igmpv3grec + sizeof(struct igmpv3_grec) + (igmpv3grec->grec_auxwords + src_num) * sizeof(__u32));
            }
            break;
        case IGMP_HOST_MEMBERSHIP_QUERY:
        default:
            break;
        }

    return 0;
}

unsigned char * siwifi_multicast_to_unicast(struct siwifi_hw *siwifi_hw, struct sk_buff *skb,
                                            struct net_device *dev)
{
    struct sk_buff *mcg_copy_skb = NULL;
    struct ethhdr* tmp_ethhdr = NULL;
    struct iphdr* tmp_iphdr = NULL;
    __be32 src_addr;
    __be32 dst_addr;
    struct multicast_group * tmp_mcg = NULL;
    int i;
    int unicast_addr_number;

    tmp_ethhdr = (struct ethhdr *)skb->data;
    tmp_iphdr = (struct iphdr *) ((unsigned char*)tmp_ethhdr  + sizeof(struct ethhdr));

    src_addr = tmp_iphdr->saddr;
    dst_addr = tmp_iphdr->daddr;
    tmp_mcg = siwifi_search_muladdr(siwifi_hw, dst_addr);
    if(tmp_mcg){
        unicast_addr_number = siwifi_handle_mcgroup(0, NULL, tmp_mcg, HANDLE_MCG_GET_MEMBER_NUMBER);
        for(i = 0; i < MAX_MEMBER_IN_MULTICAST; i++){
            if(tmp_mcg->member[i].member_addr == src_addr)
                continue;
            if(tmp_mcg->member[i].member_addr != 0) {
                if(unicast_addr_number > 1){
                    mcg_copy_skb = skb_copy(skb, GFP_ATOMIC);
                    _siwifi_start_xmit(mcg_copy_skb, dev, tmp_mcg->member[i].member_source);
                    unicast_addr_number--;
                } else {
                    return tmp_mcg->member[i].member_source;
                }
            }
        }
    }
    return NULL;
}

int siwifi_del_mcgmember(struct siwifi_hw *siwifi_hw, u8 *del_macaddr)
{
    struct multicast_group * tmp_mul_group = NULL;
    unsigned char del_mac[ETH_ALEN];
    int i;

    if(del_macaddr)
        memcpy(&del_mac, del_macaddr, ETH_ALEN);
    else
        return -1;

    tmp_mul_group = siwifi_hw->mul_group;
    while(tmp_mul_group != NULL) {
        //printk("group:%d.%d.%d.%d\n", NIPQUAD(tmp_mul_group->multicast_addr));
        for(i = 0; i < MAX_MEMBER_IN_MULTICAST; i++) {
            if(memcmp(del_mac, tmp_mul_group->member[i].member_source, ETH_ALEN) == 0){
                siwifi_mc_leave_group(siwifi_hw, tmp_mul_group->member[i].member_addr, tmp_mul_group->multicast_addr);
            }
        }
        tmp_mul_group = tmp_mul_group->next_mcg;
    }

    return 0;
}
