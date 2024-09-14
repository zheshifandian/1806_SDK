/**
 *
 ******************************************************************************
 *
 * @file siwifi_frame.c
 *
 * @brief SIWIFI frame custom
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#include "siwifi_frame.h"
#include "siwifi_mem.h"
#include "siwifi_msg_tx.h"

/* Elemment ID : 1 - Supported Rates
   length 8
   Supported Rate (default value)
    bit 0-7 : value * Unit 0.5Mbps
    bit 8   : if the rate is must be supported
    //5G
    0x8c   0b1 000 1100      6 Mbps  BSS Basic Rate
    0x12   0b0 001 0010      9 Mbps  Not BSS Basic Rate
    0x98   0b1 001 1000     12 Mbps  BSS Basic Rate
    0x24   0b0 010 0010     18 Mbps  Not BSS Basic Rate
    0xb0   0b1 011 0000     24 Mbps  BSS Basic Rate
    0x48   0b0 100 1000     36 Mbps  Not BSS Basic Rate
    0x60   0b0 110 0000     48 Mbps  Not BSS Basic Rate
    0x6c   0b0 110 1100     36 Mbps  Not BSS Basic Rate
    // 24G
    0x82   0b1 000 0010      1 Mbps  BSS Basic Rate
    0x84   0b1 000 0100      2 Mbps  BSS Basic Rate
    0x8b   0b1 000 1010    5.5 Mbps  BSS Basic Rate
    0x96   0b1 001 0110     11 Mbps  BSS Basic Rate
    0x0c   0b0 000 1100      6 Mbps  Not BSS Basic Rate
    0x12   0b0 001 0010      9 Mbps  Not BSS Basic Rate
    0x18   0b0 001 1000     12 Mbps  Not BSS Basic Rate
    0x24   0b0 010 0100     18 Mbps  Not BSS Basic Rate
 */
static int siwifi_ie_default(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        // default value
        variable[0] = WLAN_EID_SUPP_RATES;
        variable[1] = SIWIFI_IE_SUPP_RATES_LEN;
        if (is_hb){
            variable[2] = 0x8c;
            variable[3] = 0x12;
            variable[4] = 0x98;
            variable[5] = 0x24;
            variable[6] = 0xb0;
            variable[7] = 0x48;
            variable[8] = 0x60;
            variable[9] = 0x6c;
        } else {
            variable[2] = 0x82;
            variable[3] = 0x84;
            variable[4] = 0x8B;
            variable[5] = 0x96;
            variable[6] = 0x0c;
            variable[7] = 0x12;
            variable[8] = 0x18;
            variable[9] = 0x24;
        }
        ie_len = (2 + SIWIFI_IE_SUPP_RATES_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}

/* same as WLAN_EID_SUPP_RATES
    0x30   0b0 011 0000     24 Mbps  Not BSS Basic Rate
    0x48   0b0 100 1000     36 Mbps  Not BSS Basic Rate
    0x60   0b0 110 1000     48 Mbps  Not BSS Basic Rate
    0x6c   0b0 110 1100     54 Mbps  Not BSS Basic Rate
 */
static int siwifi_ie_ext_supp_rates(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        // default value
        variable[0] = WLAN_EID_EXT_SUPP_RATES;
        variable[1] = SIWIFI_IE_EXT_SUPP_RATES_LEN;
        variable[2] = 0x30;
        variable[3] = 0x48;
        variable[4] = 0x60;
        variable[5] = 0x6c;
        ie_len = (2 + SIWIFI_IE_EXT_SUPP_RATES_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}

/*  Elemment ID : 36 Supported Channels
    length 2
*/
static int siwifi_fill_ie_supported_channels(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_SUPPORTED_CHANNELS;
        variable[1] = SIWIFI_IE_SUPPORTED_CHANNELS_LEN;
        variable[2] = 0;
        variable[3] = 0;
        ie_len += (2 + SIWIFI_IE_SUPPORTED_CHANNELS_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

/*  Elemment ID : 7  Country
    length 12
 */
static int siwifi_fill_ie_country(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
            variable[0] = WLAN_EID_COUNTRY;
            if (is_hb) {
            variable[1] = SIWIFI_IE_COUNTRY_LEN;
            // 'C' = 0x43, 'N' = 0x4e
            variable[2] = 'C';
            variable[3] = 'N';
            // Environment: 0x20  Any
            variable[4] = 0x20;
            // Starting Channel: 36 (0x24)
            // Number of Channels: 4 (36 40 44 48)
            // Max Tx Power (dBm): 23 (0x17)
            variable[5] = 36;
            variable[6] = 4;
            variable[7] = 23;
            // Starting Channel: 100 (0x64)
            // Number of Channels: 11
            // Max Tx Power (dBm): 24 (0x18)
            variable[8] = 100;
            variable[9] = 11;
            variable[10] = 24;
            // Starting Channel: 149 (0x95)
            // Number of Channels: 5 (149 153 157 161 165)
            // Max Tx Power (dBm): 30 (0x1e)
            variable[11] = 149;
            variable[12] = 5;
            variable[13] = 30;
            ie_len = (2 + SIWIFI_IE_COUNTRY_LEN);
        } else {
            variable[1] = SIWIFI_IE_LB_COUNTRY_LEN;
            // 'C' = 0x43, 'N' = 0x4e
            variable[2] = 'C';
            variable[3] = 'N';
            // Environment: 0x20  Any
            variable[4] = 0x20;
            // Starting Channel: 36 (0x24)
            // Number of Channels: 4 (36 40 44 48)
            // Max Tx Power (dBm): 23 (0x17)
            variable[5] = 1;
            variable[6] = 13;
            variable[7] = 20;
            ie_len = (2 + SIWIFI_IE_LB_COUNTRY_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}

/*  Elemment ID : 11  QBSS(QOS enhanced basic service set) Load
    length 5
 */
static int siwifi_fill_ie_qbss(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_QBSS_LOAD;
        variable[1] = SIWIFI_IE_QBSS_LEN;
        // Station Count
        variable[2] = 0;
        variable[3] = 0;
        // Channel Utilization (1 / 255) * 100%
        variable[4] = 0x1;
        // Avail Admission Capacity
        variable[5] = 0;
        variable[6] = 0;
        ie_len = (2 + SIWIFI_IE_QBSS_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

/*  Elemment ID : 59 Supported Regulatory Classes
    length 13
 */
static int siwifi_fill_ie_supp_reg(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_SUPPORTED_REGULATORY_CLASSES;
        if (is_hb) {
            variable[1] = SIWIFI_IE_SUPPORTED_REGULATORY_LEN;
            variable[2] = 0x7c;
            variable[3] = 0x73;
            variable[4] = 0x74;
            variable[5] = 0x75;
            variable[6] = 0x79;
            variable[7] = 0x7a;
            variable[8] = 0x7b;
            variable[9] = 0x7c;
            variable[10] = 0x7d;
            variable[11] = 0x7e;
            variable[12] = 0x7f;
            variable[13] = 0x80;
            variable[14] = 0x82;
            ie_len = (2 + SIWIFI_IE_SUPPORTED_REGULATORY_LEN);
        } else {
            variable[1] = SIWIFI_IE_LB_SUPPORTED_REGULATORY_LEN;
            variable[2] = 0x51;
            variable[3] = 0x51;
            variable[4] = 0x53;
            variable[5] = 0x54;
            ie_len = (2 + SIWIFI_IE_LB_SUPPORTED_REGULATORY_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

static int siwifi_fill_ie_custom_supp_reg(uint8_t *variable, int is_hb)
{
    int ie_len = SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2 + 2;

    uint8_t custom_ie[SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2 + 2];
    custom_ie[0] =  WLAN_EID_SUPPORTED_REGULATORY_CLASSES;
    custom_ie[1] =  SIWIFI_IE_SUPPORTED_REGULATORY_LEN_2;
    if (is_hb)
        custom_ie[2] = 0x80;
    else
        custom_ie[2] = 0x51;
    custom_ie[3] = 0;
    return siwifi_fill_ie_supp_reg(variable, custom_ie, ie_len, is_hb);
}


/*  Elemment ID : 45  HT Capability Info
    length 26
 */
static int siwifi_fill_ie_ht_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_HT_CAPABILITY;
        variable[1] = SIWIFI_IE_HT_CAPABILITY_LEN;
        /*  HT capability info  (bit 0 - 15) : 0x09ff
                        0000 1001 1111 1111
                bit 15: 0... .... .... .... L-SIG TXOP Protection Support: Not Supported
                bit 14:  0.. .... .... .... AP is not required to restrict the use of 40 MHz transmissions within its BSS
                bit 13:   0. .... .... .... Reserved
                bit 12:    0 .... .... .... Device will Not use DSSS/CCK Rates @40MHz
                bit 11:      1... .... .... Maximal A-MSDU size: 7935 bytes
                bit 10:       0.. .... .... Does Not Support HT-Delayed BlockAck Operation
                bit  8:        01 .... .... Rx STBC: Rx Support of One Spatial Stream
                bit  7:           1... .... Transmitter does Support Tx STBC
                bit  6:            1.. .... Short GI for 40 MHz: Supported
                bit  5:             1. .... Short GI for 20 MHz: Supported
                bit  4:              1 .... Can receive PPDUs with HT-Greenfield format
                // probe resp        0 .... Can Not receive PPDUs with HT-Greenfield format
                bit  2:                11.. SM Power Save Disabled
                bit  1:                  1. Both 20MHz and 40MHz Operation is Supported
                bit  0:                   1 LDPC coding capability: Supported                    */
        variable[2] = 0xff;
        if (is_hb)
            variable[3] = 0x9;
        else
            variable[3] = 0x1;
        /*  A-MPDU Parameters
                        0001 1111
                bit  5: 000                 Reserved
                bit  2:    1 11             Minimum MPDU Start Spacing: 16 usec
                bit  0:        11           Maximum Rx A-MPDU Size: 65,535 Bytes                 */
        variable[4] = 0x1f;
        /*  Supported MCS Set
            Spatial Stream 1-4
            Rx Bitmask     1-6          */
        variable[5] = 0xff;
        variable[6] = 0xff;
        variable[7] = 0;
        variable[8] = 0;
        // maybe set 0
        // variable[assoc_ie_len+9] = 0x1;
        variable[9] = 0;
        variable[10] = 0;
        variable[11] = 0;
        variable[12] = 0;
        variable[13] = 0;
        variable[14] = 0;
        // Highest Supported Rate (MASK 0x3FF) 0x12c maybe set 0
        // variable[assoc_ie_len+15] = 0x2c;
        // variable[assoc_ie_len+16] = 0x1;
        variable[15] = 0;
        variable[16] = 0;
        // Tx Supported MCS Set (MASK  0x1): 1 maybe set 0
        // Tx and Rx MCS Set    (MASK  0x2): 0 Equal
        // Tx Max # Spatial Streams Supported (MASK 0xc) 0x0, 2 spatial streams
        // Tx Unequal Modulation(MASK 0x10): 0  Not Supported
        // Reserved (MASK 0xe0)
        // variable[assoc_ie_len+17] = 1;
        variable[17] = 0;
        // Reserved 3 bytes
        variable[18] = 0;
        variable[19] = 0;
        variable[20] = 0;
        // HT Extended Capabilities Info
        variable[21] = 0;
        variable[22] = 0;
        // Tx Beam Forming Capability
        variable[23] = 0;
        variable[24] = 0;
        variable[25] = 0;
        variable[26] = 0;
        // Antenna Selection Capability
        variable[27] = 0;
        ie_len = (2 + SIWIFI_IE_HT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

static int siwifi_fill_ie_ext_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        // Perhaps it's not necessary
        // Elemment ID : 127  Extended Capabilities
        variable[0] = WLAN_EID_EXT_CAPABILITY;
        // length 8
        variable[1] = SIWIFI_IE_EXT_CAPABILITY_LEN;
        // Extended Channel Switching Supported
        variable[2] = 4;
        variable[3] = 0;
        // BSSTransitionActivated is true
        // WNMSleepModeActivated is true
        // variable[assoc_ie_len+4] = 0xa; // assoc req
        variable[4] = 0;  // probe resp
        // SSIDListActivated is true
        variable[5] = 2;
        variable[6] = 0;
        variable[7] = 0;
        // The SSID in this BSS is interpreted using UTF-8 encoding
        // variable[assoc_ie_len+8] = 0;  // assoc req
        variable[8] = 1; // probe resp
        // OperatingModeNotificationImplemented is true
        variable[9] = 0x40;
        ie_len = (2 + SIWIFI_IE_EXT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

extern uint16_t siwifi_freq_to_channel(uint16_t freq);
/*  Elemment ID : 3  Direct Sequence Parameter Set
    length 1
*/
static int siwifi_fill_ie_dsps(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t channel_num = siwifi_freq_to_channel(siwifi_vif->siwifi_hw->chanctx_table[siwifi_vif->ch_index].chan_def.chan->center_freq);

    variable[0] = WLAN_EID_DS_PARAMS;
    variable[1] = SIWIFI_IE_DSPS_LEN;
    variable[2] = channel_num;
    ie_len = (2 + SIWIFI_IE_DSPS_LEN);

    return ie_len;
}

/*  Elemment ID : 61  HT Operation Information
    length 22
 */
static int siwifi_fill_ie_ht_operation(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t channel_num = siwifi_freq_to_channel(siwifi_vif->siwifi_hw->chanctx_table[siwifi_vif->ch_index].chan_def.chan->center_freq);

    variable[0] = WLAN_EID_HT_OPERATION;
    variable[1] = SIWIFI_IE_HT_OPERATION_LEN;
    variable[2] = channel_num;
    /* HT Operation Element 1
                 0000 0111
         bit  4: xxxx .... Reserved
         bit  3: .... 0... RIFS Mode: Use of RIFS Prohibited
         bit  2: .... .1.. STA Channel Width: Use Any Channel Width Enabled Under Supported Channel Width Set
         // TODO
         bit  0: .... ..11 2nd Channel Offset: Below the Primary Channel
     */
    variable[3] = 0x7;
    /*
      HT Operation Element 2
                 0000 0000 0000 0000
         bit 13: xxx. .... .... .... Reserved
         bit  5: ...0 0000 000. .... Channel Center Frequency Segment 2: 0
         bit  4: .... .... ...0 .... OBSS Non-HT STAs: Use of Protection for Non-HT STAs Not Needed
         bit  3: .... .... .... 0... Reserved: not set
         bit  2: .... .... .... .0.. Non-Greenfield STAs: HT STAs associated are Greenfield Capable
         bit  0: .... .... .... ..00 HT Protection: Pure HT (No Protection) - All STAs in the BSS are 20/40 MHz HT
     */
    variable[4] = 0;
    variable[5] = 0;
    /*  HT Operation Element 2
                 0000 0000 0000 0000
         bit 12: xxxx .... .... .... Reserved
         bit 11: .... 0... .... .... PCO Phase: Switch To/Continue Use 20MHz Phase
         bit 10: .... .0.. .... .... PCO Active: Not Active in the BSS
         bit  9: .... ..0. .... .... L-SIG TXOP Protection: Not Full Support
         bit  8: .... ...0 .... .... Secondary Beacon: Primary Beacon
         bit  7: .... .... 0... .... Dual CTS Protection: Not Required
         bit  6: .... .... .0.. .... Dual Beacon: No Secondary Beacon Transmitted
         bit  0: .... .... ..xx xxxx Reserved
     */
    variable[6] = 0;
    variable[7] = 0;
    // Basic MCS Set
    variable[8] = 0;
    variable[9] = 0;
    variable[10] = 0;
    variable[11] = 0;
    variable[12] = 0;
    variable[13] = 0;
    variable[14] = 0;
    variable[15] = 0;
    variable[16] = 0;
    variable[17] = 0;
    variable[18] = 0;
    variable[19] = 0;
    variable[20] = 0;
    variable[11] = 0;
    variable[22] = 0;
    variable[23] = 0;
    ie_len = (2 + SIWIFI_IE_HT_OPERATION_LEN);

    return ie_len;
}

/*  Elemment ID : 191  VHT Capabilities element
    length 12
 */
static int siwifi_fill_ie_vht_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_VHT_CAPABILITY;
        variable[1] = SIWIFI_IE_VHT_CAPABILITY_LEN;
        /*
            VHT capability info (bit 0 - 31) : 0x038001b1
                        0000 0011 1000 0000
                bit 30: 00.. .... .... .... Reserved
                bit 29:   0. .... .... .... Tx Antenna Pattern Consistency: Tx antenna pattern might change
                bit 28:    0 .... .... .... Rx Antenna Pattern Consistency: Rx antenna pattern might change
                bit 26:      00.. .... .... VHT Link Adaptation Capable: No Feedback
                bit 23:        11 1... .... Maximum AMPDU Length Exponent: 1048575
                bit 22:            0.. .... +HTC-VHT Capable: Not Supported
                bit 21:             0. .... VHT TXOP PS: Not Supported
                bit 20:              0 .... MU Beamformee Capable: Not Supported
                bit 19:                0... MU Beamformer Capable: Not Supported
                bit 16:                 000 Number of Sounding Dimensions: 1
                        0000 1001 1111 1111
                bit 13: 000. .... .... .... Compressed Steering Number of Beamformer Antennas Supported: reserved
                bit 12:    0 .... .... .... SU Beamformee Capable: Not Supported
                bit 11:      0... .... .... SU Beam-former Capable: Not Supported
                bit  8:       001 .... .... Rx STBC: support of one spatial stream
                bit  7:           1... .... Tx STBC: Supported
                bit  6:            0.. .... Short GI for 160 and 80+80 MHz: Not Supported
                bit  5:             1. .... Short GI for 80 MHz: Supported
                bit  4:              1 .... Rx LDPC: Supported
                bit  2:                00.. Supported Channel Width Set:  no support for 160 or 80+80 MHz
                bit  0:                  01 Maximum MPDU Length: 7991 octets
        */

        variable[2] = 0xb1;
        variable[3] = 0x1;
        variable[4] = 0x80;
        variable[5] = 0x3;
        /*
            VHT Supported MCS Set
                        1111 1111 1111 1010
                bit 15: 1111 1111 11.. .... RX 4-8 SS: Not Supported (0x3)
                bit  4:             11 .... RX 3 SS: Not Supported (0x3)
                bit  2:                10.. RX 2 SS: MCS 0-9 (0x2)
                bit  0:                  10 RX 1 SS: MCS 0-9 (0x2)
                        0000 0011 0000 1100
                bit 13: 000. .... .... .... Max NSTS Total: 0
                bit  0:    0 0011 0000 1100 Rx Highest Long GI Data Rate (in Mb/s, 0 = subfield not in use): 0x030c
                        1111 1111 1111 1010
                bit 15: 1111 1111 11.. .... TX 4-8 SS: Not Supported (0x3)
                bit  4:             11 .... TX 3 SS: Not Supported (0x3)
                bit  2:                10.. TX 2 SS: MCS 0-9 (0x2)
                bit  0:                  10 TX 1 SS: MCS 0-9 (0x2)
                        0000 0011 0000 1100
                bit 14: 00.. .... .... .... Reserved
                bit 13:   0. .... .... .... Extended NSS BW Capable: Not capable
                bit  0:    0 0011 0000 1100 Rx Highest Long GI Data Rate (in Mb/s, 0 = subfield not in use): 0x030c
         */
        variable[6] = 0xfa;
        variable[7] = 0xff;
        variable[8] = 0xc;
        variable[9] = 0x3;
        variable[10] = 0xfa;
        variable[11] = 0xff;
        variable[12] = 0xc;
        variable[13] = 0x3;
        ie_len = (2 + SIWIFI_IE_VHT_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

static int siwifi_get_center_freq(struct siwifi_vif *vif)
{
    int center_freq = vif->siwifi_hw->chanctx_table[vif->ch_index].chan_def.chan->center_freq;
    // channel 149~165
    if (center_freq > 5735 && center_freq < 5835)
        return 155;

    // channel 36~48
    if (center_freq > 5170 && center_freq < 5250)
        return 42;

    // channel 52~64
    if (center_freq > 5250 && center_freq < 5330)
        return 58;

    // channel 100~144
    printk("get freq(%d) center fail\n", center_freq);
    return 0;
}

/*  Elemment ID : 192  VHT Operation element
    length 5
 */
static int siwifi_fill_ie_vht_operation(struct siwifi_vif *siwifi_vif, uint8_t *variable)
{
    int ie_len = 0;
    uint8_t center_freq = siwifi_get_center_freq(siwifi_vif);

    if (!center_freq)
        return 0;

    variable[0] = WLAN_EID_VHT_OPERATION;
    variable[1] = SIWIFI_IE_VHT_OPERATION_LEN;
    // Channel Width: 1  80 MHz
    variable[2] = 1;
    // TODO
    // Center Frequency Channel for 80 and 160 MHz operation: ??? MHz
    variable[3] = center_freq;
    // Center Frequency Channel for 80+80 MHz operation: 0  MHz
    variable[4] = 0;
    // VHT Basic MCS Set
    variable[5] = 0xfc;
    variable[6] = 0xff;
    ie_len = (2 + SIWIFI_IE_VHT_OPERATION_LEN);

    return ie_len;
}

/*  Elemment ID : 195  VHT Transmit Power Envelope
    length 4
 */
static int siwifi_fill_ie_vht_tx_power_envelope(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_VHT_TX_POWER_ENVELOPE;
        variable[1] = SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN;
        /*
          Transmit Power Information:0x02  [175]
                     0000 0010
              bit 6: xx.. .... Reserved
              bit 3: ..00 0... Local Maximum Transmit Power Units Interpretation: EIRP
              bit 0: .... .010 Local Maximum Transmit Power for: 20 MHz / 40 MHz / 80 MHz
            Local Maximum Transmit Power For 20 MHz:60  Max limit 30 dBm
            Local Maximum Transmit Power For 40 MHz:60  Max limit 30 dBm
            Local Maximum Transmit Power For 80 MHz:60  Max limit 30 dBm
        */
        variable[2] = 0x02;
        variable[3] = 0x3c;
        variable[4] = 0x3c;
        variable[5] = 0x3c;
        ie_len = (2 + SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

/*  Elemment ID : 221  WMM
    length 24
 */
static int siwifi_fill_ie_wmm(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_VENDOR_SPECIFIC;
        variable[1] = SIWIFI_IE_WMM_LEN_2;
        // 0x00-0x50-0xF2  MICROSOFT CORP.
        variable[2] = 0;
        variable[3] = 0x50;
        variable[4] = 0xf2;
        // OUI Type WMM/WME (0x2)
        variable[5] = 0x02;
        // WME Subtype: Parameter Element (1)
        variable[6] = 0x01;
        // WME Version: 1
        variable[7] = 0x01;
        /* WME QoS Info: 0x01
                    0000 0001
            bit  7: 0... .... WMM AP does not support U-APSD
            bit  4: .xxx .... Reserved
            bit  0: .... 0001 Parameter Set Count: 1
         */
        variable[8] = 0x01;
        // Reserved
        variable[9] = 0;
        /*ACI/AIFSN:    0000 0011
                        x... .... Reserved
                        .00. .... ACI: Best Effort
                        ...0 .... ACM: Admission Control Not Mandatory
                        .... 0011 AIFSN: 3
          ECW Min/Max:  1010 .... ECW Max: 10 (CW Max: 1,023)
                        .... 0100 ECW Min: 4 (CW Min: 15)
          TXOP Limit: 0 (2 bytes)
         */
        variable[10] = 0x03;
        variable[11] = 0xa4;
        variable[12] = 0;
        variable[13] = 0;
        /*ACI/AIFSN:    0010 0111
                        x... .... Reserved
                        .01. .... ACI: Background
                        ...0 .... ACM: Admission Control Not Mandatory
                        .... 0111 AIFSN: 7
          ECW Min/Max:  1010 .... ECW Max: 10 (CW Max: 1,023)
                        .... 0100 ECW Min: 4 (CW Min: 15)
          TXOP Limit: 0 (2 bytes)
         */
        variable[14] = 0x27;
        variable[15] = 0xa4;
        variable[16] = 0;
        variable[17] = 0;
        /*ACI/AIFSN:    0100 0010
                        x... .... Reserved
                        .10. .... ACI: Video
                        ...0 .... ACM: Admission Control Not Mandatory
                        .... 0010 AIFSN: 2
          ECW Min/Max:  0100 .... ECW Max: 4 (CW Max: 15)
                        .... 0011 ECW Min: 3 (CW Min: 7)
          TXOP Limit: 94 (2 bytes)
         */
        variable[18] = 0x42;
        variable[19] = 0x43;
        variable[20] = 0x5e;
        variable[11] = 0;
        /*ACI/AIFSN:    0110 0010
                        x... .... Reserved
                        .11. .... ACI: Voice
                        ...0 .... ACM: Admission Control Not Mandatory
                        .... 0010 AIFSN: 2
          ECW Min/Max:  0011 .... ECW Max: 3 (CW Max: 7)
                        .... 0010 ECW Min: 2 (CW Min: 3)
          TXOP Limit: 47 (2 bytes)
         */
        variable[22] = 0x62;
        variable[23] = 0x32;
        variable[24] = 0x2f;
        variable[25] = 0;
        ie_len = (2 + SIWIFI_IE_WMM_LEN_2);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

static int siwifi_fill_ie_custom_wmm(uint8_t *variable)
{
    int ie_len = SIWIFI_IE_WMM_LEN + 2;
    uint8_t custom_ie[SIWIFI_IE_WMM_LEN + 2] = {
        // Elemment ID : 221  WMM
        WLAN_EID_VENDOR_SPECIFIC,
        // length 7
        SIWIFI_IE_WMM_LEN,
        // 0x00-0x50-0xF2  MICROSOFT CORP.
        0,
        0x50,
        0xf2,
        // OUI Type
        0x2,
        // OUI SubType
        0,
        // Version
        0x1,
        // QoS Info
        0
    };

    return siwifi_fill_ie_wmm(variable, custom_ie, ie_len);
}

/*  Elemment ID : 33 Power Capability
    length 2
 */
static int siwifi_fill_ie_pwr_capability(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len)
{
    int ie_len = 0;
    if (!custom_ie) {
        variable[0] = WLAN_EID_PWR_CAPABILITY;
        variable[1] = SIWIFI_IE_PWR_CAPABILITY_LEN;
        variable[2] = 0;
        variable[3] = 25;
        ie_len = (2 + SIWIFI_IE_PWR_CAPABILITY_LEN);
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }

    return ie_len;
}

/*  Elemment ID : 42 ERP information
    length 1
 */
static int siwifi_fill_ie_erp_information(uint8_t *variable, uint8_t *custom_ie, int custom_ie_len, int is_hb)
{
    int ie_len = 0;
    if (!custom_ie) {
        if (!is_hb) {
            variable[0] = WLAN_EID_ERP_INFO;
            variable[1] = SIWIFI_IE_ERP_INFORMATION_LEN;
            // TODO
            variable[2] = 0x02;
            ie_len = (2 + SIWIFI_IE_ERP_INFORMATION_LEN);
        }
    } else {
        memcpy(variable, custom_ie, custom_ie_len);
        ie_len = custom_ie_len;
    }
    return ie_len;
}

static int siwifi_fill_ie_default(uint8_t *variable, enum ieee80211_eid eid, int is_hb)
{
    int ret = 0;
    switch(eid)
    {
        case WLAN_EID_SUPP_RATES:
            ret = siwifi_ie_default(variable, NULL, SIWIFI_IE_SUPP_RATES_LEN, is_hb);
            break;
        case WLAN_EID_EXT_SUPP_RATES:
            ret = siwifi_ie_ext_supp_rates(variable, NULL, SIWIFI_IE_EXT_SUPP_RATES_LEN);
            break;
        case WLAN_EID_SUPPORTED_CHANNELS:
            ret = siwifi_fill_ie_supported_channels(variable, NULL, SIWIFI_IE_SUPPORTED_CHANNELS_LEN);
            break;
        case WLAN_EID_COUNTRY:
            ret = siwifi_fill_ie_country(variable, NULL, SIWIFI_IE_COUNTRY_LEN, is_hb);
            break;
        case WLAN_EID_QBSS_LOAD:
            ret = siwifi_fill_ie_qbss(variable, NULL, SIWIFI_IE_QBSS_LEN);
            break;
        case WLAN_EID_SUPPORTED_REGULATORY_CLASSES:
            ret = siwifi_fill_ie_supp_reg(variable, NULL, SIWIFI_IE_SUPPORTED_REGULATORY_LEN, is_hb);
            break;
        case WLAN_EID_HT_CAPABILITY:
            ret = siwifi_fill_ie_ht_capability(variable, NULL, SIWIFI_IE_HT_CAPABILITY_LEN, is_hb);
            break;
        case WLAN_EID_EXT_CAPABILITY:
            ret = siwifi_fill_ie_ext_capability(variable, NULL, SIWIFI_IE_EXT_CAPABILITY_LEN);
            break;
        case WLAN_EID_VHT_CAPABILITY:
            ret = siwifi_fill_ie_vht_capability(variable, NULL, SIWIFI_IE_VHT_CAPABILITY_LEN);
            break;
        case WLAN_EID_VHT_TX_POWER_ENVELOPE:
            ret = siwifi_fill_ie_vht_tx_power_envelope(variable, NULL, SIWIFI_IE_VHT_TX_POWER_ENVELOPE_LEN);
            break;
        case WLAN_EID_VENDOR_SPECIFIC:
            ret = siwifi_fill_ie_wmm(variable, NULL, SIWIFI_IE_WMM_LEN_2);
            break;
        case WLAN_EID_PWR_CAPABILITY:
            ret = siwifi_fill_ie_pwr_capability(variable, NULL, SIWIFI_IE_PWR_CAPABILITY_LEN);
            break;
        case WLAN_EID_ERP_INFO:
            ret = siwifi_fill_ie_erp_information(variable, NULL, SIWIFI_IE_ERP_INFORMATION_LEN, is_hb);
            break;
        default:
            return 0;
    }

    return ret;
}

static int siwifi_fill_beacon_frame(struct siwifi_vif *siwifi_vif, uint8_t *beacon_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt beacon_mgmt;
    int beacon_length = 0;
    int beacon_ie_len = 0;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b1000 (beacon)
        frame control flags (bit 8 - 15) : 0                        */
    beacon_mgmt.frame_control = 0x80;
    beacon_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 0                        */
    beacon_mgmt.duration = 0;
    beacon_length += 2;

    ether_addr_copy(beacon_mgmt.da, node_mac);
    ether_addr_copy(beacon_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(beacon_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    beacon_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0
     */
    beacon_mgmt.seq_ctrl = 0x00;
    beacon_length += 2;

    /*  __le64 timestamp;
        __le16 beacon_int;
     */
    beacon_mgmt.u.beacon.timestamp = 0;
    // 100  Time Units (102 Milliseconds, and 400 Microseconds)
    beacon_mgmt.u.beacon.beacon_int = 100;

    /*  __le16 capab_info;
        capability info     (bit 0 - 15) : 0x0521
                    0000 0101 0010 0001
            bit 15: 0... .... .... .... Immediate Block Ack Not Allowed
            bit 14:  0.. .... .... .... Delayed Block Ack Not Allowed
            bit 13:   0. .... .... .... DSSS-OFDM is Not Allowed
            bit 12:    0 .... .... .... No Radio Measurement
            bit 11:      0... .... .... APSD is not supported
            bit 10:       0.. .... .... G Mode Short Slot Time [20 microseconds] TODO
            bit  9:        0. .... .... QoS is Not Supported
            bit  8:         0 .... .... Spectrum Mgmt Disabled TODO
            bit  7:           0... .... Channel Agility Not Used
            bit  6:            0.. .... PBCC Not Allowed
            bit  5:             1. .... Short Preamble
            bit  4:              0 .... Privacy Disabled
            bit  3:                0... CF Poll Not Requested
            bit  2:                 0.. CF Not Pollable
            bit  1:                  0. Not an IBSS Type Network
            bit  0:                   1 ESS Type Network
     */
    beacon_mgmt.u.beacon.capab_info = 0x21;
    beacon_length += 12;

    memcpy(beacon_frame, &beacon_mgmt, beacon_length);
    /*  u8 variable[0];
        ie_info
     */
    variable = beacon_frame + beacon_length;

    // Elemment ID : 0 - SSID
    variable[beacon_ie_len] = WLAN_EID_SSID;
    // SSID length
    variable[beacon_ie_len+1] = siwifi_vif->ap_settings->ssid_len;
    memcpy((variable + beacon_ie_len + 2), siwifi_vif->ap_settings->ssid, siwifi_vif->ap_settings->ssid_len);
    beacon_ie_len += (2 + siwifi_vif->ap_settings->ssid_len);

    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    beacon_ie_len += siwifi_fill_ie_dsps(siwifi_vif, variable + beacon_ie_len);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_COUNTRY, is_hb);
    // TODO TIM len5 beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_TIM, is_hb);
    if (!is_hb)
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_ERP_INFO, is_hb);
    if (!is_hb)
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_QBSS_LOAD, is_hb);
    beacon_ie_len += siwifi_fill_ie_custom_supp_reg(variable + beacon_ie_len, is_hb);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    beacon_ie_len += siwifi_fill_ie_ht_operation(siwifi_vif, variable + beacon_ie_len);
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    if (is_hb) {
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);
        beacon_ie_len += siwifi_fill_ie_vht_operation(siwifi_vif, variable + beacon_ie_len);
        beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_VHT_TX_POWER_ENVELOPE, is_hb);
    }
    beacon_ie_len += siwifi_fill_ie_default(variable + beacon_ie_len, WLAN_EID_VENDOR_SPECIFIC, is_hb);

    beacon_length += beacon_ie_len;
    if (beacon_length > length) {
        printk("beacon frame length[%d] is larger than skb length[%d] \n", beacon_length, length);
        return 0;
    }

    return beacon_length;

}

static int siwifi_fill_auth_req_frame(struct siwifi_vif *siwifi_vif, uint8_t *auth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt auth_mgmt;
    int auth_length = 0;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b1011 (authentication)
        frame control flags (bit 8 - 15) : 0                        */
    auth_mgmt.frame_control = 0xb0;
    auth_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 60                       */
    auth_mgmt.duration = 0x3c;
    auth_length += 2;

    ether_addr_copy(auth_mgmt.da, node_mac);
    ether_addr_copy(auth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(auth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    auth_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0                        */
    auth_mgmt.seq_ctrl = 0;
    auth_length += 2;
    /*  __le16 auth_alg
        __le16 auth_transaction
        __le16 status_code
        auth algorithm      (bit 0 - 15) : 0 (open system)
        auth seq num        (bit 0 - 15) : 1
        reserved            (bit 0 - 15) : 0                        */
    auth_mgmt.u.auth.auth_alg = 0;
    auth_mgmt.u.auth.auth_transaction = 1;
    auth_mgmt.u.auth.status_code = 0;
    auth_length += 6;
    /*  u8 variable[0];
        nothing          */

    if (auth_length > length) {
        printk("auth req frame length[%d] is larger than skb length[%d] \n", auth_length, length);
        return 0;
    }

    printk("fill auth length(%d) ie_len(0)\n", auth_length);

    memcpy(auth_frame, &auth_mgmt, auth_length);
    return auth_length;
}

static int siwifi_fill_auth_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *auth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt auth_mgmt;
    int auth_length = 0;
    int auth_ie_len = 0;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b1011 (auth)
        frame control flags (bit 8 - 15) : 0                        */
    auth_mgmt.frame_control = 0xB0;
    auth_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 0                        */
    auth_mgmt.duration = 0;
    auth_length += 2;

    ether_addr_copy(auth_mgmt.da, node_mac);
    ether_addr_copy(auth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(auth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    auth_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0
     */
    auth_mgmt.seq_ctrl = 0x00;
    auth_length += 2;

    /*  __le16 auth_alg
        __le16 auth_transaction
        __le16 status_code
        auth algorithm      (bit 0 - 15) : 0 (open system)
        auth seq num        (bit 0 - 15) : 2
        reserved            (bit 0 - 15) : 0 (successful)           */
    auth_mgmt.u.auth.auth_alg = 0;
    auth_mgmt.u.auth.auth_transaction = 2;
    auth_mgmt.u.auth.status_code = 0;
    auth_length += 6;

    memcpy(auth_frame, &auth_mgmt, auth_length);

    if (auth_length > length) {
        printk("auth rsp frame length[%d] is larger than skb length[%d] \n", auth_length, length);
        return 0;
    }

    printk("fill auth length(%d) ie_len(%d)\n", auth_length, auth_ie_len);

    return auth_length;

}

static int siwifi_fill_deauth_frame(struct siwifi_vif *siwifi_vif, uint8_t *deauth_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt deauth_mgmt;
    int deauth_length = 0;
    int deauth_ie_len = 0;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b1100 (deauth)
        frame control flags (bit 8 - 15) : 0                        */
    deauth_mgmt.frame_control = 0xC0;
    deauth_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 0                        */
    deauth_mgmt.duration = 0;
    deauth_length += 2;

    ether_addr_copy(deauth_mgmt.da, node_mac);
    ether_addr_copy(deauth_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(deauth_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    deauth_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0
     */
    deauth_mgmt.seq_ctrl = 0x00;
    deauth_length += 2;

    //  __le16 reason_code
    deauth_mgmt.u.deauth.reason_code = WLAN_REASON_UNSPECIFIED;
    deauth_length += 2;

    memcpy(deauth_frame, &deauth_mgmt, deauth_length);

    if (deauth_length > length) {
        printk("deauth frame length[%d] is larger than skb length[%d] \n", deauth_length, length);
        return 0;
    }

    printk("fill deauth length(%d) ie_len(%d)\n", deauth_length, deauth_ie_len);

    return deauth_length;
}

static int siwifi_fill_assoc_req_frame(struct siwifi_vif *siwifi_vif, uint8_t *assoc_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt assoc_mgmt;
    int assocreq_length = 0;
    int assocreq_ie_len = 0;
    struct cfg80211_ap_settings *siwifi_ap_settings = NULL;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b0000 (association request)
        frame control flags (bit 8 - 15) : 0                            */
    assoc_mgmt.frame_control = 0x00;
    assocreq_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 60                           */
    assoc_mgmt.duration = 0x3c;
    assocreq_length += 2;

    ether_addr_copy(assoc_mgmt.da, node_mac);
    ether_addr_copy(assoc_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(assoc_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    assocreq_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 1                           */
    assoc_mgmt.seq_ctrl = 0x10;
    assocreq_length += 2;
    /*  __le16 capab_info;
        capability info     (bit 0 - 15) : 0x0521
                    0000 0101 0010 0001
            bit 15: 0... .... .... .... Immediate Block Ack Not Allowed
            bit 14:  0.. .... .... .... Delayed Block Ack Not Allowed
            bit 13:   0. .... .... .... DSSS-OFDM is Not Allowed
            bit 12:    0 .... .... .... No Radio Measurement
            bit 11:      0... .... .... APSD is not supported
            bit 10:       1.. .... .... G Mode Short Slot Time [9 microseconds]
            bit  9:        0. .... .... QoS is Not Supported
            bit  8:         1 .... .... Spectrum Mgmt Enabled
            bit  7:           0... .... Channel Agility Not Used
            bit  6:            0.. .... PBCC Not Allowed
            bit  5:             1. .... Short Preamble
            bit  4:              0 .... Privacy Disabled
            bit  3:                0... CF Poll Not Requested
            bit  2:                 0.. CF Not Pollable
            bit  1:                  0. Not an IBSS Type Network
            bit  0:                   1 ESS Type Network
     */
    assoc_mgmt.u.assoc_req.capab_info = 0x0521;
    /*    __le16 listen_interval;
        listen interval     (bit 0 - 15) : 5      */
    assoc_mgmt.u.assoc_req.listen_interval = 5;
    assocreq_length += 4;

    memcpy(assoc_frame, &assoc_mgmt, assocreq_length);
    /*  u8 variable[0];
        ie_info                                   */
    variable = assoc_frame + assocreq_length;

    siwifi_ap_settings = siwifi_vif->ap_settings;
    // Elemment ID : 0 - SSID
    variable[assocreq_ie_len] = WLAN_EID_SSID;
    // SSID length
    variable[assocreq_ie_len+1] = siwifi_ap_settings->ssid_len;
    memcpy((variable + assocreq_ie_len + 2), siwifi_ap_settings->ssid, siwifi_ap_settings->ssid_len);
    assocreq_ie_len += (2 + siwifi_ap_settings->ssid_len);

    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    if (!is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_PWR_CAPABILITY, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPPORTED_CHANNELS, is_hb);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_SUPPORTED_REGULATORY_CLASSES, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_custom_wmm(variable + assocreq_ie_len);
    assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    if (is_hb)
        assocreq_ie_len += siwifi_fill_ie_default(variable + assocreq_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);


    assocreq_length += assocreq_ie_len;

    if (assocreq_length > length) {
        printk("assoc frame length[%d] is larger than skb length[%d] \n", assocreq_length, length);
        return 0;
    }

//    printk("fill assoc length(%d) ie_len(%d)\n", assocreq_length, assocreq_ie_len);

    return assocreq_length;
}

static int siwifi_fill_assoc_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *assoc_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt assocresp_mgmt;
    int assocresp_length = 0;
    int assocresp_ie_len = 0;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;
    uint8_t *variable = NULL;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b0001 (assoc response)
        frame control flags (bit 8 - 15) : 0                        */
    assocresp_mgmt.frame_control = 0x10;
    assocresp_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 0                        */
    assocresp_mgmt.duration = 0;
    assocresp_length += 2;

    ether_addr_copy(assocresp_mgmt.da, node_mac);
    ether_addr_copy(assocresp_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(assocresp_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    assocresp_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0
     */
    assocresp_mgmt.seq_ctrl = 0x00;
    assocresp_length += 2;

    /*  __le16 capab_info;
        capability info     (bit 0 - 15) : 0x0521
                    0000 0101 0010 0001
            bit 15: 0... .... .... .... Immediate Block Ack Not Allowed
            bit 14:  0.. .... .... .... Delayed Block Ack Not Allowed
            bit 13:   0. .... .... .... DSSS-OFDM is Not Allowed
            bit 12:    0 .... .... .... No Radio Measurement
            bit 11:      0... .... .... APSD is not supported
            bit 10:       0.. .... .... G Mode Short Slot Time [20 microseconds] TODO
            bit  9:        0. .... .... QoS is Not Supported
            bit  8:         0 .... .... Spectrum Mgmt Disabled TODO
            bit  7:           0... .... Channel Agility Not Used
            bit  6:            0.. .... PBCC Not Allowed
            bit  5:             1. .... Short Preamble
            bit  4:              0 .... Privacy Disabled
            bit  3:                0... CF Poll Not Requested
            bit  2:                 0.. CF Not Pollable
            bit  1:                  0. Not an IBSS Type Network
            bit  0:                   1 ESS Type Network
     */
    assocresp_mgmt.u.assoc_resp.capab_info = 0x21;
    /*  __le16 status_code : 0 - successful
        __le16 aid;
     */
    assocresp_mgmt.u.assoc_resp.status_code = 0;
    assocresp_mgmt.u.assoc_resp.aid = 1;
    assocresp_length += 6;

    memcpy(assoc_frame, &assocresp_mgmt, assocresp_length);
    /*  u8 variable[0];
        ie_info
     */
    variable = assoc_frame + assocresp_length;

    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_SUPP_RATES, is_hb);
    if (!is_hb)
        assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_EXT_SUPP_RATES, is_hb);
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_HT_CAPABILITY, is_hb);
    assocresp_ie_len += siwifi_fill_ie_ht_operation(siwifi_vif, variable + assocresp_ie_len);
    if (is_hb) {
        assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_VHT_CAPABILITY, is_hb);
        assocresp_ie_len += siwifi_fill_ie_vht_operation(siwifi_vif, variable + assocresp_ie_len);
    }
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_EXT_CAPABILITY, is_hb);
    // TODO assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_BSS_MAX_IDLE_PERIOD);
    assocresp_ie_len += siwifi_fill_ie_default(variable + assocresp_ie_len, WLAN_EID_VENDOR_SPECIFIC, is_hb);

    assocresp_length += assocresp_ie_len;
    if (assocresp_length > length) {
        printk("assocresp frame length[%d] is larger than skb length[%d] \n", assocresp_length, length);
        return 0;
    }

    printk("fill assocresp length(%d) ie_len(%d)\n", assocresp_length, assocresp_ie_len);

    return assocresp_length;
}

static int siwifi_fill_probe_rsp_frame(struct siwifi_vif *siwifi_vif, uint8_t *probe_rsp_frame, int length, uint8_t *node_mac)
{
    struct ieee80211_mgmt probe_rsp_mgmt;
    int probe_rsp_length = 0;
    int probe_rsp_ie_length = 0;
    uint8_t *variable = NULL;
    struct cfg80211_ap_settings *siwifi_ap_settings = NULL;
    int is_hb = siwifi_vif->siwifi_hw->mod_params->is_hb;

    /*  __le16 frame_control
        version             (bit 0 - 1 ) : 0
        type                (bit 2 - 3 ) : 0 (managemngt)
        Subtype             (bit 4 - 7 ) : 0b0101 (probe response)
        frame control flags (bit 8 - 15) : 0                        */
    probe_rsp_mgmt.frame_control = 0x50;
    probe_rsp_length += 2;
    /*  __le16 duration
        duration            (bit 0 - 15) : 0                        */
    probe_rsp_mgmt.duration = 0;
    probe_rsp_length += 2;

    ether_addr_copy(probe_rsp_mgmt.da, node_mac);
    ether_addr_copy(probe_rsp_mgmt.sa, siwifi_vif->ndev->dev_addr);
    ether_addr_copy(probe_rsp_mgmt.bssid, siwifi_vif->ndev->dev_addr);
    probe_rsp_length += 18;

    /*  __le16 seq_ctrl
        frag number         (bit 0 - 3 ) : 0
        seq number          (bit 4 - 15) : 0
     */
    probe_rsp_mgmt.seq_ctrl = 0x00;
    probe_rsp_length += 2;

    /*  __le64 timestamp;
        __le16 beacon_int;
     */
    probe_rsp_mgmt.u.probe_resp.timestamp = 0;
    // 100  Time Units (102 Milliseconds, and 400 Microseconds)
    probe_rsp_mgmt.u.probe_resp.beacon_int = 100;

    /*  __le16 capab_info;
        capability info     (bit 0 - 15) : 0x0521
                    0000 0101 0010 0001
            bit 15: 0... .... .... .... Immediate Block Ack Not Allowed
            bit 14:  0.. .... .... .... Delayed Block Ack Not Allowed
            bit 13:   0. .... .... .... DSSS-OFDM is Not Allowed
            bit 12:    0 .... .... .... No Radio Measurement
            bit 11:      0... .... .... APSD is not supported
            bit 10:       0.. .... .... G Mode Short Slot Time [20 microseconds] TODO
            bit  9:        0. .... .... QoS is Not Supported
            bit  8:         0 .... .... Spectrum Mgmt Disabled TODO
            bit  7:           0... .... Channel Agility Not Used
            bit  6:            0.. .... PBCC Not Allowed
            bit  5:             1. .... Short Preamble
            bit  4:              0 .... Privacy Disabled
            bit  3:                0... CF Poll Not Requested
            bit  2:                 0.. CF Not Pollable
            bit  1:                  0. Not an IBSS Type Network
            bit  0:                   1 ESS Type Network
     */
    probe_rsp_mgmt.u.probe_resp.capab_info = 0x21;
    probe_rsp_length += 12;

    memcpy(probe_rsp_frame, &probe_rsp_mgmt, probe_rsp_length);
    /*  u8 variable[0];
        ie_info
     */
    variable = probe_rsp_frame + probe_rsp_length;

    siwifi_ap_settings = siwifi_vif->ap_settings;
    // Elemment ID : 0 - SSID
    variable[probe_rsp_ie_length] = WLAN_EID_SSID;
    // SSID length
    variable[probe_rsp_ie_length+1] = siwifi_ap_settings->ssid_len;
    memcpy((variable + probe_rsp_ie_length + 2), siwifi_ap_settings->ssid, siwifi_ap_settings->ssid_len);
    probe_rsp_ie_length += (2 + siwifi_ap_settings->ssid_len);

    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_SUPP_RATES, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_dsps(siwifi_vif, variable + probe_rsp_ie_length);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_COUNTRY, is_hb);
    if (is_hb)
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_QBSS_LOAD, is_hb);
    if (is_hb)
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_ERP_INFO, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_custom_supp_reg(variable + probe_rsp_ie_length, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_HT_CAPABILITY, is_hb);
    probe_rsp_ie_length += siwifi_fill_ie_ht_operation(siwifi_vif, variable + probe_rsp_ie_length);
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_EXT_CAPABILITY, is_hb);
    if (is_hb) {
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VHT_CAPABILITY, is_hb);
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VHT_OPERATION, is_hb);
        probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VHT_TX_POWER_ENVELOPE, is_hb);
    }
    probe_rsp_ie_length += siwifi_fill_ie_default(variable + probe_rsp_ie_length, WLAN_EID_VENDOR_SPECIFIC, is_hb);

    probe_rsp_length += probe_rsp_ie_length;
    if (probe_rsp_length > length) {
        printk("proberesp frame length[%d] is larger than skb length[%d] \n", probe_rsp_length, length);
        return 0;
    }

    printk("fill proberesp length(%d) ie_len(%d)\n", probe_rsp_length, probe_rsp_ie_length);

    return probe_rsp_length;
}

struct sk_buff *siwifi_create_beacon(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t beacon_frame[SIWIFI_BEACON_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_BEACON_SKB_LENGTH);

    fill_length = siwifi_fill_beacon_frame(siwifi_vif, beacon_frame, SIWIFI_BEACON_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, beacon_frame, fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_auth_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t auth_frame[SIWIFI_AUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_AUTH_SKB_LENGTH);

    fill_length = siwifi_fill_auth_req_frame(siwifi_vif, auth_frame, SIWIFI_AUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, auth_frame, fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_auth_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t auth_frame[SIWIFI_AUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_AUTH_SKB_LENGTH);

    fill_length = siwifi_fill_auth_rsp_frame(siwifi_vif, auth_frame, SIWIFI_AUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, auth_frame, fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_deauth(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t deauth_frame[SIWIFI_DEAUTH_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_DEAUTH_SKB_LENGTH);

    fill_length = siwifi_fill_deauth_frame(siwifi_vif, deauth_frame, SIWIFI_DEAUTH_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, deauth_frame, fill_length);

    printk("siwifi_create_deauth success fill_length(%d) \n", fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_assoc_req(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t assoc_frame[SIWIFI_ASSOC_REQ_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_ASSOC_REQ_SKB_LENGTH);

    fill_length = siwifi_fill_assoc_req_frame(siwifi_vif, assoc_frame, SIWIFI_ASSOC_REQ_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, assoc_frame, fill_length);

    printk("siwifi_create_assoc_req success fill_length(%d) \n", fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_assoc_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t assoc_frame[SIWIFI_ASSOC_RSP_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_ASSOC_RSP_SKB_LENGTH);

    fill_length = siwifi_fill_assoc_rsp_frame(siwifi_vif, assoc_frame, SIWIFI_ASSOC_RSP_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, assoc_frame, fill_length);

    printk("siwifi_create_assoc_rsp success fill_length(%d) \n", fill_length);

    return ret_skb;
}

struct sk_buff *siwifi_create_probe_rsp(struct siwifi_vif *siwifi_vif, uint8_t *add_mac)
{
    int fill_length = 0;
    uint8_t probe_rsp_frame[SIWIFI_PROBE_RSP_FRAME_LENGTH] = { 0 };
    struct sk_buff *ret_skb = dev_alloc_skb(SIWIFI_PROBE_RSP_SKB_LENGTH);

    fill_length = siwifi_fill_probe_rsp_frame(siwifi_vif, probe_rsp_frame, SIWIFI_PROBE_RSP_FRAME_LENGTH, add_mac);
    skb_put(ret_skb, fill_length);
    memcpy(ret_skb->data, probe_rsp_frame, fill_length);

    printk("siwifi_create_probe_rsp success fill_length(%d) \n", fill_length);

    return ret_skb;
}

int siwifi_frame_send_mgmt(struct siwifi_vif *vif, struct sk_buff *skb)
{
    struct cfg80211_mgmt_tx_params params;
    u64 cookie;
    int ret = 0;

    params.len = skb->len;
    params.buf = skb->data;
    ret = siwifi_start_mgmt_xmit(vif, NULL, &params, false, &cookie);
    return ret;
}
