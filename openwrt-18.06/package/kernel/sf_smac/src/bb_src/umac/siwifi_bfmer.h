/**
 ******************************************************************************
 *
 * @file siwifi_bfmer.h
 *
 * @brief VHT Beamformer function declarations
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_BFMER_H_
#define _SIWIFI_BFMER_H_

/**
 * INCLUDE FILES
 ******************************************************************************
 */

#include "siwifi_defs.h"

/**
 * DEFINES
 ******************************************************************************
 */

/// Maximal supported report length (in bytes)
#define SIWIFI_BFMER_REPORT_MAX_LEN     2048

/// Size of the allocated report space (twice the maximum report length)
#define SIWIFI_BFMER_REPORT_SPACE_SIZE  (SIWIFI_BFMER_REPORT_MAX_LEN * 2)

/**
 * TYPE DEFINITIONS
 ******************************************************************************
 */

/*
 * Structure used to store a beamforming report.
 */
struct siwifi_bfmer_report {
    dma_addr_t dma_addr;    /* Virtual address provided to MAC for
                               DMA transfer of the Beamforming Report */
    unsigned int length;    /* Report Length */
    u8 report[1];           /* Report to be used for VHT TX Beamforming */
};

/**
 * FUNCTION DECLARATIONS
 ******************************************************************************
 */

/**
 ******************************************************************************
 * @brief Allocate memory aiming to contains the Beamforming Report received
 * from a Beamformee capable capable.
 * The providing length shall be large enough to contain the VHT Compressed
 * Beaforming Report and the MU Exclusive part.
 * It also perform a DMA Mapping providing an address to be provided to the HW
 * responsible for the DMA transfer of the report.
 * If successful a struct siwifi_bfmer_report object is allocated, it's address
 * is stored in siwifi_sta->bfm_report.
 *
 * @param[in] siwifi_hw   PHY Information
 * @param[in] siwifi_sta  Peer STA Information
 * @param[in] length    Memory size to be allocated
 *
 * @return 0 if operation is successful, else -1.
 ******************************************************************************
 */
int siwifi_bfmer_report_add(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta,
                          unsigned int length);

/**
 ******************************************************************************
 * @brief Free a previously allocated memory intended to be used for
 * Beamforming Reports.
 *
 * @param[in] siwifi_hw   PHY Information
 * @param[in] siwifi_sta  Peer STA Information
 *
 ******************************************************************************
 */
void siwifi_bfmer_report_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta);

/**
 ******************************************************************************
 * @brief Parse a Rx VHT-MCS map in order to deduce the maximum number of
 * Spatial Streams supported by a beamformee.
 *
 * @param[in] vht_capa  Received VHT Capability field.
 *
 ******************************************************************************
 */
u8 siwifi_bfmer_get_rx_nss(const struct ieee80211_vht_cap *vht_capa);

#endif /* _SIWIFI_BFMER_H_ */
