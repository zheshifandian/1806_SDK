/**
 ******************************************************************************
 *
 * @file siwifi_bfmer.c
 *
 * @brief VHT Beamformer function definitions
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

/**
 * INCLUDE FILES
 ******************************************************************************
 */

#include <linux/slab.h>
#include "siwifi_bfmer.h"
#include "siwifi_mem.h"

/**
 * FUNCTION DEFINITIONS
 ******************************************************************************
 */

int siwifi_bfmer_report_add(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta,
                          unsigned int length)
{
    gfp_t flags;
    struct siwifi_bfmer_report *bfm_report ;

    if (in_softirq())
        flags = GFP_ATOMIC;
    else
        flags = GFP_KERNEL;

    /* Allocate a structure that will contain the beamforming report */
    bfm_report = siwifi_kmalloc(sizeof(*bfm_report) + length, flags);


    /* Check report allocation */
    if (!bfm_report) {
        /* Do not use beamforming */
        return -1;
    }

    /* Store report length */
    bfm_report->length = length;

    /*
     * Need to provide a Virtual Address to the MAC so that it can
     * upload the received Beamforming Report in driver memory
     */
    bfm_report->dma_addr = dma_map_single(siwifi_hw->dev, &bfm_report->report[0],
                                          length, DMA_FROM_DEVICE);

    /* Check DMA mapping result */
    if (dma_mapping_error(siwifi_hw->dev, bfm_report->dma_addr)) {
        /* Free allocated report */
        siwifi_kfree(bfm_report);
        /* And leave */
        return -1;
    }

    /* Store report structure */
    siwifi_sta->bfm_report = bfm_report;

    return 0;
}

void siwifi_bfmer_report_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *siwifi_sta)
{
    /* Verify if a report has been allocated */
    if (siwifi_sta->bfm_report) {
        struct siwifi_bfmer_report *bfm_report = siwifi_sta->bfm_report;

        /* Unmap DMA region */
        dma_unmap_single(siwifi_hw->dev, bfm_report->dma_addr,
                         bfm_report->length, DMA_BIDIRECTIONAL);

        /* Free allocated report structure and clean the pointer */
        siwifi_kfree(bfm_report);
        siwifi_sta->bfm_report = NULL;
    }
}

u8 siwifi_bfmer_get_rx_nss(const struct ieee80211_vht_cap *vht_capa)
{
    int i;
    u8 rx_nss = 0;
    u16 rx_mcs_map = le16_to_cpu(vht_capa->supp_mcs.rx_mcs_map);

    for (i = 7; i >= 0; i--) {
        u8 mcs = (rx_mcs_map >> (2 * i)) & 3;

        if (mcs != IEEE80211_VHT_MCS_NOT_SUPPORTED) {
            rx_nss = i + 1;
            break;
        }
    }

    return rx_nss;
}
