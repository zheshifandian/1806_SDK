/**
 ****************************************************************************************
 *
 * @file siwifi_ipc_emb.h
 *
 * @brief IPC module register definitions
 *
 * Copyright (C) Siflower 2018-2025
 *
 ****************************************************************************************
 */

#ifndef _REG_IPC_EMB_H_
#define _REG_IPC_EMB_H_

#ifndef __KERNEL__
#include <stdint.h>
#include "arch.h"
#else
#include "ipc_compat.h"
#endif
#include "reg_access.h"

#define WIFI_REG_IPC_EMB_BASE_ADDR(band)  (WIFI_BASE_ADDR(band) + 0x000C0000)
/**
 * @name EMB2APP_TRIGGER register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00      EMB2APP_TRIGGER   0x0
 * </pre>
 *
 * @{
 */

/// Address of the EMB2APP_TRIGGER register
#define IPC_EMB2APP_TRIGGER_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000100)
/// Offset of the EMB2APP_TRIGGER register from the base address
#define IPC_EMB2APP_TRIGGER_OFFSET 0x00000000
/// Index of the EMB2APP_TRIGGER register
#define IPC_EMB2APP_TRIGGER_INDEX  0x00000000
/// Reset value of the EMB2APP_TRIGGER register
#define IPC_EMB2APP_TRIGGER_RESET  0x00000000

/**
 * @brief Returns the current value of the EMB2APP_TRIGGER register.
 * The EMB2APP_TRIGGER register will be read and its value returned.
 * @return The current value of the EMB2APP_TRIGGER register.
 */
__INLINE uint32_t ipc_emb2app_trigger_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_EMB2APP_TRIGGER_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the EMB2APP_TRIGGER register to a value.
 * The EMB2APP_TRIGGER register will be written.
 * @param value - The value to write.
 */
#ifndef CONFIG_INLINE_OPT
__INLINE void ipc_emb2app_trigger_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_EMB2APP_TRIGGER_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#else
void ipc_emb2app_trigger_set(uint32_t value);
#endif

// field definitions
/// EMB2APP_TRIGGER field mask
#define IPC_EMB2APP_TRIGGER_MASK   ((uint32_t)0xFFFFFFFF)
/// EMB2APP_TRIGGER field LSB position
#define IPC_EMB2APP_TRIGGER_LSB    0
/// EMB2APP_TRIGGER field width
#define IPC_EMB2APP_TRIGGER_WIDTH  ((uint32_t)0x00000020)

/// EMB2APP_TRIGGER field reset value
#define IPC_EMB2APP_TRIGGER_RST    0x0

/**
 * @brief Returns the current value of the EMB2APP_TRIGGER field in the EMB2APP_TRIGGER register.
 *
 * The EMB2APP_TRIGGER register will be read and the EMB2APP_TRIGGER field's value will be returned.
 *
 * @return The current value of the EMB2APP_TRIGGER field in the EMB2APP_TRIGGER register.
 */
__INLINE uint32_t ipc_emb2app_trigger_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_EMB2APP_TRIGGER_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/**
 * @brief Sets the EMB2APP_TRIGGER field of the EMB2APP_TRIGGER register.
 *
 * The EMB2APP_TRIGGER register will be read, modified to contain the new field value, and written.
 *
 * @param[in] emb2apptrigger - The value to set the field to.
 */
__INLINE void ipc_emb2app_trigger_setf(struct siwifi_hw *siwifi_hw, uint32_t emb2apptrigger)
{
	ASSERT_ERR((((uint32_t)emb2apptrigger << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(IPC_EMB2APP_TRIGGER_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)emb2apptrigger << 0);
}

/// @}

/**
 * @name APP2EMB_RAWSTATUS register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    APP2EMB_RAWSTATUS   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_RAWSTATUS register
#define IPC_APP2EMB_RAWSTATUS_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000104)
/// Offset of the APP2EMB_RAWSTATUS register from the base address
#define IPC_APP2EMB_RAWSTATUS_OFFSET 0x00000004
/// Index of the APP2EMB_RAWSTATUS register
#define IPC_APP2EMB_RAWSTATUS_INDEX  0x00000001
/// Reset value of the APP2EMB_RAWSTATUS register
#define IPC_APP2EMB_RAWSTATUS_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_RAWSTATUS register.
 * The APP2EMB_RAWSTATUS register will be read and its value returned.
 * @return The current value of the APP2EMB_RAWSTATUS register.
 */
__INLINE uint32_t ipc_app2emb_rawstatus_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_RAWSTATUS_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// APP2EMB_RAWSTATUS field mask
#define IPC_APP2EMB_RAWSTATUS_MASK   ((uint32_t)0xFFFFFFFF)
/// APP2EMB_RAWSTATUS field LSB position
#define IPC_APP2EMB_RAWSTATUS_LSB    0
/// APP2EMB_RAWSTATUS field width
#define IPC_APP2EMB_RAWSTATUS_WIDTH  ((uint32_t)0x00000020)

/// APP2EMB_RAWSTATUS field reset value
#define IPC_APP2EMB_RAWSTATUS_RST    0x0

/**
 * @brief Returns the current value of the APP2EMB_RAWSTATUS field in the APP2EMB_RAWSTATUS register.
 *
 * The APP2EMB_RAWSTATUS register will be read and the APP2EMB_RAWSTATUS field's value will be returned.
 *
 * @return The current value of the APP2EMB_RAWSTATUS field in the APP2EMB_RAWSTATUS register.
 */
__INLINE uint32_t ipc_app2emb_rawstatus_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_RAWSTATUS_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/// @}

/**
 * @name APP2EMB_ACK register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00          APP2EMB_ACK   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_ACK register
#define IPC_APP2EMB_ACK_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000108)
/// Offset of the APP2EMB_ACK register from the base address
#define IPC_APP2EMB_ACK_OFFSET 0x00000008
/// Index of the APP2EMB_ACK register
#define IPC_APP2EMB_ACK_INDEX  0x00000002
/// Reset value of the APP2EMB_ACK register
#define IPC_APP2EMB_ACK_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_ACK register.
 * The APP2EMB_ACK register will be read and its value returned.
 * @return The current value of the APP2EMB_ACK register.
 */
__INLINE uint32_t ipc_app2emb_ack_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_ACK_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the APP2EMB_ACK register to a value.
 * The APP2EMB_ACK register will be written.
 * @param value - The value to write.
 */

#ifndef CONFIG_INLINE_OPT
__INLINE void ipc_app2emb_ack_clear(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_ACK_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#else
void ipc_app2emb_ack_clear(uint32_t value);
#endif

// field definitions
/// APP2EMB_ACK field mask
#define IPC_APP2EMB_ACK_MASK   ((uint32_t)0xFFFFFFFF)
/// APP2EMB_ACK field LSB position
#define IPC_APP2EMB_ACK_LSB    0
/// APP2EMB_ACK field width
#define IPC_APP2EMB_ACK_WIDTH  ((uint32_t)0x00000020)

/// APP2EMB_ACK field reset value
#define IPC_APP2EMB_ACK_RST    0x0

/**
 * @brief Returns the current value of the APP2EMB_ACK field in the APP2EMB_ACK register.
 *
 * The APP2EMB_ACK register will be read and the APP2EMB_ACK field's value will be returned.
 *
 * @return The current value of the APP2EMB_ACK field in the APP2EMB_ACK register.
 */
__INLINE uint32_t ipc_app2emb_ack_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_ACK_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/**
 * @brief Sets the APP2EMB_ACK field of the APP2EMB_ACK register.
 *
 * The APP2EMB_ACK register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emback - The value to set the field to.
 */
__INLINE void ipc_app2emb_ack_clearf(struct siwifi_hw *siwifi_hw, uint32_t app2emback)
{
	ASSERT_ERR((((uint32_t)app2emback << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(IPC_APP2EMB_ACK_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)app2emback << 0);
}

/// @}

/**
 * @name APP2EMB_UNMASK_SET register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00       APP2EMB_UNMASK   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_UNMASK_SET register
#define IPC_APP2EMB_UNMASK_SET_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x0000010C)
/// Offset of the APP2EMB_UNMASK_SET register from the base address
#define IPC_APP2EMB_UNMASK_SET_OFFSET 0x0000000C
/// Index of the APP2EMB_UNMASK_SET register
#define IPC_APP2EMB_UNMASK_SET_INDEX  0x00000003
/// Reset value of the APP2EMB_UNMASK_SET register
#define IPC_APP2EMB_UNMASK_SET_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_UNMASK_SET register.
 * The APP2EMB_UNMASK_SET register will be read and its value returned.
 * @return The current value of the APP2EMB_UNMASK_SET register.
 */
__INLINE uint32_t ipc_app2emb_unmask_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_UNMASK_SET_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the APP2EMB_UNMASK_SET register to a value.
 * The APP2EMB_UNMASK_SET register will be written.
 * @param value - The value to write.
 */

#ifndef CONFIG_INLINE_OPT
__INLINE void ipc_app2emb_unmask_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_UNMASK_SET_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#else
void ipc_app2emb_unmask_set(uint32_t value);
#endif

// field definitions
/// APP2EMB_UNMASK field mask
#define IPC_APP2EMB_UNMASK_MASK   ((uint32_t)0xFFFFFFFF)
/// APP2EMB_UNMASK field LSB position
#define IPC_APP2EMB_UNMASK_LSB    0
/// APP2EMB_UNMASK field width
#define IPC_APP2EMB_UNMASK_WIDTH  ((uint32_t)0x00000020)

/// APP2EMB_UNMASK field reset value
#define IPC_APP2EMB_UNMASK_RST    0x0

/**
 * @brief Returns the current value of the APP2EMB_UNMASK field in the APP2EMB_UNMASK_SET register.
 *
 * The APP2EMB_UNMASK_SET register will be read and the APP2EMB_UNMASK field's value will be returned.
 *
 * @return The current value of the APP2EMB_UNMASK field in the APP2EMB_UNMASK_SET register.
 */
__INLINE uint32_t ipc_app2emb_unmask_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_UNMASK_SET_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/**
 * @brief Sets the APP2EMB_UNMASK field of the APP2EMB_UNMASK_SET register.
 *
 * The APP2EMB_UNMASK_SET register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2embunmask - The value to set the field to.
 */
__INLINE void ipc_app2emb_unmask_setf(struct siwifi_hw *siwifi_hw, uint32_t app2embunmask)
{
	ASSERT_ERR((((uint32_t)app2embunmask << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(IPC_APP2EMB_UNMASK_SET_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)app2embunmask << 0);
}

/// @}

/**
 * @name APP2EMB_UNMASK_CLEAR register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00       APP2EMB_UNMASK   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_UNMASK_CLEAR register
#define IPC_APP2EMB_UNMASK_CLEAR_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000110)
/// Offset of the APP2EMB_UNMASK_CLEAR register from the base address
#define IPC_APP2EMB_UNMASK_CLEAR_OFFSET 0x00000010
/// Index of the APP2EMB_UNMASK_CLEAR register
#define IPC_APP2EMB_UNMASK_CLEAR_INDEX  0x00000004
/// Reset value of the APP2EMB_UNMASK_CLEAR register
#define IPC_APP2EMB_UNMASK_CLEAR_RESET  0x00000000

/**
 * @brief Sets the APP2EMB_UNMASK_CLEAR register to a value.
 * The APP2EMB_UNMASK_CLEAR register will be written.
 * @param value - The value to write.
 */
#ifndef CONFIG_INLINE_OPT
__INLINE void ipc_app2emb_unmask_clear(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_UNMASK_CLEAR_ADDR(siwifi_hw->mod_params->is_hb), value);
}
#else
void ipc_app2emb_unmask_clear(uint32_t value);
#endif

// fields defined in symmetrical set/clear register
/**
 * @brief Sets the APP2EMB_UNMASK field of the APP2EMB_UNMASK_CLEAR register.
 *
 * The APP2EMB_UNMASK_CLEAR register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2embunmask - The value to set the field to.
 */
__INLINE void ipc_app2emb_unmask_clearf(struct siwifi_hw *siwifi_hw, uint32_t app2embunmask)
{
	ASSERT_ERR((((uint32_t)app2embunmask << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(IPC_APP2EMB_UNMASK_CLEAR_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)app2embunmask << 0);
}

/// @}

/**
 * @name APP2EMB_LINE_SEL register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:30        APP2EMB15_SEL   0x0
 *  29:28        APP2EMB14_SEL   0x0
 *  27:26        APP2EMB13_SEL   0x0
 *  25:24        APP2EMB12_SEL   0x0
 *  23:22        APP2EMB11_SEL   0x0
 *  21:20        APP2EMB10_SEL   0x0
 *  19:18         APP2EMB9_SEL   0x0
 *  17:16         APP2EMB8_SEL   0x0
 *  15:14         APP2EMB7_SEL   0x0
 *  13:12         APP2EMB6_SEL   0x0
 *  11:10         APP2EMB5_SEL   0x0
 *  09:08         APP2EMB4_SEL   0x0
 *  07:06         APP2EMB3_SEL   0x0
 *  05:04         APP2EMB2_SEL   0x0
 *  03:02         APP2EMB1_SEL   0x0
 *  01:00         APP2EMB0_SEL   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_LINE_SEL register
#define IPC_APP2EMB_LINE_SEL_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000114)
/// Offset of the APP2EMB_LINE_SEL register from the base address
#define IPC_APP2EMB_LINE_SEL_OFFSET 0x00000014
/// Index of the APP2EMB_LINE_SEL register
#define IPC_APP2EMB_LINE_SEL_INDEX  0x00000005
/// Reset value of the APP2EMB_LINE_SEL register
#define IPC_APP2EMB_LINE_SEL_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_LINE_SEL register.
 * The APP2EMB_LINE_SEL register will be read and its value returned.
 * @return The current value of the APP2EMB_LINE_SEL register.
 */
__INLINE uint32_t ipc_app2emb_line_sel_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the APP2EMB_LINE_SEL register to a value.
 * The APP2EMB_LINE_SEL register will be written.
 * @param value - The value to write.
 */
__INLINE void ipc_app2emb_line_sel_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// APP2EMB15_SEL field mask
#define IPC_APP2EMB15_SEL_MASK   ((uint32_t)0xC0000000)
/// APP2EMB15_SEL field LSB position
#define IPC_APP2EMB15_SEL_LSB    30
/// APP2EMB15_SEL field width
#define IPC_APP2EMB15_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB14_SEL field mask
#define IPC_APP2EMB14_SEL_MASK   ((uint32_t)0x30000000)
/// APP2EMB14_SEL field LSB position
#define IPC_APP2EMB14_SEL_LSB    28
/// APP2EMB14_SEL field width
#define IPC_APP2EMB14_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB13_SEL field mask
#define IPC_APP2EMB13_SEL_MASK   ((uint32_t)0x0C000000)
/// APP2EMB13_SEL field LSB position
#define IPC_APP2EMB13_SEL_LSB    26
/// APP2EMB13_SEL field width
#define IPC_APP2EMB13_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB12_SEL field mask
#define IPC_APP2EMB12_SEL_MASK   ((uint32_t)0x03000000)
/// APP2EMB12_SEL field LSB position
#define IPC_APP2EMB12_SEL_LSB    24
/// APP2EMB12_SEL field width
#define IPC_APP2EMB12_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB11_SEL field mask
#define IPC_APP2EMB11_SEL_MASK   ((uint32_t)0x00C00000)
/// APP2EMB11_SEL field LSB position
#define IPC_APP2EMB11_SEL_LSB    22
/// APP2EMB11_SEL field width
#define IPC_APP2EMB11_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB10_SEL field mask
#define IPC_APP2EMB10_SEL_MASK   ((uint32_t)0x00300000)
/// APP2EMB10_SEL field LSB position
#define IPC_APP2EMB10_SEL_LSB    20
/// APP2EMB10_SEL field width
#define IPC_APP2EMB10_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB9_SEL field mask
#define IPC_APP2EMB9_SEL_MASK    ((uint32_t)0x000C0000)
/// APP2EMB9_SEL field LSB position
#define IPC_APP2EMB9_SEL_LSB     18
/// APP2EMB9_SEL field width
#define IPC_APP2EMB9_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB8_SEL field mask
#define IPC_APP2EMB8_SEL_MASK    ((uint32_t)0x00030000)
/// APP2EMB8_SEL field LSB position
#define IPC_APP2EMB8_SEL_LSB     16
/// APP2EMB8_SEL field width
#define IPC_APP2EMB8_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB7_SEL field mask
#define IPC_APP2EMB7_SEL_MASK    ((uint32_t)0x0000C000)
/// APP2EMB7_SEL field LSB position
#define IPC_APP2EMB7_SEL_LSB     14
/// APP2EMB7_SEL field width
#define IPC_APP2EMB7_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB6_SEL field mask
#define IPC_APP2EMB6_SEL_MASK    ((uint32_t)0x00003000)
/// APP2EMB6_SEL field LSB position
#define IPC_APP2EMB6_SEL_LSB     12
/// APP2EMB6_SEL field width
#define IPC_APP2EMB6_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB5_SEL field mask
#define IPC_APP2EMB5_SEL_MASK    ((uint32_t)0x00000C00)
/// APP2EMB5_SEL field LSB position
#define IPC_APP2EMB5_SEL_LSB     10
/// APP2EMB5_SEL field width
#define IPC_APP2EMB5_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB4_SEL field mask
#define IPC_APP2EMB4_SEL_MASK    ((uint32_t)0x00000300)
/// APP2EMB4_SEL field LSB position
#define IPC_APP2EMB4_SEL_LSB     8
/// APP2EMB4_SEL field width
#define IPC_APP2EMB4_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB3_SEL field mask
#define IPC_APP2EMB3_SEL_MASK    ((uint32_t)0x000000C0)
/// APP2EMB3_SEL field LSB position
#define IPC_APP2EMB3_SEL_LSB     6
/// APP2EMB3_SEL field width
#define IPC_APP2EMB3_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB2_SEL field mask
#define IPC_APP2EMB2_SEL_MASK    ((uint32_t)0x00000030)
/// APP2EMB2_SEL field LSB position
#define IPC_APP2EMB2_SEL_LSB     4
/// APP2EMB2_SEL field width
#define IPC_APP2EMB2_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB1_SEL field mask
#define IPC_APP2EMB1_SEL_MASK    ((uint32_t)0x0000000C)
/// APP2EMB1_SEL field LSB position
#define IPC_APP2EMB1_SEL_LSB     2
/// APP2EMB1_SEL field width
#define IPC_APP2EMB1_SEL_WIDTH   ((uint32_t)0x00000002)
/// APP2EMB0_SEL field mask
#define IPC_APP2EMB0_SEL_MASK    ((uint32_t)0x00000003)
/// APP2EMB0_SEL field LSB position
#define IPC_APP2EMB0_SEL_LSB     0
/// APP2EMB0_SEL field width
#define IPC_APP2EMB0_SEL_WIDTH   ((uint32_t)0x00000002)

/// APP2EMB15_SEL field reset value
#define IPC_APP2EMB15_SEL_RST    0x0
/// APP2EMB14_SEL field reset value
#define IPC_APP2EMB14_SEL_RST    0x0
/// APP2EMB13_SEL field reset value
#define IPC_APP2EMB13_SEL_RST    0x0
/// APP2EMB12_SEL field reset value
#define IPC_APP2EMB12_SEL_RST    0x0
/// APP2EMB11_SEL field reset value
#define IPC_APP2EMB11_SEL_RST    0x0
/// APP2EMB10_SEL field reset value
#define IPC_APP2EMB10_SEL_RST    0x0
/// APP2EMB9_SEL field reset value
#define IPC_APP2EMB9_SEL_RST     0x0
/// APP2EMB8_SEL field reset value
#define IPC_APP2EMB8_SEL_RST     0x0
/// APP2EMB7_SEL field reset value
#define IPC_APP2EMB7_SEL_RST     0x0
/// APP2EMB6_SEL field reset value
#define IPC_APP2EMB6_SEL_RST     0x0
/// APP2EMB5_SEL field reset value
#define IPC_APP2EMB5_SEL_RST     0x0
/// APP2EMB4_SEL field reset value
#define IPC_APP2EMB4_SEL_RST     0x0
/// APP2EMB3_SEL field reset value
#define IPC_APP2EMB3_SEL_RST     0x0
/// APP2EMB2_SEL field reset value
#define IPC_APP2EMB2_SEL_RST     0x0
/// APP2EMB1_SEL field reset value
#define IPC_APP2EMB1_SEL_RST     0x0
/// APP2EMB0_SEL field reset value
#define IPC_APP2EMB0_SEL_RST     0x0

/**
 * @brief Constructs a value for the APP2EMB_LINE_SEL register given values for its fields
 * and writes the value to the register.
 *
 * @param[in] app2emb15sel - The value to use for the APP2EMB15_SEL field.
 * @param[in] app2emb14sel - The value to use for the APP2EMB14_SEL field.
 * @param[in] app2emb13sel - The value to use for the APP2EMB13_SEL field.
 * @param[in] app2emb12sel - The value to use for the APP2EMB12_SEL field.
 * @param[in] app2emb11sel - The value to use for the APP2EMB11_SEL field.
 * @param[in] app2emb10sel - The value to use for the APP2EMB10_SEL field.
 * @param[in] app2emb9sel - The value to use for the APP2EMB9_SEL field.
 * @param[in] app2emb8sel - The value to use for the APP2EMB8_SEL field.
 * @param[in] app2emb7sel - The value to use for the APP2EMB7_SEL field.
 * @param[in] app2emb6sel - The value to use for the APP2EMB6_SEL field.
 * @param[in] app2emb5sel - The value to use for the APP2EMB5_SEL field.
 * @param[in] app2emb4sel - The value to use for the APP2EMB4_SEL field.
 * @param[in] app2emb3sel - The value to use for the APP2EMB3_SEL field.
 * @param[in] app2emb2sel - The value to use for the APP2EMB2_SEL field.
 * @param[in] app2emb1sel - The value to use for the APP2EMB1_SEL field.
 * @param[in] app2emb0sel - The value to use for the APP2EMB0_SEL field.
 */
__INLINE void ipc_app2emb_line_sel_pack(struct siwifi_hw *siwifi_hw, uint8_t app2emb15sel, uint8_t app2emb14sel, uint8_t app2emb13sel, uint8_t app2emb12sel, uint8_t app2emb11sel, uint8_t app2emb10sel, uint8_t app2emb9sel, uint8_t app2emb8sel, uint8_t app2emb7sel, uint8_t app2emb6sel, uint8_t app2emb5sel, uint8_t app2emb4sel, uint8_t app2emb3sel, uint8_t app2emb2sel, uint8_t app2emb1sel, uint8_t app2emb0sel)
{
	ASSERT_ERR((((uint32_t)app2emb15sel << 30) & ~((uint32_t)0xC0000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb14sel << 28) & ~((uint32_t)0x30000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb13sel << 26) & ~((uint32_t)0x0C000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb12sel << 24) & ~((uint32_t)0x03000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb11sel << 22) & ~((uint32_t)0x00C00000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb10sel << 20) & ~((uint32_t)0x00300000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb9sel << 18) & ~((uint32_t)0x000C0000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb8sel << 16) & ~((uint32_t)0x00030000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb7sel << 14) & ~((uint32_t)0x0000C000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb6sel << 12) & ~((uint32_t)0x00003000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb5sel << 10) & ~((uint32_t)0x00000C00)) == 0);
	ASSERT_ERR((((uint32_t)app2emb4sel << 8) & ~((uint32_t)0x00000300)) == 0);
	ASSERT_ERR((((uint32_t)app2emb3sel << 6) & ~((uint32_t)0x000000C0)) == 0);
	ASSERT_ERR((((uint32_t)app2emb2sel << 4) & ~((uint32_t)0x00000030)) == 0);
	ASSERT_ERR((((uint32_t)app2emb1sel << 2) & ~((uint32_t)0x0000000C)) == 0);
	ASSERT_ERR((((uint32_t)app2emb0sel << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb),  ((uint32_t)app2emb15sel << 30) | ((uint32_t)app2emb14sel << 28) | ((uint32_t)app2emb13sel << 26) | ((uint32_t)app2emb12sel << 24) | ((uint32_t)app2emb11sel << 22) | ((uint32_t)app2emb10sel << 20) | ((uint32_t)app2emb9sel << 18) | ((uint32_t)app2emb8sel << 16) | ((uint32_t)app2emb7sel << 14) | ((uint32_t)app2emb6sel << 12) | ((uint32_t)app2emb5sel << 10) | ((uint32_t)app2emb4sel << 8) | ((uint32_t)app2emb3sel << 6) | ((uint32_t)app2emb2sel << 4) | ((uint32_t)app2emb1sel << 2) | ((uint32_t)app2emb0sel << 0));
}

/**
 * @brief Unpacks APP2EMB_LINE_SEL's fields from current value of the APP2EMB_LINE_SEL register.
 *
 * Reads the APP2EMB_LINE_SEL register and populates all the _field variables with the corresponding
 * values from the register.
 *
 * @param[out] app2emb15sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb14sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb13sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb12sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb11sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb10sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb9sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb8sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb7sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb6sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb5sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb4sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb3sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb2sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb1sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb0sel - Will be populated with the current value of this field from the register.
 */
__INLINE void ipc_app2emb_line_sel_unpack(struct siwifi_hw *siwifi_hw, uint8_t* app2emb15sel, uint8_t* app2emb14sel, uint8_t* app2emb13sel, uint8_t* app2emb12sel, uint8_t* app2emb11sel, uint8_t* app2emb10sel, uint8_t* app2emb9sel, uint8_t* app2emb8sel, uint8_t* app2emb7sel, uint8_t* app2emb6sel, uint8_t* app2emb5sel, uint8_t* app2emb4sel, uint8_t* app2emb3sel, uint8_t* app2emb2sel, uint8_t* app2emb1sel, uint8_t* app2emb0sel)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));

	*app2emb15sel = (localVal & ((uint32_t)0xC0000000)) >> 30;
	*app2emb14sel = (localVal & ((uint32_t)0x30000000)) >> 28;
	*app2emb13sel = (localVal & ((uint32_t)0x0C000000)) >> 26;
	*app2emb12sel = (localVal & ((uint32_t)0x03000000)) >> 24;
	*app2emb11sel = (localVal & ((uint32_t)0x00C00000)) >> 22;
	*app2emb10sel = (localVal & ((uint32_t)0x00300000)) >> 20;
	*app2emb9sel = (localVal & ((uint32_t)0x000C0000)) >> 18;
	*app2emb8sel = (localVal & ((uint32_t)0x00030000)) >> 16;
	*app2emb7sel = (localVal & ((uint32_t)0x0000C000)) >> 14;
	*app2emb6sel = (localVal & ((uint32_t)0x00003000)) >> 12;
	*app2emb5sel = (localVal & ((uint32_t)0x00000C00)) >> 10;
	*app2emb4sel = (localVal & ((uint32_t)0x00000300)) >> 8;
	*app2emb3sel = (localVal & ((uint32_t)0x000000C0)) >> 6;
	*app2emb2sel = (localVal & ((uint32_t)0x00000030)) >> 4;
	*app2emb1sel = (localVal & ((uint32_t)0x0000000C)) >> 2;
	*app2emb0sel = (localVal & ((uint32_t)0x00000003)) >> 0;
}

/**
 * @brief Returns the current value of the APP2EMB15_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB15_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB15_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb15_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0xC0000000)) >> 30);
}

/**
 * @brief Sets the APP2EMB15_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb15sel - The value to set the field to.
 */
__INLINE void ipc_app2emb15_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb15sel)
{
	ASSERT_ERR((((uint32_t)app2emb15sel << 30) & ~((uint32_t)0xC0000000)) == 0);
	REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0xC0000000)) | ((uint32_t)app2emb15sel << 30));
}

/**
 * @brief Returns the current value of the APP2EMB14_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB14_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB14_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb14_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x30000000)) >> 28);
}

/**
 * @brief Sets the APP2EMB14_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb14sel - The value to set the field to.
 */
__INLINE void ipc_app2emb14_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb14sel)
{
	ASSERT_ERR((((uint32_t)app2emb14sel << 28) & ~((uint32_t)0x30000000)) == 0);
	REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x30000000)) | ((uint32_t)app2emb14sel << 28));
}

/**
 * @brief Returns the current value of the APP2EMB13_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB13_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB13_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb13_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0C000000)) >> 26);
}

/**
 * @brief Sets the APP2EMB13_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb13sel - The value to set the field to.
 */
__INLINE void ipc_app2emb13_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb13sel)
{
	ASSERT_ERR((((uint32_t)app2emb13sel << 26) & ~((uint32_t)0x0C000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0C000000)) | ((uint32_t)app2emb13sel << 26));
}

/**
 * @brief Returns the current value of the APP2EMB12_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB12_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB12_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb12_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x03000000)) >> 24);
}

/**
 * @brief Sets the APP2EMB12_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb12sel - The value to set the field to.
 */
__INLINE void ipc_app2emb12_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb12sel)
{
	ASSERT_ERR((((uint32_t)app2emb12sel << 24) & ~((uint32_t)0x03000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x03000000)) | ((uint32_t)app2emb12sel << 24));
}

/**
 * @brief Returns the current value of the APP2EMB11_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB11_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB11_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb11_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00C00000)) >> 22);
}

/**
 * @brief Sets the APP2EMB11_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb11sel - The value to set the field to.
 */
__INLINE void ipc_app2emb11_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb11sel)
{
	ASSERT_ERR((((uint32_t)app2emb11sel << 22) & ~((uint32_t)0x00C00000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00C00000)) | ((uint32_t)app2emb11sel << 22));
}

/**
 * @brief Returns the current value of the APP2EMB10_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB10_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB10_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb10_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00300000)) >> 20);
}

/**
 * @brief Sets the APP2EMB10_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb10sel - The value to set the field to.
 */
__INLINE void ipc_app2emb10_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb10sel)
{
	ASSERT_ERR((((uint32_t)app2emb10sel << 20) & ~((uint32_t)0x00300000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00300000)) | ((uint32_t)app2emb10sel << 20));
}

/**
 * @brief Returns the current value of the APP2EMB9_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB9_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB9_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb9_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x000C0000)) >> 18);
}

/**
 * @brief Sets the APP2EMB9_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb9sel - The value to set the field to.
 */
__INLINE void ipc_app2emb9_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb9sel)
{
	ASSERT_ERR((((uint32_t)app2emb9sel << 18) & ~((uint32_t)0x000C0000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000C0000)) | ((uint32_t)app2emb9sel << 18));
}

/**
 * @brief Returns the current value of the APP2EMB8_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB8_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB8_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb8_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00030000)) >> 16);
}

/**
 * @brief Sets the APP2EMB8_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb8sel - The value to set the field to.
 */
__INLINE void ipc_app2emb8_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb8sel)
{
	ASSERT_ERR((((uint32_t)app2emb8sel << 16) & ~((uint32_t)0x00030000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00030000)) | ((uint32_t)app2emb8sel << 16));
}

/**
 * @brief Returns the current value of the APP2EMB7_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB7_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB7_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb7_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0000C000)) >> 14);
}

/**
 * @brief Sets the APP2EMB7_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb7sel - The value to set the field to.
 */
__INLINE void ipc_app2emb7_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb7sel)
{
	ASSERT_ERR((((uint32_t)app2emb7sel << 14) & ~((uint32_t)0x0000C000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0000C000)) | ((uint32_t)app2emb7sel << 14));
}

/**
 * @brief Returns the current value of the APP2EMB6_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB6_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB6_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb6_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00003000)) >> 12);
}

/**
 * @brief Sets the APP2EMB6_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb6sel - The value to set the field to.
 */
__INLINE void ipc_app2emb6_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb6sel)
{
	ASSERT_ERR((((uint32_t)app2emb6sel << 12) & ~((uint32_t)0x00003000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00003000)) | ((uint32_t)app2emb6sel << 12));
}

/**
 * @brief Returns the current value of the APP2EMB5_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB5_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB5_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb5_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000C00)) >> 10);
}

/**
 * @brief Sets the APP2EMB5_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb5sel - The value to set the field to.
 */
__INLINE void ipc_app2emb5_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb5sel)
{
	ASSERT_ERR((((uint32_t)app2emb5sel << 10) & ~((uint32_t)0x00000C00)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000C00)) | ((uint32_t)app2emb5sel << 10));
}

/**
 * @brief Returns the current value of the APP2EMB4_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB4_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB4_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb4_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000300)) >> 8);
}

/**
 * @brief Sets the APP2EMB4_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb4sel - The value to set the field to.
 */
__INLINE void ipc_app2emb4_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb4sel)
{
	ASSERT_ERR((((uint32_t)app2emb4sel << 8) & ~((uint32_t)0x00000300)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000300)) | ((uint32_t)app2emb4sel << 8));
}

/**
 * @brief Returns the current value of the APP2EMB3_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB3_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB3_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb3_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x000000C0)) >> 6);
}

/**
 * @brief Sets the APP2EMB3_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb3sel - The value to set the field to.
 */
__INLINE void ipc_app2emb3_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb3sel)
{
	ASSERT_ERR((((uint32_t)app2emb3sel << 6) & ~((uint32_t)0x000000C0)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000000C0)) | ((uint32_t)app2emb3sel << 6));
}

/**
 * @brief Returns the current value of the APP2EMB2_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB2_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB2_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb2_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000030)) >> 4);
}

/**
 * @brief Sets the APP2EMB2_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb2sel - The value to set the field to.
 */
__INLINE void ipc_app2emb2_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb2sel)
{
	ASSERT_ERR((((uint32_t)app2emb2sel << 4) & ~((uint32_t)0x00000030)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000030)) | ((uint32_t)app2emb2sel << 4));
}

/**
 * @brief Returns the current value of the APP2EMB1_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB1_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB1_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb1_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0000000C)) >> 2);
}

/**
 * @brief Sets the APP2EMB1_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb1sel - The value to set the field to.
 */
__INLINE void ipc_app2emb1_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb1sel)
{
	ASSERT_ERR((((uint32_t)app2emb1sel << 2) & ~((uint32_t)0x0000000C)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0000000C)) | ((uint32_t)app2emb1sel << 2));
}

/**
 * @brief Returns the current value of the APP2EMB0_SEL field in the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read and the APP2EMB0_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB0_SEL field in the APP2EMB_LINE_SEL register.
 */
__INLINE uint8_t ipc_app2emb0_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000003)) >> 0);
}

/**
 * @brief Sets the APP2EMB0_SEL field of the APP2EMB_LINE_SEL register.
 *
 * The APP2EMB_LINE_SEL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb0sel - The value to set the field to.
 */
__INLINE void ipc_app2emb0_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb0sel)
{
	ASSERT_ERR((((uint32_t)app2emb0sel << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000003)) | ((uint32_t)app2emb0sel << 0));
}

/// @}
/// Address of the APP2EMB_LINE_SEL_LOW register
#define IPC_APP2EMB_LINE_SEL_LOW_ADDR(band)     (WIFI_REG_IPC_EMB_BASE_ADDR(band)  + 0x00000114)
/// Offset of the APP2EMB_LINE_SEL_LOW register from the base address
#define IPC_APP2EMB_LINE_SEL_LOW_OFFSET 0x00000014
/// Index of the APP2EMB_LINE_SEL_LOW register
#define IPC_APP2EMB_LINE_SEL_LOW_INDEX  0x00000005
/// Reset value of the APP2EMB_LINE_SEL_LOW register
#define IPC_APP2EMB_LINE_SEL_LOW_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_LINE_SEL_LOW register.
 * The APP2EMB_LINE_SEL_LOW register will be read and its value returned.
 * @return The current value of the APP2EMB_LINE_SEL_LOW register.
 */
__INLINE uint32_t ipc_app2emb_line_sel_low_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_LINE_SEL_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the APP2EMB_LINE_SEL_LOW register to a value.
 * The APP2EMB_LINE_SEL_LOW register will be written.
 * @param value - The value to write.
 */
__INLINE void ipc_app2emb_line_sel_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_LINE_SEL_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}


/**
 * @name APP2EMB_LINE_SEL_HIGH register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:30        APP2EMB31_SEL   0x0
 *  29:28        APP2EMB30_SEL   0x0
 *  27:26        APP2EMB29_SEL   0x0
 *  25:24        APP2EMB28_SEL   0x0
 *  23:22        APP2EMB27_SEL   0x0
 *  21:20        APP2EMB26_SEL   0x0
 *  19:18        APP2EMB25_SEL   0x0
 *  17:16        APP2EMB24_SEL   0x0
 *  15:14        APP2EMB23_SEL   0x0
 *  13:12        APP2EMB22_SEL   0x0
 *  11:10        APP2EMB21_SEL   0x0
 *  09:08        APP2EMB20_SEL   0x0
 *  07:06        APP2EMB19_SEL   0x0
 *  05:04        APP2EMB18_SEL   0x0
 *  03:02        APP2EMB17_SEL   0x0
 *  01:00        APP2EMB16_SEL   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_LINE_SEL_HIGH register
#define IPC_APP2EMB_LINE_SEL_HIGH_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000118)
/// Offset of the APP2EMB_LINE_SEL_HIGH register from the base address
#define IPC_APP2EMB_LINE_SEL_HIGH_OFFSET 0x00000018
/// Index of the APP2EMB_LINE_SEL_HIGH register
#define IPC_APP2EMB_LINE_SEL_HIGH_INDEX  0x00000006
/// Reset value of the APP2EMB_LINE_SEL_HIGH register
#define IPC_APP2EMB_LINE_SEL_HIGH_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_LINE_SEL_HIGH register.
 * The APP2EMB_LINE_SEL_HIGH register will be read and its value returned.
 * @return The current value of the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint32_t ipc_app2emb_line_sel_high_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the APP2EMB_LINE_SEL_HIGH register to a value.
 * The APP2EMB_LINE_SEL_HIGH register will be written.
 * @param value - The value to write.
 */
__INLINE void ipc_app2emb_line_sel_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
	REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// APP2EMB31_SEL field mask
#define IPC_APP2EMB31_SEL_MASK   ((uint32_t)0xC0000000)
/// APP2EMB31_SEL field LSB position
#define IPC_APP2EMB31_SEL_LSB    30
/// APP2EMB31_SEL field width
#define IPC_APP2EMB31_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB30_SEL field mask
#define IPC_APP2EMB30_SEL_MASK   ((uint32_t)0x30000000)
/// APP2EMB30_SEL field LSB position
#define IPC_APP2EMB30_SEL_LSB    28
/// APP2EMB30_SEL field width
#define IPC_APP2EMB30_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB29_SEL field mask
#define IPC_APP2EMB29_SEL_MASK   ((uint32_t)0x0C000000)
/// APP2EMB29_SEL field LSB position
#define IPC_APP2EMB29_SEL_LSB    26
/// APP2EMB29_SEL field width
#define IPC_APP2EMB29_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB28_SEL field mask
#define IPC_APP2EMB28_SEL_MASK   ((uint32_t)0x03000000)
/// APP2EMB28_SEL field LSB position
#define IPC_APP2EMB28_SEL_LSB    24
/// APP2EMB28_SEL field width
#define IPC_APP2EMB28_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB27_SEL field mask
#define IPC_APP2EMB27_SEL_MASK   ((uint32_t)0x00C00000)
/// APP2EMB27_SEL field LSB position
#define IPC_APP2EMB27_SEL_LSB    22
/// APP2EMB27_SEL field width
#define IPC_APP2EMB27_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB26_SEL field mask
#define IPC_APP2EMB26_SEL_MASK   ((uint32_t)0x00300000)
/// APP2EMB26_SEL field LSB position
#define IPC_APP2EMB26_SEL_LSB    20
/// APP2EMB26_SEL field width
#define IPC_APP2EMB26_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB25_SEL field mask
#define IPC_APP2EMB25_SEL_MASK   ((uint32_t)0x000C0000)
/// APP2EMB25_SEL field LSB position
#define IPC_APP2EMB25_SEL_LSB    18
/// APP2EMB25_SEL field width
#define IPC_APP2EMB25_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB24_SEL field mask
#define IPC_APP2EMB24_SEL_MASK   ((uint32_t)0x00030000)
/// APP2EMB24_SEL field LSB position
#define IPC_APP2EMB24_SEL_LSB    16
/// APP2EMB24_SEL field width
#define IPC_APP2EMB24_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB23_SEL field mask
#define IPC_APP2EMB23_SEL_MASK   ((uint32_t)0x0000C000)
/// APP2EMB23_SEL field LSB position
#define IPC_APP2EMB23_SEL_LSB    14
/// APP2EMB23_SEL field width
#define IPC_APP2EMB23_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB22_SEL field mask
#define IPC_APP2EMB22_SEL_MASK   ((uint32_t)0x00003000)
/// APP2EMB22_SEL field LSB position
#define IPC_APP2EMB22_SEL_LSB    12
/// APP2EMB22_SEL field width
#define IPC_APP2EMB22_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB21_SEL field mask
#define IPC_APP2EMB21_SEL_MASK   ((uint32_t)0x00000C00)
/// APP2EMB21_SEL field LSB position
#define IPC_APP2EMB21_SEL_LSB    10
/// APP2EMB21_SEL field width
#define IPC_APP2EMB21_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB20_SEL field mask
#define IPC_APP2EMB20_SEL_MASK   ((uint32_t)0x00000300)
/// APP2EMB20_SEL field LSB position
#define IPC_APP2EMB20_SEL_LSB    8
/// APP2EMB20_SEL field width
#define IPC_APP2EMB20_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB19_SEL field mask
#define IPC_APP2EMB19_SEL_MASK   ((uint32_t)0x000000C0)
/// APP2EMB19_SEL field LSB position
#define IPC_APP2EMB19_SEL_LSB    6
/// APP2EMB19_SEL field width
#define IPC_APP2EMB19_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB18_SEL field mask
#define IPC_APP2EMB18_SEL_MASK   ((uint32_t)0x00000030)
/// APP2EMB18_SEL field LSB position
#define IPC_APP2EMB18_SEL_LSB    4
/// APP2EMB18_SEL field width
#define IPC_APP2EMB18_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB17_SEL field mask
#define IPC_APP2EMB17_SEL_MASK   ((uint32_t)0x0000000C)
/// APP2EMB17_SEL field LSB position
#define IPC_APP2EMB17_SEL_LSB    2
/// APP2EMB17_SEL field width
#define IPC_APP2EMB17_SEL_WIDTH  ((uint32_t)0x00000002)
/// APP2EMB16_SEL field mask
#define IPC_APP2EMB16_SEL_MASK   ((uint32_t)0x00000003)
/// APP2EMB16_SEL field LSB position
#define IPC_APP2EMB16_SEL_LSB    0
/// APP2EMB16_SEL field width
#define IPC_APP2EMB16_SEL_WIDTH  ((uint32_t)0x00000002)

/// APP2EMB31_SEL field reset value
#define IPC_APP2EMB31_SEL_RST    0x0
/// APP2EMB30_SEL field reset value
#define IPC_APP2EMB30_SEL_RST    0x0
/// APP2EMB29_SEL field reset value
#define IPC_APP2EMB29_SEL_RST    0x0
/// APP2EMB28_SEL field reset value
#define IPC_APP2EMB28_SEL_RST    0x0
/// APP2EMB27_SEL field reset value
#define IPC_APP2EMB27_SEL_RST    0x0
/// APP2EMB26_SEL field reset value
#define IPC_APP2EMB26_SEL_RST    0x0
/// APP2EMB25_SEL field reset value
#define IPC_APP2EMB25_SEL_RST    0x0
/// APP2EMB24_SEL field reset value
#define IPC_APP2EMB24_SEL_RST    0x0
/// APP2EMB23_SEL field reset value
#define IPC_APP2EMB23_SEL_RST    0x0
/// APP2EMB22_SEL field reset value
#define IPC_APP2EMB22_SEL_RST    0x0
/// APP2EMB21_SEL field reset value
#define IPC_APP2EMB21_SEL_RST    0x0
/// APP2EMB20_SEL field reset value
#define IPC_APP2EMB20_SEL_RST    0x0
/// APP2EMB19_SEL field reset value
#define IPC_APP2EMB19_SEL_RST    0x0
/// APP2EMB18_SEL field reset value
#define IPC_APP2EMB18_SEL_RST    0x0
/// APP2EMB17_SEL field reset value
#define IPC_APP2EMB17_SEL_RST    0x0
/// APP2EMB16_SEL field reset value
#define IPC_APP2EMB16_SEL_RST    0x0

/**
 * @brief Constructs a value for the APP2EMB_LINE_SEL_HIGH register given values for its fields
 * and writes the value to the register.
 *
 * @param[in] app2emb31sel - The value to use for the APP2EMB31_SEL field.
 * @param[in] app2emb30sel - The value to use for the APP2EMB30_SEL field.
 * @param[in] app2emb29sel - The value to use for the APP2EMB29_SEL field.
 * @param[in] app2emb28sel - The value to use for the APP2EMB28_SEL field.
 * @param[in] app2emb27sel - The value to use for the APP2EMB27_SEL field.
 * @param[in] app2emb26sel - The value to use for the APP2EMB26_SEL field.
 * @param[in] app2emb25sel - The value to use for the APP2EMB25_SEL field.
 * @param[in] app2emb24sel - The value to use for the APP2EMB24_SEL field.
 * @param[in] app2emb23sel - The value to use for the APP2EMB23_SEL field.
 * @param[in] app2emb22sel - The value to use for the APP2EMB22_SEL field.
 * @param[in] app2emb21sel - The value to use for the APP2EMB21_SEL field.
 * @param[in] app2emb20sel - The value to use for the APP2EMB20_SEL field.
 * @param[in] app2emb19sel - The value to use for the APP2EMB19_SEL field.
 * @param[in] app2emb18sel - The value to use for the APP2EMB18_SEL field.
 * @param[in] app2emb17sel - The value to use for the APP2EMB17_SEL field.
 * @param[in] app2emb16sel - The value to use for the APP2EMB16_SEL field.
 */
__INLINE void ipc_app2emb_line_sel_high_pack(struct siwifi_hw *siwifi_hw, uint8_t app2emb31sel, uint8_t app2emb30sel, uint8_t app2emb29sel, uint8_t app2emb28sel, uint8_t app2emb27sel, uint8_t app2emb26sel, uint8_t app2emb25sel, uint8_t app2emb24sel, uint8_t app2emb23sel, uint8_t app2emb22sel, uint8_t app2emb21sel, uint8_t app2emb20sel, uint8_t app2emb19sel, uint8_t app2emb18sel, uint8_t app2emb17sel, uint8_t app2emb16sel)
{
	ASSERT_ERR((((uint32_t)app2emb31sel << 30) & ~((uint32_t)0xC0000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb30sel << 28) & ~((uint32_t)0x30000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb29sel << 26) & ~((uint32_t)0x0C000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb28sel << 24) & ~((uint32_t)0x03000000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb27sel << 22) & ~((uint32_t)0x00C00000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb26sel << 20) & ~((uint32_t)0x00300000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb25sel << 18) & ~((uint32_t)0x000C0000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb24sel << 16) & ~((uint32_t)0x00030000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb23sel << 14) & ~((uint32_t)0x0000C000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb22sel << 12) & ~((uint32_t)0x00003000)) == 0);
	ASSERT_ERR((((uint32_t)app2emb21sel << 10) & ~((uint32_t)0x00000C00)) == 0);
	ASSERT_ERR((((uint32_t)app2emb20sel << 8) & ~((uint32_t)0x00000300)) == 0);
	ASSERT_ERR((((uint32_t)app2emb19sel << 6) & ~((uint32_t)0x000000C0)) == 0);
	ASSERT_ERR((((uint32_t)app2emb18sel << 4) & ~((uint32_t)0x00000030)) == 0);
	ASSERT_ERR((((uint32_t)app2emb17sel << 2) & ~((uint32_t)0x0000000C)) == 0);
	ASSERT_ERR((((uint32_t)app2emb16sel << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb),  ((uint32_t)app2emb31sel << 30) | ((uint32_t)app2emb30sel << 28) | ((uint32_t)app2emb29sel << 26) | ((uint32_t)app2emb28sel << 24) | ((uint32_t)app2emb27sel << 22) | ((uint32_t)app2emb26sel << 20) | ((uint32_t)app2emb25sel << 18) | ((uint32_t)app2emb24sel << 16) | ((uint32_t)app2emb23sel << 14) | ((uint32_t)app2emb22sel << 12) | ((uint32_t)app2emb21sel << 10) | ((uint32_t)app2emb20sel << 8) | ((uint32_t)app2emb19sel << 6) | ((uint32_t)app2emb18sel << 4) | ((uint32_t)app2emb17sel << 2) | ((uint32_t)app2emb16sel << 0));
}

/**
 * @brief Unpacks APP2EMB_LINE_SEL_HIGH's fields from current value of the APP2EMB_LINE_SEL_HIGH register.
 *
 * Reads the APP2EMB_LINE_SEL_HIGH register and populates all the _field variables with the corresponding
 * values from the register.
 *
 * @param[out] app2emb31sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb30sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb29sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb28sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb27sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb26sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb25sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb24sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb23sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb22sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb21sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb20sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb19sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb18sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb17sel - Will be populated with the current value of this field from the register.
 * @param[out] app2emb16sel - Will be populated with the current value of this field from the register.
 */
__INLINE void ipc_app2emb_line_sel_high_unpack(struct siwifi_hw *siwifi_hw, uint8_t* app2emb31sel, uint8_t* app2emb30sel, uint8_t* app2emb29sel, uint8_t* app2emb28sel, uint8_t* app2emb27sel, uint8_t* app2emb26sel, uint8_t* app2emb25sel, uint8_t* app2emb24sel, uint8_t* app2emb23sel, uint8_t* app2emb22sel, uint8_t* app2emb21sel, uint8_t* app2emb20sel, uint8_t* app2emb19sel, uint8_t* app2emb18sel, uint8_t* app2emb17sel, uint8_t* app2emb16sel)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));

	*app2emb31sel = (localVal & ((uint32_t)0xC0000000)) >> 30;
	*app2emb30sel = (localVal & ((uint32_t)0x30000000)) >> 28;
	*app2emb29sel = (localVal & ((uint32_t)0x0C000000)) >> 26;
	*app2emb28sel = (localVal & ((uint32_t)0x03000000)) >> 24;
	*app2emb27sel = (localVal & ((uint32_t)0x00C00000)) >> 22;
	*app2emb26sel = (localVal & ((uint32_t)0x00300000)) >> 20;
	*app2emb25sel = (localVal & ((uint32_t)0x000C0000)) >> 18;
	*app2emb24sel = (localVal & ((uint32_t)0x00030000)) >> 16;
	*app2emb23sel = (localVal & ((uint32_t)0x0000C000)) >> 14;
	*app2emb22sel = (localVal & ((uint32_t)0x00003000)) >> 12;
	*app2emb21sel = (localVal & ((uint32_t)0x00000C00)) >> 10;
	*app2emb20sel = (localVal & ((uint32_t)0x00000300)) >> 8;
	*app2emb19sel = (localVal & ((uint32_t)0x000000C0)) >> 6;
	*app2emb18sel = (localVal & ((uint32_t)0x00000030)) >> 4;
	*app2emb17sel = (localVal & ((uint32_t)0x0000000C)) >> 2;
	*app2emb16sel = (localVal & ((uint32_t)0x00000003)) >> 0;
}

/**
 * @brief Returns the current value of the APP2EMB31_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB31_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB31_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb31_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0xC0000000)) >> 30);
}

/**
 * @brief Sets the APP2EMB31_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb31sel - The value to set the field to.
 */
__INLINE void ipc_app2emb31_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb31sel)
{
	ASSERT_ERR((((uint32_t)app2emb31sel << 30) & ~((uint32_t)0xC0000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0xC0000000)) | ((uint32_t)app2emb31sel << 30));
}

/**
 * @brief Returns the current value of the APP2EMB30_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB30_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB30_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb30_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x30000000)) >> 28);
}

/**
 * @brief Sets the APP2EMB30_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb30sel - The value to set the field to.
 */
__INLINE void ipc_app2emb30_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb30sel)
{
	ASSERT_ERR((((uint32_t)app2emb30sel << 28) & ~((uint32_t)0x30000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x30000000)) | ((uint32_t)app2emb30sel << 28));
}

/**
 * @brief Returns the current value of the APP2EMB29_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB29_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB29_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb29_sel_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0C000000)) >> 26);
}

/**
 * @brief Sets the APP2EMB29_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb29sel - The value to set the field to.
 */
__INLINE void ipc_app2emb29_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb29sel)
{
	ASSERT_ERR((((uint32_t)app2emb29sel << 26) & ~((uint32_t)0x0C000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0C000000)) | ((uint32_t)app2emb29sel << 26));
}

/**
 * @brief Returns the current value of the APP2EMB28_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB28_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB28_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb28_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x03000000)) >> 24);
}

/**
 * @brief Sets the APP2EMB28_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb28sel - The value to set the field to.
 */
__INLINE void ipc_app2emb28_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb28sel)
{
	ASSERT_ERR((((uint32_t)app2emb28sel << 24) & ~((uint32_t)0x03000000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x03000000)) | ((uint32_t)app2emb28sel << 24));
}

/**
 * @brief Returns the current value of the APP2EMB27_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB27_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB27_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb27_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00C00000)) >> 22);
}

/**
 * @brief Sets the APP2EMB27_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb27sel - The value to set the field to.
 */
__INLINE void ipc_app2emb27_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb27sel)
{
	ASSERT_ERR((((uint32_t)app2emb27sel << 22) & ~((uint32_t)0x00C00000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00C00000)) | ((uint32_t)app2emb27sel << 22));
}

/**
 * @brief Returns the current value of the APP2EMB26_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB26_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB26_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb26_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00300000)) >> 20);
}

/**
 * @brief Sets the APP2EMB26_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb26sel - The value to set the field to.
 */
__INLINE void ipc_app2emb26_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb26sel)
{
	ASSERT_ERR((((uint32_t)app2emb26sel << 20) & ~((uint32_t)0x00300000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00300000)) | ((uint32_t)app2emb26sel << 20));
}

/**
 * @brief Returns the current value of the APP2EMB25_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB25_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB25_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb25_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x000C0000)) >> 18);
}

/**
 * @brief Sets the APP2EMB25_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb25sel - The value to set the field to.
 */
__INLINE void ipc_app2emb25_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb25sel)
{
	ASSERT_ERR((((uint32_t)app2emb25sel << 18) & ~((uint32_t)0x000C0000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000C0000)) | ((uint32_t)app2emb25sel << 18));
}

/**
 * @brief Returns the current value of the APP2EMB24_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB24_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB24_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb24_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00030000)) >> 16);
}

/**
 * @brief Sets the APP2EMB24_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb24sel - The value to set the field to.
 */
__INLINE void ipc_app2emb24_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb24sel)
{
	ASSERT_ERR((((uint32_t)app2emb24sel << 16) & ~((uint32_t)0x00030000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00030000)) | ((uint32_t)app2emb24sel << 16));
}

/**
 * @brief Returns the current value of the APP2EMB23_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB23_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB23_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb23_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0000C000)) >> 14);
}

/**
 * @brief Sets the APP2EMB23_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb23sel - The value to set the field to.
 */
__INLINE void ipc_app2emb23_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb23sel)
{
	ASSERT_ERR((((uint32_t)app2emb23sel << 14) & ~((uint32_t)0x0000C000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0000C000)) | ((uint32_t)app2emb23sel << 14));
}

/**
 * @brief Returns the current value of the APP2EMB22_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB22_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB22_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb22_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00003000)) >> 12);
}

/**
 * @brief Sets the APP2EMB22_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb22sel - The value to set the field to.
 */
__INLINE void ipc_app2emb22_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb22sel)
{
	ASSERT_ERR((((uint32_t)app2emb22sel << 12) & ~((uint32_t)0x00003000)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00003000)) | ((uint32_t)app2emb22sel << 12));
}

/**
 * @brief Returns the current value of the APP2EMB21_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB21_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB21_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb21_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000C00)) >> 10);
}

/**
 * @brief Sets the APP2EMB21_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb21sel - The value to set the field to.
 */
__INLINE void ipc_app2emb21_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb21sel)
{
	ASSERT_ERR((((uint32_t)app2emb21sel << 10) & ~((uint32_t)0x00000C00)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000C00)) | ((uint32_t)app2emb21sel << 10));
}

/**
 * @brief Returns the current value of the APP2EMB20_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB20_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB20_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb20_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000300)) >> 8);
}

/**
 * @brief Sets the APP2EMB20_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb20sel - The value to set the field to.
 */
__INLINE void ipc_app2emb20_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb20sel)
{
	ASSERT_ERR((((uint32_t)app2emb20sel << 8) & ~((uint32_t)0x00000300)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000300)) | ((uint32_t)app2emb20sel << 8));
}

/**
 * @brief Returns the current value of the APP2EMB19_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB19_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB19_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb19_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x000000C0)) >> 6);
}

/**
 * @brief Sets the APP2EMB19_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb19sel - The value to set the field to.
 */
__INLINE void ipc_app2emb19_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb19sel)
{
	ASSERT_ERR((((uint32_t)app2emb19sel << 6) & ~((uint32_t)0x000000C0)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000000C0)) | ((uint32_t)app2emb19sel << 6));
}

/**
 * @brief Returns the current value of the APP2EMB18_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB18_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB18_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb18_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000030)) >> 4);
}

/**
 * @brief Sets the APP2EMB18_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb18sel - The value to set the field to.
 */
__INLINE void ipc_app2emb18_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb18sel)
{
	ASSERT_ERR((((uint32_t)app2emb18sel << 4) & ~((uint32_t)0x00000030)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000030)) | ((uint32_t)app2emb18sel << 4));
}

/**
 * @brief Returns the current value of the APP2EMB17_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB17_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB17_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb17_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x0000000C)) >> 2);
}

/**
 * @brief Sets the APP2EMB17_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb17sel - The value to set the field to.
 */
__INLINE void ipc_app2emb17_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb17sel)
{
	ASSERT_ERR((((uint32_t)app2emb17sel << 2) & ~((uint32_t)0x0000000C)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x0000000C)) | ((uint32_t)app2emb17sel << 2));
}

/**
 * @brief Returns the current value of the APP2EMB16_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read and the APP2EMB16_SEL field's value will be returned.
 *
 * @return The current value of the APP2EMB16_SEL field in the APP2EMB_LINE_SEL_HIGH register.
 */
__INLINE uint8_t ipc_app2emb16_sel_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
	return ((localVal & ((uint32_t)0x00000003)) >> 0);
}

/**
 * @brief Sets the APP2EMB16_SEL field of the APP2EMB_LINE_SEL_HIGH register.
 *
 * The APP2EMB_LINE_SEL_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] app2emb16sel - The value to set the field to.
 */
__INLINE void ipc_app2emb16_sel_setf(struct siwifi_hw *siwifi_hw, uint8_t app2emb16sel)
{
	ASSERT_ERR((((uint32_t)app2emb16sel << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(IPC_APP2EMB_LINE_SEL_HIGH_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000003)) | ((uint32_t)app2emb16sel << 0));
}

/// @}

/**
 * @name APP2EMB_STATUS register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00       APP2EMB_STATUS   0x0
 * </pre>
 *
 * @{
 */

/// Address of the APP2EMB_STATUS register
#define IPC_APP2EMB_STATUS_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x0000011C)
/// Offset of the APP2EMB_STATUS register from the base address
#define IPC_APP2EMB_STATUS_OFFSET 0x0000001C
/// Index of the APP2EMB_STATUS register
#define IPC_APP2EMB_STATUS_INDEX  0x00000007
/// Reset value of the APP2EMB_STATUS register
#define IPC_APP2EMB_STATUS_RESET  0x00000000

/**
 * @brief Returns the current value of the APP2EMB_STATUS register.
 * The APP2EMB_STATUS register will be read and its value returned.
 * @return The current value of the APP2EMB_STATUS register.
 */
__INLINE uint32_t ipc_app2emb_status_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_APP2EMB_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// APP2EMB_STATUS field mask
#define IPC_APP2EMB_STATUS_MASK   ((uint32_t)0xFFFFFFFF)
/// APP2EMB_STATUS field LSB position
#define IPC_APP2EMB_STATUS_LSB    0
/// APP2EMB_STATUS field width
#define IPC_APP2EMB_STATUS_WIDTH  ((uint32_t)0x00000020)

/// APP2EMB_STATUS field reset value
#define IPC_APP2EMB_STATUS_RST    0x0

/**
 * @brief Returns the current value of the APP2EMB_STATUS field in the APP2EMB_STATUS register.
 *
 * The APP2EMB_STATUS register will be read and the APP2EMB_STATUS field's value will be returned.
 *
 * @return The current value of the APP2EMB_STATUS field in the APP2EMB_STATUS register.
 */
__INLINE uint32_t ipc_app2emb_status_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_APP2EMB_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/// @}

/**
 * @name EMB_SIGNATURE register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00            SIGNATURE   0x49504332
 * </pre>
 *
 * @{
 */

/// Address of the EMB_SIGNATURE register
#define IPC_EMB_SIGNATURE_ADDR(band)   (WIFI_REG_IPC_EMB_BASE_ADDR(band) + 0x00000140)
/// Offset of the EMB_SIGNATURE register from the base address
#define IPC_EMB_SIGNATURE_OFFSET 0x00000040
/// Index of the EMB_SIGNATURE register
#define IPC_EMB_SIGNATURE_INDEX  0x00000010
/// Reset value of the EMB_SIGNATURE register
#define IPC_EMB_SIGNATURE_RESET  0x49504332

/**
 * @brief Returns the current value of the EMB_SIGNATURE register.
 * The EMB_SIGNATURE register will be read and its value returned.
 * @return The current value of the EMB_SIGNATURE register.
 */
__INLINE uint32_t ipc_emb_signature_get(struct siwifi_hw *siwifi_hw)
{
	return REG_PL_RD(IPC_EMB_SIGNATURE_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// SIGNATURE field mask
#define IPC_SIGNATURE_MASK   ((uint32_t)0xFFFFFFFF)
/// SIGNATURE field LSB position
#define IPC_SIGNATURE_LSB    0
/// SIGNATURE field width
#define IPC_SIGNATURE_WIDTH  ((uint32_t)0x00000020)

/// SIGNATURE field reset value
#define IPC_SIGNATURE_RST    0x49504332

/**
 * @brief Returns the current value of the SIGNATURE field in the EMB_SIGNATURE register.
 *
 * The EMB_SIGNATURE register will be read and the SIGNATURE field's value will be returned.
 *
 * @return The current value of the SIGNATURE field in the EMB_SIGNATURE register.
 */
__INLINE uint32_t ipc_signature_getf(struct siwifi_hw *siwifi_hw)
{
	uint32_t localVal = REG_PL_RD(IPC_EMB_SIGNATURE_ADDR(siwifi_hw->mod_params->is_hb));
	ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
	return (localVal >> 0);
}

/// @}


#endif // _REG_IPC_EMB_H_

/// @}
