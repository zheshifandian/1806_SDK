/**
 ****************************************************************************************
 *
 * @file siwifi_mesh.c
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

/**
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "siwifi_mesh.h"

/**
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

struct siwifi_mesh_proxy *siwifi_get_mesh_proxy_info(struct siwifi_vif *p_siwifi_vif, u8 *p_sta_addr, bool local)
{
    struct siwifi_mesh_proxy *p_mesh_proxy = NULL;
    struct siwifi_mesh_proxy *p_cur_proxy;

    /* Look for proxied devices with provided address */
    list_for_each_entry(p_cur_proxy, &p_siwifi_vif->ap.proxy_list, list) {
        if (p_cur_proxy->local != local) {
            continue;
        }

        if (!memcmp(&p_cur_proxy->ext_sta_addr, p_sta_addr, ETH_ALEN)) {
            p_mesh_proxy = p_cur_proxy;
            break;
        }
    }

    /* Return the found information */
    return p_mesh_proxy;
}
