/*
 * =====================================================================================
 *
 *       Filename:  temp/src/temp.h
 *
 *    Description:  Monitor the current chip temperature and take corresponding cooling
 *      measures.
 *
 *        Version:  1.0
 *        Created:  12/20/2021 10:18:20 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming.Guang, ming.guang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */

#ifndef __TEMP_H
#define __TEMP_H

#define TEMP_CTRL_DEFAULT_JUMP_LIMIT 5
#define SIZE_OF_FILE_NAME 11
#define TEMP_CTRL_LB_FILE_PTAH "/lib/firmware/temperature_lb_control.ini"
#define TEMP_CTRL_HB_FILE_PTAH "/lib/firmware/temperature_hb_control.ini"
#define GET_TEMP_PTAH "/sys/kernel/debug/aetnensis/temperature"

#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)

/**
 * struct siwifi_temp_ctrl - temperature control
 *
 * This structure describes some temp info about a band.
 *
 * @enable_temp_ctrl: Temperature control switch,0 means
 * close, 1 means open.
 * @enable_cool_ctrl: Describe whether cooling measures
 * are being taken,0 means not,1 means yes.
 * @band_flag: Description is 2.4G or 5g,0 means 2.4G,1 means 5G.
 * @sleep_time: Time interval of each monitoring.
 * @jump_count: Cooling level jump times,total 5.
 * @temp_ctrl_level: Regulating temperature threshold,first means
 * the high temp, second means the low temp.
 * @time_ctrl_count: Count listening times,first means duration of
 * current cooling level,second means current cooling duration.
 * @time_ctrl_limit: Jump limit, forst means jump limit for different
 * cooling levels, second means total cooling interval.The listening
 * interval needs to be multiplied by this value.
 * @tx_ctrl_limit: Different levels of restrictive TX measures,total 5.
 */
struct siwifi_temp_ctrl {
    bool enable_temp_ctrl;
    bool enable_cool_ctrl;
    int band_flag;
    int sleep_time;
    int jump_count;
    int temp_ctrl_level[2];
    int time_ctrl_count[2];
    int time_ctrl_limit[2];
    int tx_ctrl_limit[TEMP_CTRL_DEFAULT_JUMP_LIMIT];
};

const char * info_name[SIZE_OF_FILE_NAME] = {
    "TEMP_CTRL_ENABLE=",
    "UPPER_LIMIT_TEMP=",
    "LOWER_LIMIT_TEMP=",
    "SLEEP_TIME=",
    "JUMP_INTERVAL=",
    "COOLING_INTERVAL=",
    "TX_CTRL_LIMIT_PERCENT_LV1=",
    "TX_CTRL_LIMIT_PERCENT_LV2=",
    "TX_CTRL_LIMIT_PERCENT_LV3=",
    "TX_CTRL_LIMIT_PERCENT_LV4=",
    "TX_CTRL_LIMIT_PERCENT_LV5=",
};

const int lb_rate2hwq_table[100] = {
    // close aggregation of MSDU frames
    // 0 ~ 12
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 63,
    // 13 ~ 25
    63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
    // 26 ~ 38
    62, 62, 62, 62, 62, 62, 61, 61, 61, 61, 61, 61, 60,
    // 39 ~ 51
    60, 60, 60, 60, 60, 59, 59, 59, 59, 59, 58, 58, 58,
    // 52 ~ 64
    57, 57, 57, 57, 56, 56, 56, 55, 55, 55, 54, 53, 53,
    // 65 ~ 74
    52, 52, 51, 50, 50, 49, 48, 47, 0,
    // open aggregation of MSDU frames
    // 75 ~ 87
    50, 50, 50, 49, 49, 48, 47, 46, 45, 45, 44, 43, 43,
    // 88 ~ 100
    43, 43, 42, 42, 41, 40, 39, 39, 38, 37, 36, 35, 0,
};

const int hb_rate2hwq_table[100] = {
    // close aggregation of MSDU frames
    // 0 ~ 12
    64, 64, 64, 64, 64, 64, 64, 63, 63, 62, 62, 62, 62,
    // 13 ~ 25
    61, 61, 60, 59, 58, 58, 57 ,57, 56, 55, 54, 53, 52,
    // 26 ~ 38
    51, 50, 50, 49, 45, 43, 43, 42, 41, 40, 39, 38, 37,
    // 39 ~ 49
    37, 36, 35, 35, 34, 34, 32, 32, 31, 30,
    // open aggregation of MSDU frames
    // 50 ~ 62
    58, 58, 58, 57, 57, 56, 56, 56, 55, 55, 54, 53, 53,
    // 63 ~ 75
    52, 52, 52, 51, 51, 50, 50, 49, 48, 47, 46, 45, 44,
    // 76 ~ 88
    43, 42, 41, 40, 32, 31, 30, 29, 28, 27, 22, 20, 17,
    // 89 ~ 100
    16, 15, 12, 10, 10, 10, 10, 10, 10, 10, 10, 0,
};

#endif /* __H */
