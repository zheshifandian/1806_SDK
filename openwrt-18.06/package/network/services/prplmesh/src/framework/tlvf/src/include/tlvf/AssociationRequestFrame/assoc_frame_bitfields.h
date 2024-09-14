
#ifndef _ASSOCIATION_FRAME_STRUCTS_
#define _ASSOCIATION_FRAME_STRUCTS_

#include <asm/byteorder.h>
#include <cstddef>
#include <memory>

namespace assoc_frame {

constexpr uint8_t MAC_ADDR_LEN = 6;

typedef struct sStaHtCapabilityInfo {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint16_t ldcp_coding_capability : 1;
    uint16_t support_ch_width_set : 1;
    uint16_t sm_power_save : 2;
    uint16_t ht_greenfield : 1;
    uint16_t short_gi20mhz : 1;
    uint16_t short_gi40mhz : 1;
    uint16_t tx_stbc : 1;
    uint16_t rx_stbc : 2;
    uint16_t ht_delayed_block_ack : 1;
    uint16_t max_a_msdu_length : 1;
    uint16_t dsss_cck_mode40mhz : 1;
    uint16_t reserved : 1;
    uint16_t forty_mhz_intolerant : 1;
    uint16_t l_sig_txop_protection_support : 1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint16_t l_sig_txop_protection_support : 1;
    uint16_t forty_mhz_intolerant : 1;
    uint16_t reserved : 1;
    uint16_t dsss_cck_mode40mhz : 1;
    uint16_t max_a_msdu_length : 1;
    uint16_t ht_delayed_block_ack : 1;
    uint16_t rx_stbc : 2;
    uint16_t tx_stbc : 1;
    uint16_t short_gi40mhz : 1;
    uint16_t short_gi20mhz : 1;
    uint16_t ht_greenfield : 1;
    uint16_t sm_power_save : 2;
    uint16_t support_ch_width_set : 1;
    uint16_t ldcp_coding_capability : 1;
#else
#error "Bitfield macros are not defined"
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sStaHtCapabilityInfo;

typedef struct sStaVhtCapInfo {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint32_t max_mpdu_len : 2;
    uint32_t support_ch_width_set : 2;
    uint32_t rx_ldpc : 1;
    uint32_t short_gi80mhz_tvht_mode4c : 1;
    uint32_t short_gi160mhz80_80mhz : 1;
    uint32_t tx_stbc : 1;
    uint32_t rx_stbc : 3;
    uint32_t su_beamformer : 1;           // SU beamformer capable
    uint32_t su_beamformee : 1;           // SU beamformee capable
    uint32_t beamformee_sts : 3;          // Beamformee STS capability
    uint32_t sound_dimensions : 3;        // Number of sounding dimensions
    uint32_t mu_beamformer : 1;           // MU beamformer capable
    uint32_t mu_beamformee : 1;           // MU beamformee capable
    uint32_t txop_ps : 1;                 // TXOP PS
    uint32_t htc_vht : 1;                 // +HTC VHT capable
    uint32_t max_a_mpdu_len : 3;          // Maximum A-MPDU length exponent
    uint32_t vht_link_adaptation : 2;     // VHT Link adaptation capable
    uint32_t rx_antenna_pattern : 1;      // rx antenna pattern consistency
    uint32_t tx_antenna_pattern : 1;      // tx antenna pattern consistency
    uint32_t extended_nss_bw_support : 2; // Extended NSS BW Support
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint32_t extended_nss_bw_support : 2;
    uint32_t tx_antenna_pattern : 1;
    uint32_t rx_antenna_pattern : 1;
    uint32_t vht_link_adaptation : 2;
    uint32_t max_a_mpdu_len : 3;
    uint32_t htc_vht : 1;
    uint32_t txop_ps : 1;
    uint32_t mu_beamformee : 1;
    uint32_t mu_beamformer : 1;
    uint32_t sound_dimensions : 3;
    uint32_t beamformee_sts : 3;
    uint32_t su_beamformee : 1;
    uint32_t su_beamformer : 1;
    uint32_t rx_stbc : 3;
    uint32_t tx_stbc : 1;
    uint32_t short_gi160mhz80_80mhz : 1;
    uint32_t short_gi80mhz_tvht_mode4c : 1;
    uint32_t rx_ldpc : 1;
    uint32_t support_ch_width_set : 2;
    uint32_t max_mpdu_len : 2;
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sStaVhtCapInfo;

typedef struct sSupportedVhtMcsSet {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint64_t rx_mcs_map : 16;
    uint64_t rx_highest_supp_long_gi_data_rate : 13;
    uint64_t max_nsts_total : 3;
    uint64_t tx_mcs_map : 16;
    uint64_t tx_highest_supp_long_gi_data_rate : 13;
    uint64_t extended_nss_bw_capable : 1;
    uint64_t reserved : 2;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint64_t reserved : 2;
    uint64_t extended_nss_bw_capable : 1;
    uint64_t tx_highest_supp_long_gi_data_rate : 13;
    uint64_t tx_mcs_map : 16;
    uint64_t max_nsts_total : 3;
    uint64_t rx_highest_supp_long_gi_data_rate : 13;
    uint64_t rx_mcs_map : 16;
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sSupportedVhtMcsSet;

////////////////////////////////////////////////
// Capability Information field for non DMG STA
///////////////////////////////////////////////

/**
 * @brief This struct used in (Re)Association Request frame
 * to represent Capability Information field transmitted by a NON DMG STA.
 */
typedef struct sCapabilityInfoNonDmgSta {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint16_t ess : 1;
    uint16_t ibss : 1;
    uint16_t cf_pollable : 1;
    uint16_t cf_poll_request : 1;
    uint16_t privacy : 1;
    uint16_t short_preamble : 1;
    uint16_t reserved2 : 2;
    uint16_t spectrum_management : 1;
    uint16_t qos : 1;
    uint16_t short_slot_time : 1;
    uint16_t apsd : 1;
    uint16_t radio_measurement : 1;
    uint16_t reserved1 : 1;
    uint16_t delayed_block_ack : 1;
    uint16_t immediate_block_ack : 1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint16_t immediate_block_ack : 1;
    uint16_t delayed_block_ack : 1;
    uint16_t reserved1 : 1;
    uint16_t radio_measurement : 1;
    uint16_t apsd : 1;
    uint16_t short_slot_time : 1;
    uint16_t qos : 1;
    uint16_t spectrum_management : 1;
    uint16_t reserved2 : 2;
    uint16_t short_preamble : 1;
    uint16_t privacy : 1;
    uint16_t cf_poll_request : 1;
    uint16_t cf_pollable : 1;
    uint16_t ibss : 1;
    uint16_t ess : 1;
#endif
    // sCapabilityInfoNonDmgSta(){};
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sCapabilityInfoNonDmgSta;

typedef struct sRmEnabledCaps1 {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint32_t link_measurement : 1;
    uint32_t neighbor_report : 1;
    uint32_t parallel_measure : 1;
    uint32_t repeated_measure : 1;
    uint32_t beacon_passive_measure : 1;
    uint32_t beacon_active_measure : 1;
    uint32_t beacon_table_measure : 1;
    uint32_t
        beacon_measure_report_cond : 1; // Beacon measurement reporting conditions capability enabled
    uint32_t frame_measurement : 1;     // Frame measurement capability enabled
    uint32_t ch_load_measure : 1;       // Channel load measurement capability enabled
    uint32_t noise_histogram_measure : 1; // Noise histogram measurement capability enabled
    uint32_t stat_measure : 1;            // Statistics measurement capability enabled
    uint32_t lci_measure : 1;
    uint32_t lci_azimuth : 1;
    uint32_t tx_stream : 1;    // Transmit Stream/category measurement capability enabled
    uint32_t trigger_tx : 1;   // Triggered tx stream/category measurement capability enabled
    uint32_t ap_ch_report : 1; // AP channel report capability enabled
    uint32_t rm_mib : 1;
    uint32_t op_ch_max_measure_dur : 3;    // Operating channel Max measurement Duration
    uint32_t nonop_ch_max_measure_dur : 3; // Nonoperating channel Max measurement Duration
    uint32_t measure_pilot_cap : 3;        // Measurement pilot Capability
    uint32_t measure_pilot_trans_info : 1; // Measurement pilot Transmission information cap enabled
    uint32_t neighbor_report_tsf_offset : 1;
    uint32_t rcpi_measure : 1;
    uint32_t rsni_measure : 1;
    uint32_t bss_average_ac_delay : 1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint32_t bss_average_ac_delay : 1;
    uint32_t rsni_measure : 1;
    uint32_t rcpi_measure : 1;
    uint32_t neighbor_report_tsf_offset : 1;
    uint32_t measure_pilot_trans_info : 1;
    uint32_t measure_pilot_cap : 3;
    uint32_t nonop_ch_max_measure_dur : 3;
    uint32_t op_ch_max_measure_dur : 3;
    uint32_t rm_mib : 1;
    uint32_t ap_ch_report : 1;
    uint32_t trigger_tx : 1;
    uint32_t tx_stream : 1;
    uint32_t lci_azimuth : 1;
    uint32_t lci_measure : 1;
    uint32_t stat_measure : 1;
    uint32_t noise_histogram_measure : 1;
    uint32_t ch_load_measure : 1;
    uint32_t frame_measurement : 1;
    uint32_t beacon_measure_report_cond : 1;
    uint32_t beacon_table_measure : 1;
    uint32_t beacon_active_measure : 1;
    uint32_t beacon_passive_measure : 1;
    uint32_t repeated_measure : 1;
    uint32_t parallel_measure : 1;
    uint32_t neighbor_report : 1;
    uint32_t link_measurement : 1;
#endif
    // sRmEnabledCaps1(){};
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sRmEnabledCaps1;

typedef struct sRmEnabledCaps2 {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint8_t bss_available_adm_capacity : 1;
    uint8_t antenna : 1;
    uint8_t ftm_range_report : 1;
    uint8_t civic_location_measure : 1;
    uint8_t reserved : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint8_t reserved : 4;
    uint8_t civic_location_measure : 1;
    uint8_t ftm_range_report : 1;
    uint8_t antenna : 1;
    uint8_t bss_available_adm_capacity : 1;
#endif
    // sRmEnabledCaps2(){};
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sRmEnabledCaps2;

} // namespace assoc_frame

#endif // _ASSOCIATION_FRAME_STRUCTS_
