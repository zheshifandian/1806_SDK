/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/son/son_wireless_utils.h>

#include <gtest/gtest.h>

namespace {

// See https://docs.google.com/spreadsheets/d/1J2IYuGjFX_OQQpgb4OgsyDx73klAUk6qSmeB4acJ5us
// for the table we need to conform to in the get_operating_class_by_channel() function
// implementation.

struct sOperatingClass {
    uint8_t operating_class;
    beerocks::eWiFiBandwidth bandwidth;
    std::set<uint8_t> channels;
};

// clang-format off
const std::vector<sOperatingClass>
    table_E_4_global_operating_classes_802_11{
    { 81, beerocks::BANDWIDTH_20, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
    { 82, beerocks::BANDWIDTH_20, {14}},
    { 83, beerocks::BANDWIDTH_40, {1, 2, 3, 4, 5, 6, 7, 8, 9}},
    /* Note: we have no way to distinguish 40- and 40+, so operating class 83 is used for all bandwidth=40MHz */
    { 84, beerocks::BANDWIDTH_40, {/*5, 6, 7, 8, 9, */10, 11, 12, 13}},
    {115, beerocks::BANDWIDTH_20, {36, 40, 44, 48}},
    {116, beerocks::BANDWIDTH_40, {36, 44}},
    {117, beerocks::BANDWIDTH_40, {40, 48}},
    {118, beerocks::BANDWIDTH_20, {52, 56, 60, 64}},
    {119, beerocks::BANDWIDTH_40, {52, 60}},
    {120, beerocks::BANDWIDTH_40, {56, 64}},
    {121, beerocks::BANDWIDTH_20, {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144}},
    {122, beerocks::BANDWIDTH_40, {100, 108, 116, 124, 132, 136, 140}},
    {123, beerocks::BANDWIDTH_40, {104, 112, 120, 128, 134, 138, 144}},
    {124, beerocks::BANDWIDTH_20, {149, 153, 157, 161}},
    /* Note: we have no way to distinguish repeated channels in operating classes 124 and 125 used for bandwidth=20MHz */
    {125, beerocks::BANDWIDTH_20, {/*149, 153, 157, 161,*/ 165, 169}},
    {126, beerocks::BANDWIDTH_40, {149, 157}},
    {127, beerocks::BANDWIDTH_40, {153, 161}},
    {128, beerocks::BANDWIDTH_80, {42, 58, 106, 122, 138, 155}},
    {129, beerocks::BANDWIDTH_160, {50, 114}},
    {130, beerocks::BANDWIDTH_80_80, {42, 58, 106, 122, 138, 155}}
};
// clang-format on

/**
 * Parameter type used in value-parameterized tests of get_operating_class_by_channel() function.
 * Tuple elements are the WiFi channel used as parameter to the function and the expected
 * operating class obtained as result.
 */
typedef std::tuple<beerocks::message::sWifiChannel, uint8_t> tGetOperatingClassByChannelParam;

/**
 * @brief Gets starting channel for given center channel and bandwidth values.
 *
 * Operating classes 128, 129 and 130 defined in table_E_4_global_operating_classes_802_11 use
 * center channel **unlike the other classes**. Calls to get_operating_class_by_channel()
 * require a starting channel number and this function is used to compute it.
 *
 * @param center_channel Center channel number.
 * @param bandwidth Channel bandwidth.
 * @return Starting channel number.
 */
uint8_t get_starting_channel(uint8_t center_channel, beerocks::eWiFiBandwidth bandwidth)
{
    const uint8_t first_vht_20_mhz_channel  = 36;
    const uint8_t first_vht_80_mhz_channel  = 42;
    const uint8_t first_vht_160_mhz_channel = 50;

    if (bandwidth == beerocks::eWiFiBandwidth::BANDWIDTH_160) {
        return center_channel - (first_vht_160_mhz_channel - first_vht_20_mhz_channel);
    }

    if (bandwidth >= beerocks::eWiFiBandwidth::BANDWIDTH_80) {
        return center_channel - (first_vht_80_mhz_channel - first_vht_20_mhz_channel);
    }

    return center_channel;
}

/**
 * @brief Gets a list of all possible valid parameters to get_operating_class_by_channel() function
 * together with the expected result for each one of them.
 *
 * Creates a container with all possible combinations of channel number and channel bandwidth to
 * create a valid sWifiChannel to use as parameter to get_operating_class_by_channel() function and
 * the expected operating class to be obtained as result.
 *
 * @return vector with pairs of valid WiFi channels and their expected operating class result.
 */
std::vector<tGetOperatingClassByChannelParam> get_operating_class_by_channel_valid_parameters()
{
    std::vector<tGetOperatingClassByChannelParam> result;

    for (const auto &entry : table_E_4_global_operating_classes_802_11) {
        for (const auto channel : entry.channels) {
            beerocks::message::sWifiChannel ch;
            ch.channel           = get_starting_channel(channel, entry.bandwidth);
            ch.channel_bandwidth = entry.bandwidth;

            result.emplace_back(std::make_tuple(ch, entry.operating_class));
        }
    }

    return result;
}

/**
 * @brief Gets a list of invalid parameters to get_operating_class_by_channel() function.
 *
 * Creates a container of sWifiChannel objects built up with invalid combinations of channel
 * number and channel bandwidth. The get_operating_class_by_channel() function returns 0 when
 * called with any of these invalid channels as parameter.
 *
 * @return vector of invalid WiFi channels.
 */
std::vector<tGetOperatingClassByChannelParam> get_operating_class_by_channel_invalid_parameters()
{
    // clang-format off
    const beerocks::eWiFiBandwidth bandwidths[]{
        beerocks::BANDWIDTH_UNKNOWN,
        beerocks::BANDWIDTH_20,
        beerocks::BANDWIDTH_40,
        beerocks::BANDWIDTH_80,
        beerocks::BANDWIDTH_80_80,
        beerocks::BANDWIDTH_160
    };
    // clang-format on

    std::vector<tGetOperatingClassByChannelParam> result;

    /**
     * Note: Google Test documentation says that using value-parameterized tests to test your code
     * over various inputs (a.k.a. data-driven testing) is a feature easy to abuse of.
     * This might well be a good example of such an abuse.
     */

    /**
     * Why do-while instead of for loop?
     * So that the loop executes up until and INCLUDING channel == UINT8_MAX,
     * for (uint8_t channel = 0; channel <= UINT8_MAX; ++channel) would loop forever !!!
     */
    uint8_t channel = 0;
    do {
        for (auto bandwidth : bandwidths) {
            const auto &table = table_E_4_global_operating_classes_802_11;

            auto it = std::find_if(
                table.begin(), table.end(), [&](const sOperatingClass &operating_class) {
                    if (operating_class.bandwidth != bandwidth) {
                        return false;
                    }
                    auto op_class            = operating_class.operating_class;
                    uint8_t channel_to_check = channel;
                    if (op_class == 128 || op_class == 129 || op_class == 130) {
                        channel_to_check =
                            son::wireless_utils::get_5g_center_channel(channel, bandwidth);
                    }
                    return operating_class.channels.end() !=
                           operating_class.channels.find(channel_to_check);
                });
            if (it == table.end()) {
                beerocks::message::sWifiChannel ch;
                ch.channel           = channel;
                ch.channel_bandwidth = bandwidth;

                result.emplace_back(std::make_tuple(ch, 0));
            }
        }
    } while (channel++ < UINT8_MAX);

    return result;
}

/**
 * @brief Converts a channel number to its string representation.
 *
 * Helper function to build the channel part of the name of a value-parameterized test parameter.
 *
 * @param value Channel number.
 * @return Channel number as a string.
 */
std::string channel_to_string(uint8_t value) { return "channel_" + std::to_string(value); }

/**
 * @brief Converts a channel bandwidth to its string representation.
 *
 * Helper function to build the bandwidth part of the name of a value-parameterized test parameter.
 *
 * @param value Channel bandwidth.
 * @return Channel bandwidth as a string.
 */
std::string bandwidth_to_string(uint8_t value)
{
    // clang-format off
    const std::map<beerocks::eWiFiBandwidth, std::string> bandwidths{
        {beerocks::BANDWIDTH_UNKNOWN, "0"},
        {beerocks::BANDWIDTH_20, "20"},
        {beerocks::BANDWIDTH_40, "40"},
        {beerocks::BANDWIDTH_80, "80"},
        {beerocks::BANDWIDTH_80_80, "80_80"},
        {beerocks::BANDWIDTH_160, "160"}
    };
    // clang-format on

    std::string result = "bandwidth_";

    auto bandwidth = static_cast<beerocks::eWiFiBandwidth>(value);

    const auto &it = bandwidths.find(bandwidth);
    if (bandwidths.end() == it) {
        result += "invalid_" + std::to_string(value);
    } else {
        result += it->second;
    }

    return result;
}

/**
 * @brief Converts a operating class code to its string representation.
 *
 * Helper function to build the operating class part of the name of a value-parameterized test
 * parameter.
 *
 * @param value Operating class code.
 * @return Operating class as a string.
 */
std::string operating_class_to_string(uint8_t value)
{
    return "operating_class_" + std::to_string(value);
}

/**
 * Fixture class to write value-parameterized tests for method
 * wireless_utils::get_operating_class_by_channel()
 *
 * See Value-Parameterized Tests documentation at
 * https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#value-parameterized-tests
 */
class WirelessUtilsGetOperatingClassByChannelTest
    : public testing::TestWithParam<tGetOperatingClassByChannelParam> {
};

/**
 * @brief Converts a parameter of a get_operating_class_by_channel() value-parameterized test to
 * its string representation.
 *
 * Helper function to create a custom test name suffix based on the test parameter value.
 *
 * @param info Information of the value-parameterized test parameter.
 * @return Value-parameterized test parameter as a string.
 */
std::string operating_class_by_channel_param_to_string(
    const testing::TestParamInfo<tGetOperatingClassByChannelParam> &info)
{
    const auto &param = info.param;

    return channel_to_string(std::get<0>(param).channel) + "_" +
           bandwidth_to_string(std::get<0>(param).channel_bandwidth) + "_" +
           operating_class_to_string(std::get<1>(param));
}

TEST_P(WirelessUtilsGetOperatingClassByChannelTest, should_return_expected)
{
    beerocks::message::sWifiChannel channel;
    uint8_t expected_operating_class;
    std::tie(channel, expected_operating_class) = GetParam();

    EXPECT_EQ(son::wireless_utils::get_operating_class_by_channel(channel),
              expected_operating_class);
}

INSTANTIATE_TEST_SUITE_P(ValidParamsInstance, WirelessUtilsGetOperatingClassByChannelTest,
                         testing::ValuesIn(get_operating_class_by_channel_valid_parameters()),
                         operating_class_by_channel_param_to_string);

INSTANTIATE_TEST_SUITE_P(InvalidParamsInstance, WirelessUtilsGetOperatingClassByChannelTest,
                         testing::ValuesIn(get_operating_class_by_channel_invalid_parameters()),
                         operating_class_by_channel_param_to_string);

TEST(overlapping_channels, channel_112)
{
    // channel 112 test
    son::wireless_utils::OverlappingChannels result =
        son::wireless_utils::get_overlapping_channels(112);

    EXPECT_EQ(result.size(), 15);
    EXPECT_FALSE(std::find(result.begin(), result.end(),
                           std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                               100, beerocks::BANDWIDTH_20)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              100, beerocks::BANDWIDTH_80)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              100, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              104, beerocks::BANDWIDTH_80)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              104, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              108, beerocks::BANDWIDTH_40)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              108, beerocks::BANDWIDTH_80)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              108, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              112, beerocks::BANDWIDTH_20)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              112, beerocks::BANDWIDTH_40)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              112, beerocks::BANDWIDTH_80)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              112, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              116, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              120, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              124, beerocks::BANDWIDTH_160)) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(),
                          std::pair<uint8_t, beerocks::eWiFiBandwidth>(
                              128, beerocks::BANDWIDTH_160)) != result.end());
}

// clang-format off
const auto staCaps_2_4 = []() {
    beerocks::message::sRadioCapabilities caps;
    caps.ant_num             = 1;
    caps.wifi_standard       = beerocks::STANDARD_N;
    caps.ht_ss               = 1;
    caps.ht_mcs              = beerocks::MCS_7;
    caps.ht_bw               = beerocks::BANDWIDTH_40;
    caps.ht_low_bw_short_gi  = 1;
    caps.ht_high_bw_short_gi = 1;
    return caps;
}();

const auto staCaps_5 = []() {
    beerocks::message::sRadioCapabilities caps;
    caps.ant_num              = 2;
    caps.wifi_standard        = beerocks::STANDARD_N |
                                beerocks::STANDARD_AC;
    caps.ht_ss                = 2;
    caps.ht_mcs               = beerocks::MCS_7;
    caps.ht_bw                = beerocks::BANDWIDTH_40;
    caps.ht_low_bw_short_gi   = 1;
    caps.ht_high_bw_short_gi  = 1;
    caps.vht_ss               = 2;
    caps.vht_mcs              = beerocks::MCS_9;
    caps.vht_bw               = beerocks::BANDWIDTH_160;
    caps.vht_low_bw_short_gi  = 1;
    caps.vht_high_bw_short_gi = 0;
    return caps;
}();
// clang-format on

TEST(estimate_ul_params, too_low_phy_rate)
{
    /*
     * phy_rate is below the lowest limit in phy_rate_table
     * => then the lowest ul_params are returned
     */
    son::wireless_utils::sPhyUlParams result = son::wireless_utils::estimate_ul_params(
        -60, 60, &staCaps_2_4, beerocks::BANDWIDTH_40, false);
    son::wireless_utils::sPhyUlParams result_min = son::wireless_utils::estimate_ul_params(
        -60, 65, &staCaps_2_4, beerocks::BANDWIDTH_20, false);

    EXPECT_EQ(result.status, son::wireless_utils::ESTIMATION_FAILURE_BELOW_RANGE);
    EXPECT_EQ(result.rssi, result_min.rssi);
    EXPECT_EQ(result.tx_power, result_min.tx_power);
}

TEST(estimate_ul_params, too_high_phy_rate)
{
    /*
     * phy_rate is beyond the highest supported rate in phy_rate_table
     * => then the highest ul_params are returned
     */
    son::wireless_utils::sPhyUlParams result = son::wireless_utils::estimate_ul_params(
        -30, 18000, &staCaps_5, beerocks::BANDWIDTH_160, true);
    son::wireless_utils::sPhyUlParams result_max = son::wireless_utils::estimate_ul_params(
        -30, 17333, &staCaps_5, beerocks::BANDWIDTH_160, true);

    EXPECT_EQ(result.status, son::wireless_utils::ESTIMATION_SUCCESS);
    EXPECT_EQ(result.rssi, result_max.rssi);
    EXPECT_EQ(result.tx_power, result_max.tx_power);
}

TEST(estimate_ap_tx_phy_rate, match_rssi_to_rate)
{
    /*
     * provided dl_rssi matches a phy_rate_table entry
     * but it corresponds to a lower channel width
     */
    double result = son::wireless_utils::estimate_ap_tx_phy_rate(-81, &staCaps_5,
                                                                 beerocks::BANDWIDTH_160, true);
    double approx =
        son::wireless_utils::estimate_ap_tx_phy_rate(-81, &staCaps_5, beerocks::BANDWIDTH_80, true);

    EXPECT_EQ(result, approx);
}

TEST(estimate_ap_tx_phy_rate, no_match_rssi_to_rate)
{
    /*
     * provided dl_rssi is too low and does not match any entry
     * => the lowest estimation of phy rate is returned
     */
    double result      = son::wireless_utils::estimate_ap_tx_phy_rate(-100, &staCaps_2_4,
                                                                 beerocks::BANDWIDTH_40, false);
    double lower_limit = son::wireless_utils::estimate_ap_tx_phy_rate(
        -89, &staCaps_2_4, beerocks::BANDWIDTH_20, false);

    EXPECT_EQ(result, lower_limit);
}

TEST(mcs_from_rate, matched_mcs_from_rate)
{
    /*
     * exact rate matched while iterating through phy rate entries
     */
    uint8_t mcs;
    uint8_t short_gi;
    auto result = son::wireless_utils::get_mcs_from_rate(1350, beerocks::ANT_MODE_1X1_SS1,
                                                         beerocks::BANDWIDTH_40, mcs, short_gi);

    EXPECT_TRUE(result);
    EXPECT_EQ(mcs, beerocks::MCS_6);
    EXPECT_EQ(short_gi, 1);
}

TEST(mcs_from_rate, neerest_mcs_from_rate)
{
    /*
     * no exact rate matched while iterating through phy rate entries
     * => get neerest rate and return its mcs
     */
    uint8_t mcs;
    uint8_t short_gi;
    auto result = son::wireless_utils::get_mcs_from_rate(7030, beerocks::ANT_MODE_2X2_SS2,
                                                         beerocks::BANDWIDTH_160, mcs, short_gi);

    EXPECT_FALSE(result);
    EXPECT_EQ(mcs, beerocks::MCS_4);
    EXPECT_EQ(short_gi, 0);
}

} // namespace
