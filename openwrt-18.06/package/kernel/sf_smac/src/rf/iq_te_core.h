/**
 ****************************************************************************************
 * @file iq_te_core.h
 *
 * @brief Declarations of functions for iqdump test.
 *
 ****************************************************************************************
 */

#ifndef _IQ_TE_CORE_H_
#define _IQ_TE_CORE_H_

#include <linux/mutex.h>
#include <linux/spinlock.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

//IQ Test engine private data context
struct iq_test_engine_platform_context{
    //device point
    struct device *dev;
    //lock define
    struct mutex iq_te_mtx;
    spinlock_t iq_te_lock;
    //mandatory parameters
    int8_t band_type;  //bit1 set HIGH_BAND, bit0 set LOW_BAND, HIGH_BAND is prior than LOW_BAND, i.e. set 0x3 means HIGH_BAND
    int8_t rp_mode;    //0:record mode 1:player mode
    int8_t over_write; //for record mode only
    int8_t sel_bb;     //0:select Baseband, 1:select RF
    uint32_t iq_offset;
    uint32_t iq_length;
    uint32_t iq_trigger;
    uint32_t op_time; //record/player working time
    int32_t use_dma;
};

extern struct iq_test_engine_platform_context *g_iq_te_pl_ctx;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief this function will init the iq engine, if we use dma to do the continuous mode
 * @params point to global iq_test_engine_platform_context
 * @return 0 init success, otherwise init failed
 *
 ****************************************************************************************
 */
int8_t iq_engine_rp_init(void *);


/**
 ****************************************************************************************
 * @brief this function deinit the iq engine
 * @params point to global iq_test_engine_platform_context
 ****************************************************************************************
 */
void iq_engine_rp_deinit(void *);
/**
 ****************************************************************************************
 * @brief this function will enable the iq test engine to record the iq sample's that from BB to RF
 * @params point to global iq_test_engine_platform_context
 * @please note that once iq engine start must not use la and iram before it stop
 *
 ****************************************************************************************
 */
void iq_engine_record_start(void *);

/**
 ****************************************************************************************
 * @brief this function will stop the iq test engine to record the iq sample's that from BB to RF
 * @params point to global iq_test_engine_platform_context
 ****************************************************************************************
 */
void iq_engine_record_stop(void *);

/**
 ****************************************************************************************
 * @brief this function will enable the iq test engine to play the iq sample's that from RF to BB
 * @params point to global iq_test_engine_platform_context
 * @please note that once iq engine start must not use la and iram before it stop
 ****************************************************************************************
 */
void iq_engine_player_start(void *);

/**
 ****************************************************************************************
 * @brief this function will stop the iq test engine to record the iq sample's that from BB to RF
 * @params point to global iq_test_engine_platform_context
 ****************************************************************************************
 */
void iq_engine_player_stop(void *);

/**
 ****************************************************************************************
 * @brief this function is API for do a iq test engine player flow path once
 * @params point to global iq_test_engine_platform_context
 * @return 0 means success, otherwise  failed
 ****************************************************************************************
 */
int iq_engine_player_once(void *);

/**
 ****************************************************************************************
 * @brief this function is API for do a iq test engine player flow path once
 * @params point to global iq_test_engine_platform_context
 * @return 0 means success, otherwise  failed
 ****************************************************************************************
 */
int iq_engine_player_once(void *);

/**
 ****************************************************************************************
 * @brief this function is API for do a iq test engine record flow path once
 * @params point to global iq_test_engine_platform_context
 * @return 0 means success, otherwise  failed
 ****************************************************************************************
 */
int iq_engine_record_once(void *);

/**
 ****************************************************************************************
 * @brief this function is API for do iram reset
 * @please note that when the iram mode switch back to normal iram
 * it need to be done before cpu access the iram
 ****************************************************************************************
 */
void sys_iram_reset(void);

#endif
