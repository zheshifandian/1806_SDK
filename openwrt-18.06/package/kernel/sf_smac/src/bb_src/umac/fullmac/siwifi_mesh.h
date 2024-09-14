/**
 ****************************************************************************************
 *
 * @file siwifi_mesh.h
 *
 * @brief VHT Beamformer function declarations
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

#ifndef _SIWIFI_MESH_H_
#define _SIWIFI_MESH_H_

/**
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "siwifi_defs.h"

/**
 * DEFINES
 ****************************************************************************************
 */

/**
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief TODO [LT]
 ****************************************************************************************
 */
#ifdef CONFIG_SIWIFI_MESH
struct siwifi_mesh_proxy *siwifi_get_mesh_proxy_info(struct siwifi_vif *p_siwifi_vif, u8 *p_sta_addr, bool local);
#else
static inline struct siwifi_mesh_proxy *siwifi_get_mesh_proxy_info(struct siwifi_vif *p_siwifi_vif, u8 *p_sta_addr, bool local)
{return NULL;}
#endif


#endif /* _SIWIFI_MESH_H_ */
