/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_led.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_LED_H
#define __FAL_TIGER_LED_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "yt_led.h"
#include "fal_cmm.h"

/*
 * Macro Declaration
 */
#define LED_GLB_CTRL                    0xd0000
#define LED_CTRL_0_BASE                 0xd0004
#define LED_CTRL_1_BASE                 0xd0040
#define LED_CTRL_2_BASE                 0xd0080
#define LED_SERIAL_CTRL                 0xd0100
#define LED_SERIAL_REMAPPING_BASE       0xd0104
#define LED_PARALLEL_REMAPPING_BASE     0xd01d0
#define LED_PARALLEL_OUTPUT_CTRL        0xd01c4
#define LED_PARALLEL_POS_INVERT_CTRL    0xd01c8


#define LEDDSCP_ON_UNIT(unit)           (UNITINFO(unit)->pLEDDescp)
#define LED_MODE(unit)                  (LEDDSCP_ON_UNIT(unit)->ledMode)
#define SLED_PARAM(unit)                (LEDDSCP_ON_UNIT(unit)->pSledParam)

/**
 * @internal      fal_tiger_led_enable
 * @endinternal
 *
 * @brief         enabel LED
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_enable(yt_unit_t unit);

/**
 * @internal      fal_tiger_led_mode_set
 * @endinternal
 *
 * @brief         select the mode of led
 * @param[in]     unit                -unit id
 * @param[in]     mode                -serial or parallel mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_mode_set(yt_unit_t unit, yt_led_mode_t mode);

/**
 * @internal      fal_tiger_led_mode_get
 * @endinternal
 *
 * @brief         get the mode of led
 * @param[in]     unit                -unit id
 * @param[out]    pMode               -serial or parallel mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_mode_get(yt_unit_t unit, yt_led_mode_t *pMode);

/**
 * @internal      fal_tiger_led_action_set
 * @endinternal
 *
 * @brief         select the action of LED0~2 per port
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     ledActCfg           -config of led action
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_action_set(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_act_cfg_t ledActCfg);

/**
 * @internal      fal_tiger_led_action_get
 * @endinternal
 *
 * @brief         get the action of LED0~2 per port
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     pLedActCfg          -config of led action
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_action_get(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_act_cfg_t *);

/**
 * @internal      fal_tiger_led_blink_freq_set
 * @endinternal
 *
 * @brief         select the frequency of blink
 * @param[in]     unit                -unit id
 * @param[in]     event               -event associated with blink
 * @param[in]     freq                -frequency of blink
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_blink_freq_set(yt_unit_t unit, yt_port_t port, yt_led_blink_event_t event, yt_led_blink_freq_t freq);

/**
 * @internal      fal_tiger_led_blink_freq_get
 * @endinternal
 *
 * @brief         get the frequency of blink
 * @param[in]     unit                -unit id
 * @param[in]     event               -event associated with blink
 * @param[out]    pFreq               -frequency of blink
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_blink_freq_get(yt_unit_t unit, yt_port_t port, yt_led_blink_event_t event, yt_led_blink_freq_t *pFreq);

/**
 * @internal      fal_tiger_led_blink_duty_set
 * @endinternal
 *
 * @brief         select the duty of blink
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     duty                -duty of blink
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_blink_duty_set(yt_unit_t unit, yt_port_t port, yt_led_blink_duty_t duty);

/**
 * @internal      fal_tiger_led_blink_duty_get
 * @endinternal
 *
 * @brief         get the duty of blink
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pDuty               -duty of blink
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_blink_duty_get(yt_unit_t unit, yt_port_t port, yt_led_blink_duty_t *pDuty);

/**
 * @internal      fal_tiger_led_loopdetect_blink_rate_set
 * @endinternal
 *
 * @brief         select blinking rate of loopdetect
 * @param[in]     unit                -unit id
 * @param[in]     rate                -blinking rate
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_loopdetect_blink_rate_set(yt_unit_t unit, yt_led_loopdetect_blink_rate_t rate);

/**
 * @internal      fal_tiger_led_loopdetect_blink_rate_get
 * @endinternal
 *
 * @brief         get blinking rate of loopdetect
 * @param[in]     unit                -unit id
 * @param[out]    pRate               -blinking rate
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_loopdetect_blink_rate_get(yt_unit_t unit, yt_led_loopdetect_blink_rate_t *pRate);

/**
 * @internal      fal_tiger_led_force_mode_set
 * @endinternal
 *
 * @brief         select the cpu force mode
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     mode                -force mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_force_mode_set(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_force_mode_t mode);

/**
 * @internal      fal_tiger_led_force_mode_get
 * @endinternal
 *
 * @brief         get cpu force mode of LED
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[out]    pMode               -force mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_force_mode_get(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_force_mode_t *pMode);

/**
 * @internal      fal_tiger_led_blink_duty_set
 * @endinternal
 *
 * @brief         select the rate of force mode
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     rate                -
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_force_rate_set(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_force_rate_t rate);

/**
 * @internal      fal_tiger_led_force_rate_get
 * @endinternal
 *
 * @brief         get the rate of force mode
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[out]    pRate               -rate
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_force_rate_get(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_led_force_rate_t *pRate);

/**
 * @internal      fal_tiger_led_serial_activeMode_set
 * @endinternal
 *
 * @brief         select the active mode of serial LED
 * @param[in]     unit                -unit id
 * @param[in]     mode                -active mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_activeMode_set(yt_unit_t unit, yt_sled_activeMode_t mode);


/**
 * @internal      fal_tiger_led_serial_activeMode_get
 * @endinternal
 *
 * @brief         get the active mode of serial LED
 * @param[in]     unit                -unit id
 * @param[out]    pMode               -active mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_activeMode_get(yt_unit_t unit, yt_sled_activeMode_t *pMode);

/**
 * @internal      fal_tiger_led_serial_remapping_set
 * @endinternal
 *
 * @brief         select the remapping state of serial LED(dst-->src)
 * @param[in]     unit                -unit id
 * @param[in]     index               -index of led_data
 * @param[in]     dstInfo             -the destination information of remapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_remapping_set(yt_unit_t unit,  uint8_t index, yt_led_remapping_t dstInfo);
/**
 * @internal      fal_tiger_led_serial_remapping_get
 * @endinternal
 *
 * @brief         get the remapping state of serial LED
 * @param[in]     unit                -unit id
 * @param[in]     index               -index of led_data
 * @param[out]    pDstInfo            -the destination information of remapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_remapping_get(yt_unit_t unit, uint8_t index, yt_led_remapping_t *pDstInfo);

/**
 * @internal      fal_tiger_led_serial_enable_set
 * @endinternal
 *
 * @brief         enabel/disable serial LED
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_enable_set(yt_unit_t unit, yt_enable_t enable);
/**
 * @internal      fal_tiger_led_serial_enable_get
 * @endinternal
 *
 * @brief         get enable state of serial LED
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_serial_enable_get(yt_unit_t unit, yt_enable_t *pEnable);

/**
 * @internal      fal_tiger_led_parallel_output_set
 * @endinternal
 *
 * @brief         select output port mask of parallel LED
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_output_set(yt_unit_t unit, yt_port_mask_t port_mask);

/**
 * @internal      fal_tiger_led_parallel_output_get
 * @endinternal
 *
 * @brief         get output port mask of parallel LED
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_output_get(yt_unit_t unit, yt_port_mask_t *pport_mask);

/**
 * @internal      fal_tiger_led_parallel_remapping_set
 * @endinternal
 *
 * @brief         select the remapping state of parallel LED(dst-->src)
 * @param[in]     unit                -unit id
 * @param[in]     srcInfo             -the source information of remapping
 * @param[in]     dstInfo             -the destination information of remapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_remapping_set(yt_unit_t unit, yt_led_remapping_t srcInfo, yt_led_remapping_t dstInfo);

/**
 * @internal      fal_tiger_led_parallel_remapping_get
 * @endinternal
 *
 * @brief         get the remapping state of parallel LED
 * @param[in]     unit                -unit id
 * @param[in]     srcInfo             -the source information of remapping
 * @param[out]    pDstInfo            -the destination information of remapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_remapping_get(yt_unit_t unit, yt_led_remapping_t srcInfo, yt_led_remapping_t *pDstInfo);

/**
 * @internal      fal_tiger_led_parallel_pos_invert_set
 * @endinternal
 *
 * @brief         invert led_pos signal for parallel mode
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_pos_invert_set(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_enable_t enable);

/**
 * @internal      fal_tiger_led_parallel_pos_invert_get
 * @endinternal
 *
 * @brief         get inverted status of led_pos signal
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     ledId               -led id
 * @param[in]     pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_led_parallel_pos_invert_get(yt_unit_t unit, yt_port_t port, yt_led_id_t ledId, yt_enable_t *pEnable);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
