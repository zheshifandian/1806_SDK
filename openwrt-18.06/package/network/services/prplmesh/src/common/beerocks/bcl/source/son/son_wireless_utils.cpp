/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_utils.h>
#include <bcl/son/son_wireless_utils.h>

#include <tlvf/wfa_map/tlvChannelPreference.h>

#include <easylogging++.h>

#include <cmath>

using namespace son;

//Based on hostapd global_op_class struct, file ieee802_11_common.c
// clang-format off
const std::map<uint8_t, wireless_utils::sOperatingClass> wireless_utils::operating_classes_list = {
//  {OP Class   {Channels List,                                                Bandwidth             }}
    {81,        {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},                  beerocks::BANDWIDTH_20}},
    {82,        {{14},                                                         beerocks::BANDWIDTH_20}},
    {83,        {{1, 2, 3, 4, 5, 6, 7, 8, 9},                                  beerocks::BANDWIDTH_40}},
    {84,        {{5, 6, 7, 8, 9, 10, 11, 12, 13},                              beerocks::BANDWIDTH_40}},
    {115,       {{36, 40, 44, 48},                                             beerocks::BANDWIDTH_20}},
    {116,       {{36, 44},                                                     beerocks::BANDWIDTH_40}},
    {117,       {{40, 48},                                                     beerocks::BANDWIDTH_40}},
    {118,       {{52, 56, 60, 64},                                             beerocks::BANDWIDTH_20}},
    {119,       {{52, 60},                                                     beerocks::BANDWIDTH_40}},
    {120,       {{56, 64},                                                     beerocks::BANDWIDTH_40}},
    {121,       {{100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144}, beerocks::BANDWIDTH_20}},
    {122,       {{100, 108, 116, 124, 132, 136, 140},                          beerocks::BANDWIDTH_40}},
    {123,       {{104, 112, 120, 128, 134, 136, 138, 144},                     beerocks::BANDWIDTH_40}},
    {124,       {{149, 153, 157, 161},                                         beerocks::BANDWIDTH_20}},
    {125,       {{149, 153, 157, 161, 165, 169},                               beerocks::BANDWIDTH_20}},
    {126,       {{149, 157},                                                   beerocks::BANDWIDTH_40}},
    {127,       {{153, 161},                                                   beerocks::BANDWIDTH_40}},
//  {OP Class   {Channel center Frequency index},                              Bandwidth              }}
    {128,       {{42, 58, 106, 122, 138, 155},                                 beerocks::BANDWIDTH_80}},
    {129,       {{50, 114},                                                    beerocks::BANDWIDTH_160}},
    {130,       {{42, 58, 106, 122, 138, 155},                                 beerocks::BANDWIDTH_80_80}}
};

const std::map<uint8_t, std::map<beerocks::eWiFiBandwidth, wireless_utils::sChannel>> wireless_utils::channels_table_5g =
{
    /*
    Example:
    { Channel,  {
                   { Bandwidth,               { Center Channel, { Overlap Beacon Channels Range } } },
                }
    }
                                                                 min, max
    */
    { 36,       {
                   { beerocks::BANDWIDTH_20,  { 36,             { 36, 36                        } } },
                   { beerocks::BANDWIDTH_40,  { 38,             { 36, 40                        } } },
                   { beerocks::BANDWIDTH_80,  { 42,             { 36, 48                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 40,       {
                   { beerocks::BANDWIDTH_20,  { 40,             { 40, 40                        } } },
                   { beerocks::BANDWIDTH_40,  { 38,             { 36, 40                        } } },
                   { beerocks::BANDWIDTH_80,  { 42,             { 36, 48                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 44,       {
                   { beerocks::BANDWIDTH_20,  { 44,             { 44, 44                        } } },
                   { beerocks::BANDWIDTH_40,  { 46,             { 44, 48                        } } },
                   { beerocks::BANDWIDTH_80,  { 42,             { 36, 48                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 48,       {
                   { beerocks::BANDWIDTH_20,  { 48,             { 48, 48                        } } },
                   { beerocks::BANDWIDTH_40,  { 46,             { 44, 48                        } } },
                   { beerocks::BANDWIDTH_80,  { 42,             { 36, 48                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 52,       {
                   { beerocks::BANDWIDTH_20,  { 52,             { 52, 52                        } } },
                   { beerocks::BANDWIDTH_40,  { 54,             { 52, 56                        } } },
                   { beerocks::BANDWIDTH_80,  { 58,             { 52, 64                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 56,       {
                   { beerocks::BANDWIDTH_20,  { 56,             { 56, 56                        } } },
                   { beerocks::BANDWIDTH_40,  { 54,             { 52, 56                        } } },
                   { beerocks::BANDWIDTH_80,  { 58,             { 52, 64                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 60,       {
                   { beerocks::BANDWIDTH_20,  { 60,             { 60, 60                        } } },
                   { beerocks::BANDWIDTH_40,  { 62,             { 60, 64                        } } },
                   { beerocks::BANDWIDTH_80,  { 58,             { 52, 64                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 64,       {
                   { beerocks::BANDWIDTH_20,  { 64,             { 64, 64                        } } },
                   { beerocks::BANDWIDTH_40,  { 62,             { 60, 64                        } } },
                   { beerocks::BANDWIDTH_80,  { 58,             { 52, 64                        } } },
                   { beerocks::BANDWIDTH_160, { 50,             { 36, 64                        } } },
                }
    },
    { 100,      {
                   { beerocks::BANDWIDTH_20,  { 100,            { 100, 100                      } } },
                   { beerocks::BANDWIDTH_40,  { 102,            { 100, 104                      } } },
                   { beerocks::BANDWIDTH_80,  { 106,            { 100, 112                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 104,      {
                   { beerocks::BANDWIDTH_20,  { 104,            { 104, 104                      } } },
                   { beerocks::BANDWIDTH_40,  { 102,            { 100, 104                      } } },
                   { beerocks::BANDWIDTH_80,  { 106,            { 100, 112                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 108,      {
                   { beerocks::BANDWIDTH_20,  { 108,            { 108, 108                      } } },
                   { beerocks::BANDWIDTH_40,  { 110,            { 108, 112                      } } },
                   { beerocks::BANDWIDTH_80,  { 106,            { 100, 112                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 112,      {
                   { beerocks::BANDWIDTH_20,  { 112,            { 112, 112                      } } },
                   { beerocks::BANDWIDTH_40,  { 110,            { 108, 112                      } } },
                   { beerocks::BANDWIDTH_80,  { 106,            { 100, 112                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 116,      {
                   { beerocks::BANDWIDTH_20,  { 116,            { 116, 116                      } } },
                   { beerocks::BANDWIDTH_40,  { 118,            { 116, 120                      } } },
                   { beerocks::BANDWIDTH_80,  { 122,            { 116, 128                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 120,      {
                   { beerocks::BANDWIDTH_20,  { 120,            { 120, 120                      } } },
                   { beerocks::BANDWIDTH_40,  { 118,            { 116, 120                      } } },
                   { beerocks::BANDWIDTH_80,  { 122,            { 116, 128                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 124,      {
                   { beerocks::BANDWIDTH_20,  { 124,            { 124, 124                      } } },
                   { beerocks::BANDWIDTH_40,  { 126,            { 124, 128                      } } },
                   { beerocks::BANDWIDTH_80,  { 122,            { 116, 128                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 128,      {
                   { beerocks::BANDWIDTH_20,  { 128,            { 128, 128                      } } },
                   { beerocks::BANDWIDTH_40,  { 126,            { 124, 128                      } } },
                   { beerocks::BANDWIDTH_80,  { 122,            { 116, 128                      } } },
                   { beerocks::BANDWIDTH_160, { 114,            { 100, 128                      } } },
                }
    },
    { 132,      {
                   { beerocks::BANDWIDTH_20,  { 132,            { 132, 132                      } } },
                   { beerocks::BANDWIDTH_40,  { 134,            { 132, 136                      } } },
                   { beerocks::BANDWIDTH_80,  { 138,            { 132, 144                      } } },
                }
    },
    { 136,      {
                   { beerocks::BANDWIDTH_20,  { 136,            { 136, 136                      } } },
                   { beerocks::BANDWIDTH_40,  { 134,            { 132, 136                      } } },
                   { beerocks::BANDWIDTH_80,  { 138,            { 132, 144                      } } },
                }
    },
    { 140,      {
                   { beerocks::BANDWIDTH_20,  { 140,            { 140, 140                      } } },
                   { beerocks::BANDWIDTH_40,  { 142,            { 140, 144                      } } },
                   { beerocks::BANDWIDTH_80,  { 138,            { 132, 144                      } } },
                }
    },
    { 144,      {
                   { beerocks::BANDWIDTH_20,  { 144,            { 144, 144                      } } },
                   { beerocks::BANDWIDTH_40,  { 142,            { 140, 144                      } } },
                   { beerocks::BANDWIDTH_80,  { 138,            { 132, 144                      } } },
                }
    },
    { 149,      {
                   { beerocks::BANDWIDTH_20,  { 149,            { 149, 149                      } } },
                   { beerocks::BANDWIDTH_40,  { 151,            { 149, 153                      } } },
                   { beerocks::BANDWIDTH_80,  { 155,            { 149, 161                      } } },
                }
    },
    { 153,      {
                   { beerocks::BANDWIDTH_20,  { 153,            { 153, 153                      } } },
                   { beerocks::BANDWIDTH_40,  { 151,            { 149, 153                      } } },
                   { beerocks::BANDWIDTH_80,  { 155,            { 149, 161                      } } },
                }
    },
    { 157,      {
                   { beerocks::BANDWIDTH_20,  { 157,            { 157, 157                      } } },
                   { beerocks::BANDWIDTH_40,  { 159,            { 157, 161                      } } },
                   { beerocks::BANDWIDTH_80,  { 155,            { 149, 161                      } } },
                }
    },
    { 161,      {
                   { beerocks::BANDWIDTH_20,  { 161,            { 161, 161                      } } },
                   { beerocks::BANDWIDTH_40,  { 159,            { 157, 161                      } } },
                   { beerocks::BANDWIDTH_80,  { 155,            { 149, 161                      } } },
                }
    },
    { 165,      {
                   { beerocks::BANDWIDTH_20,  { 165,            { 165, 165                      } } },
                }
    },
    { 169,      {
                   { beerocks::BANDWIDTH_20,  { 169,            { 169, 169                      } } },
                }
    }




};

const wireless_utils::sPhyRateTableEntry wireless_utils::phy_rate_table[PHY_RATE_TABLE_ANT_MODE_MAX][PHY_RATE_TABLE_MCS_MAX] = {
    // 1X1_SS1_table
    {//MCS 0-9:{TX_power 2.4/5 ,{{20_rate_long/short,20_RSSI},{40_rate_long/short,40_RSSI},{80_rate_long/short,80_RSSI},{160_rate_long/short,160_RSSI}}}
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {65,   72,   -892}}, {beerocks::BANDWIDTH_40,  {135,   150,   -860}},
        {beerocks::BANDWIDTH_80,  {293,  325,  -824}}, {beerocks::BANDWIDTH_160, {585,   650,   -794}},
    }},
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {130,  144,  -865}}, {beerocks::BANDWIDTH_40,  {270,   300,   -841}},
        {beerocks::BANDWIDTH_80,  {585,  650,  -805}}, {beerocks::BANDWIDTH_160, {1170,  1300,  -775}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {195,  217,  -808}}, {beerocks::BANDWIDTH_40,  {405,   450,   -787}},
        {beerocks::BANDWIDTH_80,  {878,  975,  -756}}, {beerocks::BANDWIDTH_160, {1175,  1950,  -726}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {260,  289,  -811}}, {beerocks::BANDWIDTH_40,  {540,   600,   -790}},
        {beerocks::BANDWIDTH_80,  {1170, 1300, -753}}, {beerocks::BANDWIDTH_160, {2340,  2600,  -723}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {390,  433,  -755}}, {beerocks::BANDWIDTH_40,  {810,   900,   -727}},
        {beerocks::BANDWIDTH_80,  {1755, 1950, -698}}, {beerocks::BANDWIDTH_160, {3510,  3900,  -668}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {520,  578,  -717}}, {beerocks::BANDWIDTH_40,  {1080,  1200,  -686}},
        {beerocks::BANDWIDTH_80,  {2340, 2600, -656}}, {beerocks::BANDWIDTH_160, {4680,  5200,  -626}},
    }},
    {17, 14,    {
        {beerocks::BANDWIDTH_20,  {585,  650,  -698}}, {beerocks::BANDWIDTH_40,  {1215,  1350,  -672}},
        {beerocks::BANDWIDTH_80,  {2633, 2925, -641}}, {beerocks::BANDWIDTH_160, {5265,  5850,  -611}},
    }},
    {16, 13,    {
        {beerocks::BANDWIDTH_20,  {650,  722,  -673}}, {beerocks::BANDWIDTH_40,  {1350,  1500,  -647}},
        {beerocks::BANDWIDTH_80,  {2925, 3250, -632}}, {beerocks::BANDWIDTH_160, {5850,  6500,  -602}},
    }},
    {0,  10,    {
        {beerocks::BANDWIDTH_20,  {780,  867,  -644}}, {beerocks::BANDWIDTH_40,  {1620,  1800,  -619}},
        {beerocks::BANDWIDTH_80,  {3510, 3900, -587}}, {beerocks::BANDWIDTH_160, {7020,  7800,  -557}},
    }},
    {0,  8,     {
        {beerocks::BANDWIDTH_20,  {0,    0,    0   }}, {beerocks::BANDWIDTH_40,  {1800,  2000,  -599}},
        {beerocks::BANDWIDTH_80,  {3900, 4333, -558}}, {beerocks::BANDWIDTH_160, {7800,  8667,  -528}},
    }},
    },
    // 2X2_SS1_table
    {//MCS 0-9:{TX_power,{{20_rate_long/short,20_RSSI},{40_rate_long/short,40_RSSI},{80_rate_long/short,80_RSSI},{160_rate_long/short,160_RSSI}}}
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {65,   72,   -912}}, {beerocks::BANDWIDTH_40,  {135,   150,   -894}},
        {beerocks::BANDWIDTH_80,  {293,  325,  -855}}, {beerocks::BANDWIDTH_160, {585,   650,   -825}},
    }},
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {130,  144,  -900}}, {beerocks::BANDWIDTH_40,  {270,   300,   -879}},
        {beerocks::BANDWIDTH_80,  {585,  650,  -840}}, {beerocks::BANDWIDTH_160, {1170,  1300,  -810}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {195,  217,  -869}}, {beerocks::BANDWIDTH_40,  {405,   450,   -843}},
        {beerocks::BANDWIDTH_80,  {878,  975,  -805}}, {beerocks::BANDWIDTH_160, {1175,  1950,  -775}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {260,  289,  -858}}, {beerocks::BANDWIDTH_40,  {540,   600,   -831}},
        {beerocks::BANDWIDTH_80,  {1170, 1300, -792}}, {beerocks::BANDWIDTH_160, {2340,  2600,  -762}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {390,  433,  -810}}, {beerocks::BANDWIDTH_40,  {810,   900,   -785}},
        {beerocks::BANDWIDTH_80,  {1755, 1950, -748}}, {beerocks::BANDWIDTH_160, {3510,  3900,  -718}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {520,  578,  -773}}, {beerocks::BANDWIDTH_40,  {1080,  1200,  -742}},
        {beerocks::BANDWIDTH_80,  {2340, 2600, -704}}, {beerocks::BANDWIDTH_160, {4680,  5200,  -674}},
    }},
    {17, 14,    {
        {beerocks::BANDWIDTH_20,  {585,  650,  -756}}, {beerocks::BANDWIDTH_40,  {1215,  1350,  -729}},
        {beerocks::BANDWIDTH_80,  {2633, 2925, -692}}, {beerocks::BANDWIDTH_160, {5265,  5850,  -662}},
    }},
    {16, 13,    {
        {beerocks::BANDWIDTH_20,  {650,  722,  -735}}, {beerocks::BANDWIDTH_40,  {1350,  1500,  -709}},
        {beerocks::BANDWIDTH_80,  {2925, 3250, -684}}, {beerocks::BANDWIDTH_160, {5850,  6500,  -654}},
    }},
    {0,  10,    {
        {beerocks::BANDWIDTH_20,  {780,  867,  -699}}, {beerocks::BANDWIDTH_40,  {1620,  1800,  -674}},
        {beerocks::BANDWIDTH_80,  {3510, 3900, -637}}, {beerocks::BANDWIDTH_160, {7020,  7800,  -607}},
    }},
    {0,  8,     {
        {beerocks::BANDWIDTH_20,  {0,    0,    0   }}, {beerocks::BANDWIDTH_40,  {1800,  2000,  -659}},
        {beerocks::BANDWIDTH_80,  {3900, 4333, -614}}, {beerocks::BANDWIDTH_160, {7800,  8667,  -584}},
    }},
    },
    // 2X2_SS2_table
    {//MCS 0-9:{TX_power,{{20_rate_long/short,20_RSSI},{40_rate_long/short,40_RSSI},{80_rate_long/short,80_RSSI},{160_rate_long/short,160_RSSI}}}
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {130,  144,  -890}}, {beerocks::BANDWIDTH_40,  {270,   300,   -861}},
        {beerocks::BANDWIDTH_80,  {585,  650,  -834}}, {beerocks::BANDWIDTH_160, {1170,  1300,  -804}},
    }},
    {18, 16,    {
        {beerocks::BANDWIDTH_20,  {260,  288,  -855}}, {beerocks::BANDWIDTH_40,  {540,   600,   -828}},
        {beerocks::BANDWIDTH_80,  {1170, 1300, -795}}, {beerocks::BANDWIDTH_160, {2340,  2600,  -765}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {390,  434,  -815}}, {beerocks::BANDWIDTH_40,  {810,   900,   -784}},
        {beerocks::BANDWIDTH_80,  {1755, 1950, -756}}, {beerocks::BANDWIDTH_160, {3510,  3900,  -726}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {520,  578,  -774}}, {beerocks::BANDWIDTH_40,  {1080,  1200,  -755}},
        {beerocks::BANDWIDTH_80,  {2340, 2600, -718}}, {beerocks::BANDWIDTH_160, {4680,  5200,  -688}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {780,  866,  -720}}, {beerocks::BANDWIDTH_40,  {1620,  1800,  -699}},
        {beerocks::BANDWIDTH_80,  {3510, 3900, -669}}, {beerocks::BANDWIDTH_160, {7020,  7800,  -639}},
    }},
    {17, 15,    {
        {beerocks::BANDWIDTH_20,  {1040, 1156, -657}}, {beerocks::BANDWIDTH_40,  {2160,  2400,  -640}},
        {beerocks::BANDWIDTH_80,  {4680, 5200, -604}}, {beerocks::BANDWIDTH_160, {9360,  10400, -574}},
    }},
    {17, 14,    {
        {beerocks::BANDWIDTH_20,  {1170, 1300, -648}}, {beerocks::BANDWIDTH_40,  {2430,  2700,  -628}},
        {beerocks::BANDWIDTH_80,  {5265, 5850, -594}}, {beerocks::BANDWIDTH_160, {10530, 11700, -564}},
    }},
    {16, 13,    {
        {beerocks::BANDWIDTH_20,  {1300, 1444, -634}}, {beerocks::BANDWIDTH_40,  {2700,  3000,  -617}},
        {beerocks::BANDWIDTH_80,  {5850, 6500, -574}}, {beerocks::BANDWIDTH_160, {11700, 13000, -544}},
    }},
    {0,  10,    {
        {beerocks::BANDWIDTH_20,  {1560, 1733, -581}}, {beerocks::BANDWIDTH_40,  {3240,  3600,  -584}},
        {beerocks::BANDWIDTH_80,  {7020, 7800, -525}}, {beerocks::BANDWIDTH_160, {14040, 15600, -495}},
    }},
    {0,  8,     {
        {beerocks::BANDWIDTH_20,  {0,    0,    0   }}, {beerocks::BANDWIDTH_40,  {3600,  4000,  -556}},
        {beerocks::BANDWIDTH_80,  {7800, 8666, -497}}, {beerocks::BANDWIDTH_160, {15600, 17333, -467}},
    }},
    },
};
// clang-format on

/**
 * @brief According to 802.11-2016 convertion table (Table 9-154), calculation equation parameters.
 */
constexpr int RCPI_EQUATION_COEF     = 2;
constexpr int RCPI_EQUATION_CONSTANT = 110;

constexpr beerocks::eWiFiAntNum
    wireless_utils::phy_rate_table_mode_to_ant_num[PHY_RATE_TABLE_ANT_MODE_MAX];
constexpr beerocks::eWiFiSS wireless_utils::phy_rate_table_mode_to_ss[PHY_RATE_TABLE_ANT_MODE_MAX];
constexpr wireless_utils::sPhyRateBitRateEntry
    wireless_utils::bit_rate_max_table_mbps[BIT_RATE_MAX_TABLE_SIZE];

bool wireless_utils::has_operating_class_channel(const sOperatingClass &oper_class, uint8_t channel,
                                                 beerocks::eWiFiBandwidth bw)
{
    if (oper_class.band != bw) {
        return false;
    }
    auto it = oper_class.channels.find(channel);
    if (it != oper_class.channels.end()) {
        return true;
    }

    // operating classes 128,129,130 use center channel **unlike the other classes**,
    // so convert channel and bandwidth to center channel.
    // For more info, refer to Table E-4 in the 802.11 specification.
    if (channel < 36) {
        return false;
    }
    auto center_channel = wireless_utils::get_5g_center_channel(channel, bw);
    if (center_channel == 0) {
        return false;
    }
    it = oper_class.channels.find(center_channel);
    if (it == oper_class.channels.end()) {
        return false;
    }
    return true;
}

wireless_utils::sPhyUlParams
wireless_utils::estimate_ul_params(int ul_rssi, uint16_t sta_phy_tx_rate_100kb,
                                   const beerocks::message::sRadioCapabilities *sta_capabilities,
                                   beerocks::eWiFiBandwidth ap_bw, bool is_5ghz)
{
    int ul_rssi_lut = ul_rssi * 10;
    int estimated_ul_rssi_lut;
    int estimated_ul_rssi_lut_delta;
    int estimated_ul_rssi_lut_delta_min = 120 * 10;
    sPhyUlParams estimation = {0, beerocks::RSSI_INVALID, ESTIMATION_FAILURE_INVALID_RSSI};

    const int max_ant_mode = (sta_capabilities->ant_num == beerocks::ANT_1X1)
                                 ? beerocks::ANT_MODE_1X1_SS1
                                 : beerocks::ANT_MODE_2X2_SS2;
    const int max_mcs = (is_5ghz && (sta_capabilities->wifi_standard & int(beerocks::STANDARD_AC)))
                            ? sta_capabilities->vht_mcs
                            : sta_capabilities->ht_mcs;
    uint8_t max_bw = (is_5ghz && (sta_capabilities->wifi_standard & int(beerocks::STANDARD_AC)))
                         ? sta_capabilities->vht_bw
                         : sta_capabilities->ht_bw;
    if ((ap_bw < max_bw) || (max_bw == beerocks::BANDWIDTH_UNKNOWN)) {
        max_bw = ap_bw;
    }

    //enums beyond 80 (80P80, 160) are assumed 160
    if (max_bw > beerocks::BANDWIDTH_80) {
        max_bw = beerocks::BANDWIDTH_160;
    }

    LOG(DEBUG) << "UL RSSI:" << ul_rssi << " | sta_phy_tx_rate:" << sta_phy_tx_rate_100kb / 10
               << " Mbps | AP BW:"
               << beerocks::utils::convert_bandwidth_to_int((beerocks::eWiFiBandwidth)ap_bw)
               << " | is_5ghz:" << is_5ghz << " | ant_num:" << int(sta_capabilities->ant_num)
               << " | max_ant_mode:" << max_ant_mode << " | max MCS:" << max_mcs << " | max BW:"
               << beerocks::utils::convert_bandwidth_to_int((beerocks::eWiFiBandwidth)max_bw);

    if (ul_rssi == beerocks::RSSI_INVALID) {
        LOG(DEBUG) << "Can not estimate UL parameters (invalid RSSI)";
        return estimation;
    }

    // If station phyrate value is below table's minimum, return minimal estimation
    if (sta_phy_tx_rate_100kb < phy_rate_table[0][0].bw_values.begin()->second.gi_long_rate) {
        LOG(DEBUG) << "Can not estimate UL parameters (STA phyrate is too low)";
        estimation.tx_power =
            is_5ghz ? phy_rate_table[0][0].tx_power_5 : phy_rate_table[0][0].tx_power_2_4;
        estimation.rssi   = int(ceil(phy_rate_table[0][0].bw_values.begin()->second.rssi / 10.0));
        estimation.status = ESTIMATION_FAILURE_BELOW_RANGE;

        return estimation;
    }

    // If station phyrate value is above table's maximum, return maximal estimation
    auto const &bw_values_limit = phy_rate_table[max_ant_mode][max_mcs].bw_values;
    if (bw_values_limit.find(max_bw) == bw_values_limit.end()) {
        LOG(ERROR) << "Can not estimate UL parameters: unsupported bw " << max_bw;
        return estimation;
    }
    if (sta_phy_tx_rate_100kb >
        phy_rate_table[max_ant_mode][max_mcs].bw_values.at(max_bw).gi_short_rate) {
        LOG(DEBUG) << "STA phy rate (" << sta_phy_tx_rate_100kb / 10
                   << " Mbps) is above maximum possible in current MCS/NSS/BW mode ("
                   << phy_rate_table[max_ant_mode][max_mcs].bw_values.at(max_bw).gi_short_rate / 10
                   << " Mbps)";

        estimation.status   = ESTIMATION_SUCCESS;
        estimation.tx_power = is_5ghz ? phy_rate_table[max_ant_mode][max_mcs].tx_power_5
                                      : phy_rate_table[max_ant_mode][max_mcs].tx_power_2_4;
        estimation.rssi =
            int(ceil(phy_rate_table[max_ant_mode][max_mcs].bw_values.at(max_bw).rssi / 10.0));

        LOG(DEBUG) << "Return maximal estimation values | tx_power:" << estimation.tx_power
                   << " | RSSI:" << estimation.rssi;

        return estimation;
    }

    for (int ant_mode = max_ant_mode; ant_mode > -1; ant_mode--) {   // filter by ant_mode
        for (auto bw = max_bw; bw >= beerocks::BANDWIDTH_20; bw--) { // filter by max_bw
            // skip un-handled intermediate bw values
            auto const &bw_values_max = phy_rate_table[ant_mode][max_mcs].bw_values;
            if (bw_values_max.find(bw) == bw_values_max.end()) {
                continue;
            }
            for (int mcs = max_mcs; mcs > -1; mcs--) { // filter by mcs

                estimated_ul_rssi_lut       = phy_rate_table[ant_mode][mcs].bw_values.at(bw).rssi;
                estimated_ul_rssi_lut_delta = std::abs(ul_rssi_lut - estimated_ul_rssi_lut);

                auto gi_long_rate  = phy_rate_table[ant_mode][mcs].bw_values.at(bw).gi_long_rate;
                auto gi_short_rate = phy_rate_table[ant_mode][mcs].bw_values.at(bw).gi_short_rate;

                // Check if the current rate is between the gi_long_rate to the gi_short_rate
                if ((sta_phy_tx_rate_100kb >= gi_long_rate) &&
                    (sta_phy_tx_rate_100kb <= gi_short_rate)) {
                    // phyrate is in range - use table values
                    if (estimated_ul_rssi_lut_delta <= estimated_ul_rssi_lut_delta_min) {
                        estimated_ul_rssi_lut_delta_min = estimated_ul_rssi_lut_delta;
                        estimation.tx_power = is_5ghz ? phy_rate_table[ant_mode][mcs].tx_power_5
                                                      : phy_rate_table[ant_mode][mcs].tx_power_2_4;
                        estimation.rssi = int(ceil(estimated_ul_rssi_lut / 10.0));
                    }
                    continue;
                }

                // phyrate is not in range - use average rssi delta

                // Since we use the avarage, and using the [mcs -1], continue in case of mcs=0
                // to prevent segfault.
                if (mcs == 0) {
                    continue;
                }

                // Skip if the current rate is not between current gi_long_rate to the
                // (mcs -1) gi_short_rate.
                if (!((sta_phy_tx_rate_100kb <= gi_long_rate) &&
                      (sta_phy_tx_rate_100kb >=
                       phy_rate_table[ant_mode][mcs - 1].bw_values.at(bw).gi_short_rate))) {
                    continue;
                }

                // update rssi estimation and delta
                estimated_ul_rssi_lut = (phy_rate_table[ant_mode][mcs].bw_values.at(bw).rssi +
                                         phy_rate_table[ant_mode][mcs - 1].bw_values.at(bw).rssi) /
                                        2;

                estimated_ul_rssi_lut_delta = std::abs(ul_rssi_lut - estimated_ul_rssi_lut);

                if (estimated_ul_rssi_lut_delta <= estimated_ul_rssi_lut_delta_min) {
                    estimated_ul_rssi_lut_delta_min = estimated_ul_rssi_lut_delta;

                    estimation.tx_power = is_5ghz ? phy_rate_table[ant_mode][mcs].tx_power_5
                                                  : phy_rate_table[ant_mode][mcs].tx_power_2_4;

                    estimation.rssi = int(ceil(estimated_ul_rssi_lut / 10.0));
                }
            }
        }
    }

    estimation.status = ESTIMATION_SUCCESS;

    LOG(DEBUG) << "Successful estimation | tx_power:" << estimation.tx_power
               << " | RSSI:" << estimation.rssi;

    return estimation;
}

int wireless_utils::estimate_dl_rssi(int ul_rssi, int tx_power, const sPhyApParams &ap_params)
{
    int eirp_sta   = tx_power;
    int eirp_ap    = ap_params.ant_gain + ap_params.tx_power;
    int ant_factor = ANT_FACTOR_2X2;
    float pathloss;
    int dl_rssi;

    if (ap_params.ant_num == beerocks::ANT_4X4) {
        ant_factor = ANT_FACTOR_4X4;
    } else if (ap_params.ant_num == beerocks::ANT_3X3) {
        ant_factor = ANT_FACTOR_3X3;
    }

    pathloss = eirp_sta - (ul_rssi - ant_factor - ap_params.ant_gain);

    if (ap_params.is_5ghz) {
        pathloss += NOISE_FIGURE; // 5GHz extra loss
    }

    dl_rssi = eirp_ap - pathloss;

    LOG(DEBUG) << " eirp_sta:" << eirp_sta << " | UL RSSI:" << int(ul_rssi)
               << " | ant_factor:" << ant_factor << " | ant_gain:" << ap_params.ant_gain
               << " | eirp_ap:" << eirp_ap << " | pathloss:" << int(pathloss)
               << " | Returns estimated DL RSSI:" << int(dl_rssi);

    return dl_rssi;
}

double wireless_utils::estimate_ap_tx_phy_rate(
    int estimated_dl_rssi, const beerocks::message::sRadioCapabilities *sta_capabilities,
    beerocks::eWiFiBandwidth ap_bw, bool is_5ghz)
{
    int estimated_dl_rssi_lut = estimated_dl_rssi * 10;
    int dl_rssi_lut;
    double estimated_phy_rate = 0;

    int max_ant_mode = (sta_capabilities->ant_num == beerocks::ANT_1X1)
                           ? beerocks::ANT_MODE_1X1_SS1
                           : beerocks::ANT_MODE_2X2_SS2;
    int max_mcs = (is_5ghz && (sta_capabilities->wifi_standard & int(beerocks::STANDARD_AC)))
                      ? sta_capabilities->vht_mcs
                      : sta_capabilities->ht_mcs;
    uint8_t max_bw = (is_5ghz && (sta_capabilities->wifi_standard & int(beerocks::STANDARD_AC)))
                         ? sta_capabilities->vht_bw
                         : sta_capabilities->ht_bw;
    if ((ap_bw < max_bw) || (max_bw == beerocks::BANDWIDTH_UNKNOWN)) {
        max_bw = ap_bw;
    }

    // Beerocks is not supporting estimation above 160 Mhz
    // enums beyond 80 (80P80, 160) are assumed 160
    if (max_bw > beerocks::BANDWIDTH_80) {
        max_bw = beerocks::BANDWIDTH_160;
    }

    for (int ant_mode = max_ant_mode; ant_mode > -1; ant_mode--) {   // filter by ant_mode
        for (auto bw = max_bw; bw >= beerocks::BANDWIDTH_20; bw--) { // filter by max_bw
            // skip un-handled intermediate bw values
            auto const &bw_values_max = phy_rate_table[ant_mode][max_mcs].bw_values;
            if (bw_values_max.find(bw) == bw_values_max.end()) {
                continue;
            }
            for (int mcs = max_mcs; mcs > -1; mcs--) { // filter by mcs

                dl_rssi_lut = phy_rate_table[ant_mode][mcs].bw_values.at(bw).rssi;
                // same rate && min rssi_delta
                if (estimated_dl_rssi_lut >= dl_rssi_lut) {
                    estimated_phy_rate =
                        1e+5 * double(phy_rate_table[ant_mode][mcs].bw_values.at(bw).gi_short_rate);
                    break;
                }
            }
            if (estimated_phy_rate != 0)
                break;
        }
        if (estimated_phy_rate != 0)
            break;
    }

    if (estimated_phy_rate == 0) {
        estimated_phy_rate =
            1e+5 * double(phy_rate_table[0][0].bw_values.begin()->second.gi_short_rate);
    }

    LOG(DEBUG) << "estimated DL RSSI:" << int(estimated_dl_rssi)
               << " | AP BW:" << beerocks::utils::convert_bandwidth_to_int(ap_bw)
               << " | Return estimated PHY RATE:" << int(estimated_phy_rate / 1e+6) << " Mbps";

    return estimated_phy_rate;
}

double wireless_utils::get_load_max_bit_rate_mbps(double phy_rate_100kb)
{
    int size = BIT_RATE_MAX_TABLE_SIZE;
    int i;
    for (i = 0; i < size; i++) {
        if (phy_rate_100kb < bit_rate_max_table_mbps[i].phy_rate_100kb) {
            break;
        }
    }
    if ((i == 0) || (i == size)) {
        if (i == size)
            i--;
        return bit_rate_max_table_mbps[i].bit_rate_max_mbps;
    } else {
        int phy_rate_delta = bit_rate_max_table_mbps[i].phy_rate_100kb -
                             bit_rate_max_table_mbps[i - 1].phy_rate_100kb;
        int bit_rate_delta = bit_rate_max_table_mbps[i].bit_rate_max_mbps -
                             bit_rate_max_table_mbps[i - 1].bit_rate_max_mbps;
        int percentage = (phy_rate_delta == 0)
                             ? 0
                             : ((phy_rate_100kb - bit_rate_max_table_mbps[i - 1].phy_rate_100kb) /
                                phy_rate_delta);
        return (bit_rate_max_table_mbps[i - 1].bit_rate_max_mbps + bit_rate_delta * percentage);
    }
}

bool wireless_utils::get_mcs_from_rate(const uint16_t rate, const beerocks::eWiFiAntMode ant_mode,
                                       const beerocks::eWiFiBandwidth bw, uint8_t &mcs,
                                       uint8_t &short_gi)
{
    uint16_t nearest_diff = 0xFFFF;
    for (int mcs_idx = 0; mcs_idx < 8; mcs_idx++) {
        auto const &bw_values = phy_rate_table[ant_mode][mcs_idx].bw_values;
        auto iter_bw          = bw_values.find(bw);
        if (iter_bw == bw_values.end()) {
            continue;
        }

        if (iter_bw->second.gi_short_rate == rate) {
            mcs      = mcs_idx;
            short_gi = 1;
            return true;
        } else if (iter_bw->second.gi_long_rate == rate) {
            mcs      = mcs_idx;
            short_gi = 0;
            return true;
        }

        uint16_t rate_temp = iter_bw->second.gi_short_rate;
        uint16_t diff_temp = abs(rate - rate_temp);
        if (diff_temp < nearest_diff) {
            nearest_diff = diff_temp;
            mcs          = mcs_idx;
            short_gi     = 1;
        }

        rate_temp = iter_bw->second.gi_long_rate;
        diff_temp = abs(rate - rate_temp);
        if (diff_temp < nearest_diff) {
            nearest_diff = diff_temp;
            mcs          = mcs_idx;
            short_gi     = 0;
        }
    }

    LOG(DEBUG) << "rate:" << rate << " | BW:" << beerocks::utils::convert_bandwidth_to_int(bw)
               << " | ant_mode:" << ant_mode << " | Return MCS:" << mcs << " and GI:" << short_gi;

    return false;
}

int wireless_utils::channel_to_freq(int channel)
{
    if (channel == 14)
        return 2484;

    if (channel < 14)
        return (channel * 5) + 2407;

    return (channel + 1000) * 5;
}

int wireless_utils::freq_to_channel(int freq)
{
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (freq == 2484) {
        return 14;
    } else if (freq < 2484) {
        return (freq - 2407) / 5;
    } else if (freq >= 4910 && freq <= 4980) {
        return (freq - 4000) / 5;
    } else if (freq <= 45000) { /* DMG band lower limit */
        return (freq - 5000) / 5;
    } else if (freq >= 58320 && freq <= 64800) {
        return (freq - 56160) / 2160;
    } else {
        return 0;
    }
}

uint16_t wireless_utils::channel_to_vht_center_freq(int channel, beerocks::eWiFiBandwidth bandwidth,
                                                    bool channel_ext_above_secondary)
{
    int freq = channel_to_freq(channel);
    uint16_t vht_center_freq;
    switch (bandwidth) {
    case beerocks::eWiFiBandwidth::BANDWIDTH_20:
        vht_center_freq = freq;
        break;
    case beerocks::eWiFiBandwidth::BANDWIDTH_40:
        vht_center_freq = freq + (channel_ext_above_secondary ? 10 : -10);
        break;
    case beerocks::eWiFiBandwidth::BANDWIDTH_80:
    case beerocks::eWiFiBandwidth::BANDWIDTH_80_80:
        vht_center_freq = freq + (channel_ext_above_secondary ? 30 : -30);
        break;
    case beerocks::eWiFiBandwidth::BANDWIDTH_160:
        vht_center_freq = freq + (channel_ext_above_secondary ? 70 : -70);
        break;
    default:
        LOG(ERROR) << "invalid bandwidth " << bandwidth;
        return -1;
    }
    return vht_center_freq;
}

beerocks::eFreqType wireless_utils::which_freq(uint32_t chn)
{
    if ((1 <= chn) && (chn <= BAND_5G_CHANNEL_CHECK)) {
        return beerocks::eFreqType::FREQ_24G;
    }

    if (START_OF_LOW_BAND_NON_DFS <= chn) {
        return beerocks::eFreqType::FREQ_5G;
    }

    if (0 == chn) {
        LOG(DEBUG) << "ACS in progress and channel zero, so FREQ is unknown";
        return beerocks::eFreqType::FREQ_UNKNOWN;
    }

    LOG(ERROR) << "Unsupported channel:" << int(chn);
    return beerocks::eFreqType::FREQ_UNKNOWN;
}

bool wireless_utils::is_same_freq_band(int chn1, int chn2)
{
    if ((which_freq(chn1) == which_freq(chn2)) &&
        (which_freq(chn1) != beerocks::eFreqType::FREQ_UNKNOWN)) {
        return true;
    }

    return false;
}

bool wireless_utils::is_same_interface(const std::string &ifname1, const std::string &ifname2)
{
    return (ifname1 == ifname2);
}

beerocks::eSubbandType wireless_utils::which_subband(uint32_t chn)
{
    if ((START_OF_LOW_BAND_NON_DFS <= chn) && (chn <= END_OF_LOW_BAND)) {
        return beerocks::eSubbandType::LOW_SUBBAND;
    }

    if ((START_OF_HIGH_BAND <= chn) && (chn <= END_OF_HIGH_BAND)) {
        return beerocks::eSubbandType::HIGH_SUBBAND;
    }

    LOG(ERROR) << "Unsupported channel:" << (int)chn;
    return beerocks::eSubbandType::SUBBAND_UNKNOWN;
}

bool wireless_utils::is_low_subband(const uint32_t chn)
{
    return (which_subband(chn) == beerocks::eSubbandType::LOW_SUBBAND);
}

bool wireless_utils::is_high_subband(const uint32_t chn)
{
    return (which_subband(chn) == beerocks::eSubbandType::HIGH_SUBBAND);
}

bool wireless_utils::is_dfs_channel(const uint32_t chn)
{
    if (((chn >= START_OF_LOW_DFS_SUBBAND) && chn <= (END_OF_LOW_DFS_SUBBAND)) ||
        ((chn >= START_OF_HIGH_DFS_SUBBAND) && (chn <= END_OF_HIGH_DFS_SUBBAND))) {
        return true;
    }
    return false;
}

std::vector<std::pair<uint8_t, beerocks::eWifiChannelType>>
wireless_utils::split_channel_to_20MHz(int channel, beerocks::eWiFiBandwidth bw,
                                       bool channel_ext_above_secondary,
                                       bool channel_ext_above_primary)
{
    //split ch and bw to 20MHz channels.
    //example ch = 149 , bw = 80Mhz , channel_ext_above_secondary - true , channel_ext_above_primary = true.
    //channel_ext_above_secondary = (ch < center_freq) ? true:false;
    //channel_ext_above_primary - true , 153 is also primary.
    //split output: 149, 153, 157, 161.
    LOG(INFO) << "split_channel_to_20MHz";
    std::vector<std::pair<uint8_t, beerocks::eWifiChannelType>> ret;
    const int channel_step_5g = 4;
    const int channel_step_2g = 5;
    bool is_2g                = (which_freq(channel) == beerocks::eFreqType::FREQ_24G);
    bool is_5g                = (which_freq(channel) == beerocks::eFreqType::FREQ_5G);
    bool high_band            = is_5g ? wireless_utils::is_high_subband(channel) : false;
    int last_channel          = high_band ? END_OF_HIGH_BAND : END_OF_LOW_BAND;
    LOG(INFO) << "channel = " << int(channel)
              << " channel_ext_above_secondary = " << int(channel_ext_above_secondary)
              << " channel_ext_above_primary = " << int(channel_ext_above_primary);
    auto start_of_band_channel =
        channel - channel_step_5g * (channel_step_multiply(channel_ext_above_secondary,
                                                           channel_ext_above_primary));

    if (bw == beerocks::BANDWIDTH_20) {
        LOG(INFO) << "ret.push_back( {channel, beerocks::CH_PRIMARY} ); = " << int(channel)
                  << " beerocks::CH_ " << int(beerocks::CH_PRIMARY);
        ret.push_back({channel, beerocks::CH_PRIMARY});
    } else if (bw == beerocks::BANDWIDTH_40 && is_2g) {
        LOG(INFO) << "ret.push_back( {channel, beerocks::CH_PRIMARY} ); = " << int(channel)
                  << " beerocks::CH_ " << int(beerocks::CH_PRIMARY);
        ret.push_back({channel, beerocks::CH_PRIMARY});

        if (channel_ext_above_secondary) {
            channel += channel_step_2g;
            if (which_freq(channel) == beerocks::eFreqType::FREQ_24G)
                ret.push_back({channel, beerocks::CH_SECONDARY});
        } else {
            channel -= channel_step_2g;
            if (channel > 0)
                ret.push_back({channel, beerocks::CH_SECONDARY});
        }
    } else if (bw == beerocks::BANDWIDTH_40 || bw == beerocks::BANDWIDTH_80 ||
               bw == beerocks::BANDWIDTH_160) // 5G channels
    {
        LOG(INFO) << "ret.push_back( {channel, beerocks::CH_PRIMARY} ); = "
                  << int(start_of_band_channel) << " last_channel  = " << int(last_channel);

        int iterations =
            beerocks::utils::convert_bandwidth_to_int(bw) /
            40; // 40 = 20*2. 20 for number of channels, 2 for taking half to primary and half to secondary
        beerocks::eWifiChannelType earlyIterationsChannelType =
            channel_ext_above_secondary ? beerocks::CH_PRIMARY : beerocks::CH_SECONDARY;
        beerocks::eWifiChannelType lateIterationsChannelType =
            channel_ext_above_secondary ? beerocks::CH_SECONDARY : beerocks::CH_PRIMARY;
        for (int i = 0; i < iterations; i++) {
            if (start_of_band_channel <= last_channel) {
                ret.push_back({start_of_band_channel, earlyIterationsChannelType});
                start_of_band_channel += channel_step_5g;
            }
        }

        for (int i = 0; i < iterations; i++) {
            if (start_of_band_channel <= last_channel) {
                ret.push_back({start_of_band_channel, lateIterationsChannelType});
                start_of_band_channel += channel_step_5g;
            }
        }
    }

    LOG(INFO) << "channel_step_5g" << int(channel_step_5g);
    return ret;
}

std::vector<uint8_t> wireless_utils::get_5g_20MHz_channels(beerocks::eWiFiBandwidth bw,
                                                           uint16_t vht_center_frequency)
{
    std::vector<uint8_t> channels;
    LOG(INFO) << "vht_center_frequency = " << int(vht_center_frequency);
    switch (bw) {
    case beerocks::BANDWIDTH_20: {
        channels.push_back(freq_to_channel(vht_center_frequency));
        break;
    }
    case beerocks::BANDWIDTH_40: {
        channels.push_back(freq_to_channel(vht_center_frequency - 10));
        channels.push_back(freq_to_channel(vht_center_frequency + 10));
        break;
    }
    case beerocks::BANDWIDTH_80: {
        channels.push_back(freq_to_channel(vht_center_frequency - 30));
        channels.push_back(freq_to_channel(vht_center_frequency - 10));
        channels.push_back(freq_to_channel(vht_center_frequency + 10));
        channels.push_back(freq_to_channel(vht_center_frequency + 30));
        break;
    }
    case beerocks::BANDWIDTH_80_80:
    case beerocks::BANDWIDTH_160: {
        channels.push_back(freq_to_channel(vht_center_frequency - 70));
        channels.push_back(freq_to_channel(vht_center_frequency - 50));
        channels.push_back(freq_to_channel(vht_center_frequency - 30));
        channels.push_back(freq_to_channel(vht_center_frequency - 10));
        channels.push_back(freq_to_channel(vht_center_frequency + 10));
        channels.push_back(freq_to_channel(vht_center_frequency + 30));
        channels.push_back(freq_to_channel(vht_center_frequency + 50));
        channels.push_back(freq_to_channel(vht_center_frequency + 70));
        break;
    }
    default: {
        LOG(ERROR) << "INVALID BW:" << bw;
    }
    }
    std::for_each(std::begin(channels), std::end(channels),
                  [](uint8_t channel) { LOG(DEBUG) << "channel:" << int(channel); });
    return channels;
}

uint8_t wireless_utils::channel_step_multiply(bool channel_ext_above_secondary,
                                              bool channel_ext_above_primary)
{
    if (!channel_ext_above_secondary && !channel_ext_above_primary) {
        return 3;
    } else if (!channel_ext_above_secondary && channel_ext_above_primary) {
        return 2;
    } else if (channel_ext_above_secondary && !channel_ext_above_primary) {
        return 1;
    }
    return 0; //(channel_ext_above_secondary && !channel_ext_above_primary )
}

std::vector<uint8_t> wireless_utils::calc_5g_20MHz_subband_channels(
    beerocks::eWiFiBandwidth prev_bw, uint16_t prev_vht_center_frequency,
    beerocks::eWiFiBandwidth bw, uint16_t vht_center_frequency)
{
    std::vector<uint8_t> channels;

    if (prev_bw > bw) {
        std::vector<uint8_t> prev_channels =
            get_5g_20MHz_channels(prev_bw, prev_vht_center_frequency);
        std::vector<uint8_t> current_channels = get_5g_20MHz_channels(bw, vht_center_frequency);
        std::set_difference(prev_channels.begin(), prev_channels.end(), current_channels.begin(),
                            current_channels.end(), std::inserter(channels, channels.end()));
    } else {
        channels = get_5g_20MHz_channels(prev_bw, prev_vht_center_frequency);
    }
    LOG(DEBUG) << "prev_bw:" << beerocks::utils::convert_bandwidth_to_int(prev_bw)
               << " | BW:" << beerocks::utils::convert_bandwidth_to_int(bw)
               << " | channels empty:" << int(channels.empty());

    std::for_each(std::begin(channels), std::end(channels),
                  [](uint8_t channel) { LOG(DEBUG) << "channel:" << int(channel); });

    return channels;
}

uint8_t wireless_utils::get_5g_center_channel(uint8_t channel, beerocks::eWiFiBandwidth bandwidth)
{
    auto channel_it = channels_table_5g.find(channel);
    if (channel_it == channels_table_5g.end()) {
        return 0;
    }
    auto &bw_info_map = channel_it->second;

    if (bandwidth == beerocks::eWiFiBandwidth::BANDWIDTH_80_80) {
        bandwidth = beerocks::eWiFiBandwidth::BANDWIDTH_80;
    }

    auto bw_info_it = bw_info_map.find(bandwidth);
    if (bw_info_it == bw_info_map.end()) {
        return 0;
    }
    return bw_info_it->second.center_channel;
}

/**
 * @brief get operating class number by channel and channel bandwidth
 *
 * @param channel current channel parameters
 * @return operating class number
 */
uint8_t
wireless_utils::get_operating_class_by_channel(const beerocks::message::sWifiChannel &channel)
{
    // operating classes 128,129,130 use center channel **unlike the other classes**,
    // so convert channel and bandwidth to center channel.
    // For more info, refer to Table E-4 in the 802.11 specification.
    auto ch = channel.channel;
    auto bw = static_cast<beerocks::eWiFiBandwidth>(channel.channel_bandwidth);
    if (bw >= beerocks::eWiFiBandwidth::BANDWIDTH_80) {
        ch = wireless_utils::get_5g_center_channel(ch, bw);
    }
    for (auto oper_class : operating_classes_list) {
        if (oper_class.second.band == channel.channel_bandwidth &&
            oper_class.second.channels.find(ch) != oper_class.second.channels.end()) {
            return oper_class.first;
        }
    }
    return 0;
}

/**
 * @brief convert operating class to channel set based on Table 4-E in the ieee 802.11 specification
 *
 * @param operating_class operating class
 * @return std::set<uint8_t> set of supported channels by the operating class or empty if failure
 */
const std::set<uint8_t> &wireless_utils::operating_class_to_channel_set(uint8_t operating_class)
{
    static const std::set<uint8_t> empty_set = {};

    auto it = operating_classes_list.find(operating_class);
    if (it == operating_classes_list.end()) {
        LOG(ERROR) << "reserved operating class " << int(operating_class);
        return empty_set;
    }
    return it->second.channels;
}

/**
 * @brief convert operating class to bandwidth based on Table 4-E in the ieee 802.11 specification
 *
 * @param operating_class operating class
 * @return beerocks::eWiFiBandwidth enum of supported bandwidth for specific operating class.
 */
const beerocks::eWiFiBandwidth &
wireless_utils::operating_class_to_bandwidth(uint8_t operating_class)
{
    static const beerocks::eWiFiBandwidth NA = beerocks::eWiFiBandwidth::BANDWIDTH_UNKNOWN;
    auto it                                  = operating_classes_list.find(operating_class);
    if (it == operating_classes_list.end()) {
        LOG(ERROR) << "reserved operating class " << int(operating_class);
        return NA;
    }
    return it->second.band;
}

std::string wireless_utils::wsc_to_bwl_authentication(WSC::eWscAuth authtype)
{
    std::string authtype_str("");
    if (authtype & WSC::eWscAuth::WSC_AUTH_OPEN) {
        authtype_str += "NONE ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_WPAPSK) {
        authtype_str += "WPA-PSK ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_SHARED) {
        authtype_str += "SHARED ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_WPA) {
        authtype_str += "WPA ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_WPA2) {
        authtype_str += "WPA2 ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_WPA2PSK) {
        authtype_str += "WPA2-PSK ";
    }
    if (authtype & WSC::eWscAuth::WSC_AUTH_SAE) {
        authtype_str += "SAE ";
    }

    if (authtype_str.empty()) {
        return "INVALID";
    }
    return authtype_str;
}

std::string wireless_utils::wsc_to_bwl_encryption(WSC::eWscEncr enctype)
{
    switch (enctype) {
    case WSC::eWscEncr::WSC_ENCR_NONE:
        return "NONE";
    case WSC::eWscEncr::WSC_ENCR_WEP:
        return "WEP";
    case WSC::eWscEncr::WSC_ENCR_TKIP:
        return "TKIP";
    case WSC::eWscEncr::WSC_ENCR_AES:
        return "AES";
    default:
        return "INVALID";
    }
}

beerocks::eBssType wireless_utils::wsc_to_bwl_bss_type(WSC::eWscVendorExtSubelementBssType bss_type)
{
    if ((bss_type & WSC::eWscVendorExtSubelementBssType::BACKHAUL_BSS) &&
        (bss_type & WSC::eWscVendorExtSubelementBssType::FRONTHAUL_BSS)) {
        return beerocks::BSS_TYPE_BACK_FRONTHAUL;
    } else if (bss_type & WSC::eWscVendorExtSubelementBssType::BACKHAUL_BSS)
        return beerocks::BSS_TYPE_BACKHAUL;
    else if (bss_type & WSC::eWscVendorExtSubelementBssType::FRONTHAUL_BSS)
        return beerocks::BSS_TYPE_FRONTHAUL;
    else if (bss_type & WSC::eWscVendorExtSubelementBssType::TEARDOWN)
        return beerocks::BSS_TYPE_TEARDOWN;

    return beerocks::BSS_TYPE_INVALID;
}

std::list<uint8_t> wireless_utils::string_to_wsc_oper_class(const std::string &operating_class)
{
    std::list<uint8_t> radio_24g = {81, 82, 83, 84};
    std::list<uint8_t> radio_5g  = {115, 116, 117, 118, 119, 120, 121, 122,
                                   123, 124, 125, 126, 127, 128, 129, 130};
    std::list<uint8_t> radio_6g  = {131, 132, 133, 134, 135, 136};

    if (operating_class == "24g") {
        return radio_24g;
    }
    if (operating_class == "5gh") {
        return {121, 122, 123, 124, 125, 126, 127, 128, 129, 130};
    }
    if (operating_class == "5gl") {
        return {115, 116, 117, 118, 119, 120, 128, 129, 130};
    }
    if (operating_class == "5g") {
        return radio_5g;
    }
    if (operating_class == "24g-5g") {
        radio_5g.merge(radio_24g);
        return radio_5g;
    }
    if (operating_class == "6g") {
        return radio_6g;
    }
    LOG(WARNING) << "Operating class [" << operating_class << "] was not converted.";
    return {};
}

bool wireless_utils::is_channel_in_operating_class(uint8_t operating_class, uint8_t channel)
{
    auto channel_set = operating_class_to_channel_set(operating_class);

    return (channel_set.find(channel) != channel_set.end());
}

bool wireless_utils::is_frequency_band_5ghz(beerocks::eFreqType frequency_band)
{
    switch (frequency_band) {
    case beerocks::FREQ_24G:
        return false;
    case beerocks::FREQ_5G:
    case beerocks::FREQ_58G:
    case beerocks::FREQ_24G_5G:
        return true;
    default:
        LOG(WARNING) << "Cannot determine whether frequency band " << frequency_band << " is 5GHz";
        return false;
    }
}

wireless_utils::OverlappingChannels wireless_utils::get_overlapping_channels(uint8_t source_channel)
{
    OverlappingChannels ret;

    auto source_channel_it = channels_table_5g.find(source_channel);
    if (source_channel_it == channels_table_5g.end()) {
        LOG(ERROR) << "Couldn't find source channel " << source_channel
                   << " for overlapping channles";
        return ret;
    }

    // go over the table and if the source-cannel
    // is within the range of the current-channel, current-bandwidth
    // add current-channel, current-bandwidth to the output

    for (const auto &current_channel_it : channels_table_5g) {
        auto &bandwidth_map  = current_channel_it.second;
        auto current_channel = current_channel_it.first;
        for (const auto &current_bandwidth_it : bandwidth_map) {
            auto current_bandwidth = current_bandwidth_it.first;
            auto min_channel = current_bandwidth_it.second.overlap_beacon_channels_range.first;
            auto max_channel = current_bandwidth_it.second.overlap_beacon_channels_range.second;
            if (source_channel >= min_channel && source_channel <= max_channel) {
                ret.push_back(std::make_pair(current_channel, current_bandwidth));
            }
        }
    }
    return ret;
}

std::vector<uint8_t> wireless_utils::get_overlapping_beacon_channels(uint8_t beacon_channel,
                                                                     beerocks::eWiFiBandwidth bw)
{
    std::vector<uint8_t> overlapping_beacon_channels;

    auto ch_it = channels_table_5g.find(beacon_channel);
    if (ch_it == channels_table_5g.end()) {
        LOG(ERROR) << "Couldn't find channel " << beacon_channel;
        return {};
    }

    auto bw_it = ch_it->second.find(bw);
    if (bw_it == ch_it->second.end()) {
        LOG(ERROR) << "Couldn't find bw " << beerocks::utils::convert_bandwidth_to_int(bw)
                   << " on channel " << beacon_channel;
        return {};
    }

    auto channel_range_min = bw_it->second.overlap_beacon_channels_range.first;
    auto channel_range_max = bw_it->second.overlap_beacon_channels_range.second;

    constexpr uint8_t channels_distance_5g = 4;
    overlapping_beacon_channels.reserve(
        (channel_range_max - channel_range_min) / channels_distance_5g + 1);

    // Ignore if one of beacon channels is unavailable.
    for (uint8_t overlap_ch = channel_range_min; overlap_ch <= channel_range_max;
         overlap_ch += channels_distance_5g) {
        overlapping_beacon_channels.push_back(overlap_ch);
    }
    return overlapping_beacon_channels;
}

std::vector<uint8_t>
wireless_utils::center_channel_5g_to_beacon_channels(uint8_t center_channel,
                                                     beerocks::eWiFiBandwidth bw)
{
    // Return nothing on 2.4G channels
    if (center_channel < 36) {
        return {};
    }

    std::vector<uint8_t> beacon_channels;
    uint8_t beacon_channel;
    switch (bw) {
    case beerocks::BANDWIDTH_20:
        beacon_channels.push_back(center_channel);
        return beacon_channels;
    case beerocks::BANDWIDTH_40:
        beacon_channel = center_channel - 2;
        beacon_channels.reserve(2);
        break;
    case beerocks::BANDWIDTH_80:
    case beerocks::BANDWIDTH_80_80:
        beacon_channel = center_channel - 6;
        beacon_channels.reserve(4);
        break;
    case beerocks::BANDWIDTH_160:
        beacon_channel = center_channel - 14;
        beacon_channels.reserve(8);
        break;
    default: {
        LOG(DEBUG) << "Invalid BW: " << beerocks::utils::convert_bandwidth_to_int(bw)
                   << ", center_channel=" << center_channel;
        return {};
    }
    }

    for (size_t i = beacon_channel; i < beacon_channels.capacity(); i += 4) {
        beacon_channels.push_back(i);
    }

    return beacon_channels;
}

uint8_t wireless_utils::convert_rcpi_from_rssi(int8_t rssi)
{
    uint8_t rcpi;

    // According to 802.11-2016 convertion table (Table 9-154).
    constexpr int8_t lower_rssi_bound{-109}; // Standart defines as -109.5
    constexpr int8_t upper_rssi_bound{0};

    if (rssi < lower_rssi_bound) {

        rcpi = beerocks::RCPI_MIN; //represents RSSI < -109dBm

    } else if ((lower_rssi_bound <= rssi) && (rssi < upper_rssi_bound)) {

        rcpi = RCPI_EQUATION_COEF * (rssi + RCPI_EQUATION_CONSTANT);

    } else {

        rcpi = beerocks::RCPI_MAX;
    }

    return rcpi;
}

int8_t wireless_utils::convert_rssi_from_rcpi(uint8_t rcpi)
{
    if (rcpi > beerocks::RCPI_MAX) {
        LOG(ERROR) << "Invalid RCPI value in converion to RSSI.";
        return beerocks::RSSI_INVALID;
    }

    return ((rcpi / RCPI_EQUATION_COEF) - RCPI_EQUATION_CONSTANT);
}

bool wireless_utils::get_subset_20MHz_channels(const uint8_t channel_number,
                                               const uint8_t operating_class,
                                               const beerocks::eWiFiBandwidth operating_bandwidth,
                                               std::unordered_set<uint8_t> &resulting_channels)
{
    auto get_range = [&resulting_channels](std::pair<uint8_t, uint8_t> channels_range) {
        constexpr uint8_t channel_range_delta_20MHz = 4;
        for (auto iter = channels_range.first; iter <= channels_range.second;
             iter += channel_range_delta_20MHz) {
            resulting_channels.insert(iter);
        }
    };

    // If the channel is already 20MHz
    if (operating_bandwidth == beerocks::eWiFiBandwidth::BANDWIDTH_20) {
        // "channel_number" is an actual channel
        resulting_channels.insert(channel_number);
        return true;
    }

    // If the channel is using a 2.4GHz operating class
    if (operating_class < 115) {
        // "channel_number" is an actual channel
        resulting_channels.insert(channel_number);
        return true;
    }

    // The given channel number is a central channel
    // Iterate over the 5GHz channel table.
    for (const auto &channel_it : son::wireless_utils::channels_table_5g) {
        // Find the bandwidth within the channel
        const auto bw_channel_elem = channel_it.second.find(operating_bandwidth);
        if (bw_channel_elem == channel_it.second.end()) {
            continue;
        }
        // Check if the central channel matches the found bandwidth element
        if (bw_channel_elem->second.center_channel != channel_number) {
            continue;
        }
        // Get the range of the subset of 20MHz channels
        get_range(bw_channel_elem->second.overlap_beacon_channels_range);
        return true;
    }
    // No matching elements were found
    return false;
}

/**
 * @brief get max supported bandwidth in station capabilities.
 * in this order:
 * - max_ch_width (valid even for a/b/g)
 * - vht_bw (valid for ac)
 * - ht_bw (valid for n)
 * @param sta_caps in station capabilities
 * @param max_bw out filled max supported bandwidth
 * @return false if none of above is valid bw (+unchanged out param)
 */
bool wireless_utils::get_station_max_supported_bw(beerocks::message::sRadioCapabilities &sta_caps,
                                                  beerocks::eWiFiBandwidth &max_bw)
{
    auto max_bw_hdlr = [](uint8_t in_bw, beerocks::eWiFiBandwidth &out_bw) -> bool {
        if (in_bw != beerocks::BANDWIDTH_UNKNOWN && in_bw < beerocks::BANDWIDTH_MAX) {
            out_bw = beerocks::eWiFiBandwidth(in_bw);
            return true;
        }
        return false;
    };
    return (max_bw_hdlr(sta_caps.max_ch_width, max_bw) || max_bw_hdlr(sta_caps.vht_bw, max_bw) ||
            max_bw_hdlr(sta_caps.ht_bw, max_bw));
}

void wireless_utils::print_station_capabilities(beerocks::message::sRadioCapabilities &sta_caps)
{
    LOG(DEBUG) << "sta HT_CAPS:" << std::endl
               << "bw20 short gi = " << (sta_caps.ht_low_bw_short_gi ? "true" : "false")
               << std::endl
               << "bw40 short gi = " << (sta_caps.ht_high_bw_short_gi ? "true" : "false")
               << std::endl
               << "ht_mcs = " << ((int(sta_caps.ht_mcs)) ? std::to_string(sta_caps.ht_mcs) : "n/a")
               << std::endl
               << "ht_ss = " << ((int(sta_caps.ht_ss)) ? std::to_string(sta_caps.ht_ss) : "n/a")
               << std::endl
               << "ht_bw = "
               << ((sta_caps.ht_bw != beerocks::BANDWIDTH_UNKNOWN &&
                    sta_caps.ht_bw < beerocks::BANDWIDTH_MAX)
                       ? std::to_string(beerocks::utils::convert_bandwidth_to_int(
                             beerocks::eWiFiBandwidth(sta_caps.ht_bw)))
                       : "n/a")
               << std::endl
               << "ht_sm_power_save = " << ([](uint8_t n) {
                      switch (n) {
                      case beerocks::HT_SM_POWER_SAVE_MODE_STATIC:
                          return "static";
                      case beerocks::HT_SM_POWER_SAVE_MODE_DYNAMIC:
                          return "dynamic";
                      case beerocks::HT_SM_POWER_SAVE_MODE_RESERVED:
                          return "reserved(ERROR)";
                      case beerocks::HT_SM_POWER_SAVE_MODE_DISABLED:
                          return "disabled";
                      }
                      return "ERROR";
                  })(sta_caps.ht_sm_power_save);
    LOG(DEBUG) << "sta VHT_CAPS:" << std::endl
               << "bw80 short gi = " << (sta_caps.vht_low_bw_short_gi ? "true" : "false")
               << std::endl
               << "bw160 short gi = " << (sta_caps.vht_high_bw_short_gi ? "true" : "false")
               << std::endl
               << "vht_ss = " << ((int(sta_caps.vht_ss)) ? std::to_string(sta_caps.vht_ss) : "n/a")
               << std::endl
               << "vht_mcs = "
               << ((int(sta_caps.vht_mcs)) ? std::to_string(sta_caps.vht_mcs) : "n/a") << std::endl
               << "vht_su_beamformer = " << (sta_caps.vht_su_beamformer ? "true" : "false")
               << std::endl
               << "vht_mu_beamformer = " << (sta_caps.vht_mu_beamformer ? "true" : "false")
               << std::endl
               << "vht_bw = "
               << ((sta_caps.vht_bw != beerocks::BANDWIDTH_UNKNOWN &&
                    sta_caps.vht_bw < beerocks::BANDWIDTH_MAX)
                       ? std::to_string(beerocks::utils::convert_bandwidth_to_int(
                             beerocks::eWiFiBandwidth(sta_caps.vht_bw)))
                       : "n/a");
    LOG(DEBUG) << "sta DEFAULT_CAPS:" << std::endl
               << "default_mcs = " << int(sta_caps.default_mcs) << std::endl
               << "default_short_gi = " << int(sta_caps.default_short_gi);
    LOG(DEBUG) << "sta OTHER_CAPS:" << std::endl
               << "wifi_standard [enum] = " << int(sta_caps.wifi_standard) << std::endl
               << "btm_supported = " << (sta_caps.btm_supported ? "true" : "false") << std::endl
               << "nr_enabled = " << (sta_caps.nr_enabled ? "true" : "false") << std::endl
               << "cell_capa = " << int(sta_caps.cell_capa) << std::endl
               << "link_meas = " << int(sta_caps.link_meas) << std::endl
               << "beacon_report_passive = " << int(sta_caps.beacon_report_passive) << std::endl
               << "beacon_report_active = " << int(sta_caps.beacon_report_active) << std::endl
               << "beacon_report_table = " << int(sta_caps.beacon_report_table) << std::endl
               << "lci_meas = " << int(sta_caps.lci_meas) << std::endl
               << "fmt_range_report = " << int(sta_caps.fmt_range_report);
}

uint16_t wireless_utils::get_vht_mcs_set(uint8_t vht_mcs, uint8_t vht_ss)
{
    uint16_t vht_mcs_set = 0xffff;
    for (auto i = 0; vht_mcs < 10 && i < vht_ss && i < 8; i++) {
        vht_mcs_set &= ~(((10 - vht_mcs) & 0x03) << (i * 2));
    }
    return vht_mcs_set;
}
