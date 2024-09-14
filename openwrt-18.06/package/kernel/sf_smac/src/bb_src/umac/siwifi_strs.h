/**
 ****************************************************************************************
 *
 * @file siwifi_strs.h
 *
 * @brief Miscellaneous debug strings
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

#ifndef _SIWIFI_STRS_H_
#define _SIWIFI_STRS_H_

#ifdef CONFIG_SIWIFI_CMDSTR
#include "lmac_msg.h"

#define SIWIFI_ID2STR(tag) (((MSG_T(tag) < ARRAY_SIZE(siwifi_id2str)) &&        \
                           (siwifi_id2str[MSG_T(tag)]) &&          \
                           ((siwifi_id2str[MSG_T(tag)])[MSG_I(tag)])) ?   \
                          (siwifi_id2str[MSG_T(tag)])[MSG_I(tag)] : "unknown")

extern const char *const *siwifi_id2str[TASK_LAST_EMB + 1];
#else
#define SIWIFI_ID2STR(tag) ("unknown")
#endif

#endif /* _SIWIFI_STRS_H_ */
