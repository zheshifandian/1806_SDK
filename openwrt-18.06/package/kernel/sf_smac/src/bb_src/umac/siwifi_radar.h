/**
 ******************************************************************************
 *
 * @file siwifi_radar.h
 *
 * @brief Functions to handle radar detection
 *
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */
#ifndef _SIWIFI_RADAR_H_
#define _SIWIFI_RADAR_H_

#include <linux/nl80211.h>

struct siwifi_vif;
struct siwifi_hw;
struct radar_detector_specs;

enum siwifi_radar_chain {
    SIWIFI_RADAR_RIU = 0,
    SIWIFI_RADAR_FCU,
    SIWIFI_RADAR_LAST
};

enum siwifi_radar_detector {
    SIWIFI_RADAR_DETECT_DISABLE = 0, /* Ignore radar pulses */
    SIWIFI_RADAR_DETECT_ENABLE  = 1, /* Process pattern detection but do not
                                      report radar to upper layer (for test) */
    SIWIFI_RADAR_DETECT_REPORT  = 2  /* Process pattern detection and report
                                      radar to upper layer. */
};

#ifdef CONFIG_SIWIFI_RADAR
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include "lmac_msg.h"
#define SIWIFI_RADAR_PULSE_MAX  32

// correction of length due to inaccurate hardware detection
#define RADAR_LENGTH(len) (len * 2 > 22?(len * 2 - 11):len * 2)

/**
 * struct siwifi_radar_pulses - List of pulses reported by HW
 * @index: write index
 * @count: number of valid pulses
 * @buffer: buffer of pulses
 */
struct siwifi_radar_pulses {
    /* Last radar pulses received */
    int index;
    int count;
    struct phy_radar_pulse  buffer[SIWIFI_RADAR_PULSE_MAX];
};

/**
 * struct dfs_pattern_detector - DFS pattern detector
 * @region: active DFS region, NL80211_DFS_UNSET until set
 * @num_radar_types: number of different radar types
 * @last_pulse_ts: time stamp of last valid pulse in usecs
 * @prev_jiffies:
 * @radar_detector_specs: array of radar detection specs
 * @channel_detectors: list connecting channel_detector elements
 */
struct dfs_pattern_detector {
    u8 enabled;
    enum nl80211_dfs_regions region;
    u8 num_radar_types;
    u64 last_pulse_ts;
    u32 prev_jiffies;
    const struct radar_detector_specs *radar_spec;
    /* used to record pulses for a complete radar max is SIWIFI_RADAR_PULSE_MAX(32)
       if > 32, record last 32
     */
    struct siwifi_radar_pulses total_pulses;
    u8 consecutive_too_short_count;
    struct list_head detectors[];
};

#define NX_NB_RADAR_DETECTED 4

/**
 * struct siwifi_radar_detected - List of radar detected
 */
struct siwifi_radar_detected {
    u16 index;
    u16 count;
    unsigned long time[NX_NB_RADAR_DETECTED];
    s16 freq[NX_NB_RADAR_DETECTED];
};


struct siwifi_radar {
    /* pulses is pulse that be processed every workqueue */
    struct siwifi_radar_pulses pulses[SIWIFI_RADAR_LAST];
    struct dfs_pattern_detector *dpd[SIWIFI_RADAR_LAST];
    struct siwifi_radar_detected detected[SIWIFI_RADAR_LAST];
    struct work_struct detection_work;  /* Work used to process radar pulses */
    spinlock_t lock;                    /* lock for pulses processing */

    /* In softmac cac is handled by mac80211 */
    struct delayed_work cac_work;       /* Work used to handle CAC */
    struct siwifi_vif *cac_vif;           /* vif on which we started CAC */
};

bool siwifi_radar_detection_init(struct siwifi_radar *radar);
void siwifi_radar_detection_deinit(struct siwifi_radar *radar);
bool siwifi_radar_set_domain(struct siwifi_radar *radar,
                           enum nl80211_dfs_regions region);
void siwifi_radar_detection_enable(struct siwifi_radar *radar, u8 enable, u8 chain);
bool siwifi_radar_detection_is_enable(struct siwifi_radar *radar, u8 chain);
void siwifi_radar_start_cac(struct siwifi_radar *radar, u32 cac_time_ms,
                          struct siwifi_vif *vif);
void siwifi_radar_cancel_cac(struct siwifi_radar *radar);
void siwifi_radar_detection_enable_on_cur_channel(struct siwifi_hw *siwifi_hw);
int  siwifi_radar_dump_pattern_detector(char *buf, size_t len,
                                      struct siwifi_radar *radar, u8 chain);
int  siwifi_radar_dump_radar_detected(char *buf, size_t len,
                                    struct siwifi_radar *radar, u8 chain);

void siwifi_radar_detected(struct siwifi_hw *siwifi_hw);
#else

struct siwifi_radar {
};

static inline bool siwifi_radar_detection_init(struct siwifi_radar *radar)
{return true;}

static inline void siwifi_radar_detection_deinit(struct siwifi_radar *radar)
{}

static inline bool siwifi_radar_set_domain(struct siwifi_radar *radar,
                                         enum nl80211_dfs_regions region)
{return true;}

static inline void siwifi_radar_detection_enable(struct siwifi_radar *radar,
                                               u8 enable, u8 chain)
{}

static inline bool siwifi_radar_detection_is_enable(struct siwifi_radar *radar,
                                                 u8 chain)
{return false;}

static inline void siwifi_radar_start_cac(struct siwifi_radar *radar,
                                        u32 cac_time_ms, struct siwifi_vif *vif)
{}

static inline void siwifi_radar_cancel_cac(struct siwifi_radar *radar)
{}

static inline void siwifi_radar_detection_enable_on_cur_channel(struct siwifi_hw *siwifi_hw)
{}

static inline int siwifi_radar_dump_pattern_detector(char *buf, size_t len,
                                                   struct siwifi_radar *radar,
                                                   u8 chain)
{return 0;}

static inline int siwifi_radar_dump_radar_detected(char *buf, size_t len,
                                                 struct siwifi_radar *radar,
                                                 u8 chain)
{return 0;}

static inline void siwifi_radar_detected(struct siwifi_hw *siwifi_hw)
{}
#endif /* CONFIG_SIWIFI_RADAR */

#endif // _SIWIFI_RADAR_H_
