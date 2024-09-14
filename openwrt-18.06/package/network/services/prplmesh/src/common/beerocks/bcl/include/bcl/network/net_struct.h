/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _NET_STRUCT_
#define _NET_STRUCT_

#include <cstring>
#include <stdint.h>
#include <string>

#include <tlvf/tlvftypes.h>

namespace beerocks {
namespace net {

enum eNetworkStructsConsts {
    MAC_ADDR_LEN = 6,
    IP_ADDR_LEN  = 4,
};

typedef struct sIpv4Addr {
    uint8_t oct[IP_ADDR_LEN];
    void struct_swap() {}
    void struct_init() { std::memset(oct, 0, IP_ADDR_LEN); }
    bool operator==(sIpv4Addr const &rhs) const
    {
        return (0 == std::memcmp(this->oct, rhs.oct, IP_ADDR_LEN));
    }
    bool operator!=(sIpv4Addr const &rhs) const { return !(rhs == *this); }
} __attribute__((packed)) sIpv4Addr;

typedef struct sScanResult {
    sMacAddr mac;
    uint8_t channel;
    int8_t rssi;
    void struct_swap() {}
    void struct_init()
    {
        mac.struct_init();
        channel = 0;
        rssi    = 0;
    }
    bool operator==(sScanResult const &rhs) const
    {
        return (channel == rhs.channel && rssi == rhs.rssi && mac == rhs.mac);
    }
    bool operator!=(sScanResult const &rhs) const { return !(rhs == *this); }
} __attribute__((packed)) sScanResult;

/**
 * @brief Interface statistics.
 *
 * Information in this structure is obtained from IFLA_STATS attribute of a rtnetlink message
 * through a Rtnetlink socket.
 */
struct sInterfaceStats {
    /**
     * Total bytes transmitted.
     */
    uint32_t tx_bytes = 0;

    /**
     * Packet transmit problems.
     */
    uint32_t tx_errors = 0;

    /**
     * Total packets transmitted.
     */
    uint32_t tx_packets = 0;

    /**
     * Total bytes received.
     */
    uint32_t rx_bytes = 0;

    /**
     * Bad packets received.
     */
    uint32_t rx_errors = 0;

    /**
     * Total packets received.
     */
    uint32_t rx_packets = 0;
};

typedef struct sHTCapabilities {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint8_t reserved : 1;
    uint8_t ht_support_40mhz : 1;
    uint8_t short_gi_support_40mhz : 1;
    uint8_t short_gi_support_20mhz : 1;
    uint8_t max_num_of_supported_rx_spatial_streams : 2;
    uint8_t max_num_of_supported_tx_spatial_streams : 2;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint8_t max_num_of_supported_tx_spatial_streams : 2;
    uint8_t max_num_of_supported_rx_spatial_streams : 2;
    uint8_t short_gi_support_20mhz : 1;
    uint8_t short_gi_support_40mhz : 1;
    uint8_t ht_support_40mhz : 1;
    uint8_t reserved : 1;
#else
#error "Bitfield macros are not defined"
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sHTCapabilities;

typedef struct sVHTCapabilities {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint16_t reserved : 4;
    uint16_t mu_beamformer_capable : 1;
    uint16_t su_beamformer_capable : 1;
    uint16_t vht_support_160mhz : 1;
    uint16_t vht_support_80_80mhz : 1;
    uint16_t short_gi_support_160mhz_and_80_80mhz : 1;
    uint16_t short_gi_support_80mhz : 1;
    uint16_t max_num_of_supported_rx_spatial_streams : 3;
    uint16_t max_num_of_supported_tx_spatial_streams : 3;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint16_t max_num_of_supported_tx_spatial_streams : 3;
    uint16_t max_num_of_supported_rx_spatial_streams : 3;
    uint16_t short_gi_support_80mhz : 1;
    uint16_t short_gi_support_160mhz_and_80_80mhz : 1;
    uint16_t vht_support_80_80mhz : 1;
    uint16_t vht_support_160mhz : 1;
    uint16_t su_beamformer_capable : 1;
    uint16_t mu_beamformer_capable : 1;
    uint16_t reserved : 4;
#else
#error "Bitfield macros are not defined"
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sVHTCapabilities;

typedef struct sHECapabilities {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint16_t reserved : 1;
    uint16_t dl_ofdm_capable : 1;
    uint16_t ul_ofdm_capable : 1;
    uint16_t dl_mu_mimo_and_ofdm_capable : 1;
    uint16_t ul_mu_mimo_and_ofdm_capable : 1;
    uint16_t ul_mu_mimo_capable : 1;
    uint16_t mu_beamformer_capable : 1;
    uint16_t su_beamformer_capable : 1;
    uint16_t he_support_160mhz : 1;
    uint16_t he_support_80_80mhz : 1;
    uint16_t max_num_of_supported_rx_spatial_streams : 3;
    uint16_t max_num_of_supported_tx_spatial_streams : 3;
#elif defined(__BIG_ENDIAN_BITFIELD)
    uint16_t max_num_of_supported_tx_spatial_streams : 3;
    uint16_t max_num_of_supported_rx_spatial_streams : 3;
    uint16_t he_support_80_80mhz : 1;
    uint16_t he_support_160mhz : 1;
    uint16_t su_beamformer_capable : 1;
    uint16_t mu_beamformer_capable : 1;
    uint16_t ul_mu_mimo_capable : 1;
    uint16_t ul_mu_mimo_and_ofdm_capable : 1;
    uint16_t dl_mu_mimo_and_ofdm_capable : 1;
    uint16_t ul_ofdm_capable : 1;
    uint16_t dl_ofdm_capable : 1;
    uint16_t reserved : 1;
#else
#error "Bitfield macros are not defined"
#endif
    void struct_swap() {}
    void struct_init() {}
} __attribute__((packed)) sHECapabilities;

} // namespace net
} // namespace beerocks

#endif
