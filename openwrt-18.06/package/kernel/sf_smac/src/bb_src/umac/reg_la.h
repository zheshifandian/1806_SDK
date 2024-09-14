/**
 * @file reg_la.h
 * @brief Definitions of the LA HW block registers and register access functions.
 *
 * @defgroup REG_LA REG_LA
 * @ingroup REG
 * @{
 *
 * @brief Definitions of the LA HW block registers and register access functions.
 */
#ifndef _REG_LA_H_
#define _REG_LA_H_

#ifndef __KERNEL__
#include <stdint.h>
#include "arch.h"
#else
#include "ipc_compat.h"
#endif
#include "reg_access.h"

/** @brief Number of registers in the REG_LA peripheral.
 */
#define REG_LA_COUNT 16

#define REG_LA_BASE_ADDR(band) (WIFI_BASE_ADDR(band) + 0x00200000)
/** @brief Decoding mask of the REG_LA peripheral registers from the CPU point of view.
 */
#define REG_LA_DECODING_MASK 0x0000003F

/**
 * @name ID_LOW register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00               id_low   0x656E6562
 * </pre>
 *
 * @{
 */

/// Address of the ID_LOW register
#define LA_ID_LOW_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0000)
/// Offset of the ID_LOW register from the base address
#define LA_ID_LOW_OFFSET 0x00000000
/// Index of the ID_LOW register
#define LA_ID_LOW_INDEX  0x00000000
/// Reset value of the ID_LOW register
///#define LA_ID_LOW_RESET  0x00000000
#define LA_ID_LOW_RESET  0x656E6562
/**
 * @brief Returns the current value of the ID_LOW register.
 * The ID_LOW register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the ID_LOW register.
 */
__INLINE uint32_t la_id_low_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_ID_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// ID_LOW field mask
#define LA_ID_LOW_MASK   ((uint32_t)0xFFFFFFFF)
/// ID_LOW field LSB position
#define LA_ID_LOW_LSB    0
/// ID_LOW field width
#define LA_ID_LOW_WIDTH  ((uint32_t)0x00000020)

/// ID_LOW field reset value
#define LA_ID_LOW_RST    0x656E6562


/**
 * @brief Returns the current value of the id_low field in the ID_LOW register.
 *
 * The ID_LOW register will be read and the id_low field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the id_low field in the ID_LOW register.
 */
__INLINE uint32_t la_id_low_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_ID_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/// @}

/**
 * @name ID_HIGH register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             la_depth   0x0
 * </pre>
 *
 * @{
 */

/// Address of the ID_HIGH register
#define LA_ID_HIGH_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0004)
/// Offset of the ID_HIGH register from the base address
#define LA_ID_HIGH_OFFSET 0x00000004
/// Index of the ID_HIGH register
#define LA_ID_HIGH_INDEX  0x00000001
/// Reset value of the ID_HIGH register
#define LA_ID_HIGH_RESET  0x00000000

#define LA_ID_HIGH_MASK  0xFFFFFFFF

/**
 * @brief Returns the current value of the ID_HIGH register.
 * The ID_HIGH register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the ID_HIGH register.
 */
__INLINE uint32_t la_id_high_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_ID_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// LA_DEPTH field mask
#define LA_LA_DEPTH_MASK   ((uint32_t)0x0000FFFF)
/// LA_DEPTH field LSB position
#define LA_LA_DEPTH_LSB    0
/// LA_DEPTH field width
#define LA_LA_DEPTH_WIDTH  ((uint32_t)0x00000010)

/// LA_DEPTH field reset value
#define LA_LA_DEPTH_RST    0x0

/**
 * @brief Returns the current value of the la_depth field in the ID_HIGH register.
 *
 * The ID_HIGH register will be read and the la_depth field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the la_depth field in the ID_HIGH register.
 */
__INLINE uint16_t la_la_depth_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_ID_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

/// @}

/**
 * @name VERSION register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:24         samplingFreq   0x0
 *  23:00           la_version   0x40000
 * </pre>
 *
 * @{
 */

/// Address of the VERSION register
#define LA_VERSION_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0008)
/// Offset of the VERSION register from the base address
#define LA_VERSION_OFFSET 0x00000008
/// Index of the VERSION register
#define LA_VERSION_INDEX  0x00000002
/// Reset value of the VERSION register
#define LA_VERSION_RESET  0x00040000

/**
 * @brief Returns the current value of the VERSION register.
 * The VERSION register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the VERSION register.
 */
__INLINE uint32_t la_version_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// SAMPLING_FREQ field mask
#define LA_SAMPLING_FREQ_MASK   ((uint32_t)0xFF000000)
/// SAMPLING_FREQ field LSB position
#define LA_SAMPLING_FREQ_LSB    24
/// SAMPLING_FREQ field width
#define LA_SAMPLING_FREQ_WIDTH  ((uint32_t)0x00000008)
/// LA_VERSION field mask
#define LA_LA_VERSION_MASK      ((uint32_t)0x00FFFFFF)
/// LA_VERSION field LSB position
#define LA_LA_VERSION_LSB       0
/// LA_VERSION field width
#define LA_LA_VERSION_WIDTH     ((uint32_t)0x00000018)

/// SAMPLING_FREQ field reset value
#define LA_SAMPLING_FREQ_RST    0x0
/// LA_VERSION field reset value
#define LA_LA_VERSION_RST       0x40000

/**
 * @brief Unpacks VERSION's fields from current value of the VERSION register.
 *
 * Reads the VERSION register and populates all the _field variables with the corresponding
 * values from the register.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[out] samplingfreq - Will be populated with the current value of this field from the register.
 * @param[out] laversion - Will be populated with the current value of this field from the register.
 */
__INLINE void la_version_unpack(struct siwifi_hw * siwifi_hw, uint8_t* samplingfreq, uint32_t* laversion)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));

    *samplingfreq = (localVal & ((uint32_t)0xFF000000)) >> 24;
    *laversion = (localVal & ((uint32_t)0x00FFFFFF)) >> 0;
}

/**
 * @brief Returns the current value of the samplingFreq field in the VERSION register.
 *
 * The VERSION register will be read and the samplingFreq field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the samplingFreq field in the VERSION register.
 */
__INLINE uint8_t la_sampling_freq_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0xFF000000)) >> 24);
}

/**
 * @brief Returns the current value of the la_version field in the VERSION register.
 *
 * The VERSION register will be read and the la_version field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the la_version field in the VERSION register.
 */
__INLINE uint32_t la_la_version_getf(struct siwifi_hw * siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_VERSION_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00FFFFFF)) >> 0);
}

/// @}

/**
 * @name CNTRL register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     14       trigger_int_en   0
 *     13    trigger_int_clear   0
 *     12      trigger_int_set   0
 *  07:04       ext_trigger_en   0xF
 *     03           sw_trigger   0
 *     02                reset   0
 *     01                 stop   0
 *     00                start   0
 * </pre>
 *
 * @{
 */

/// Address of the CNTRL register
#define LA_CNTRL_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x000C)
/// Offset of the CNTRL register from the base address
#define LA_CNTRL_OFFSET 0x0000000C
/// Index of the CNTRL register
#define LA_CNTRL_INDEX  0x00000003
/// Reset value of the CNTRL register
#define LA_CNTRL_RESET  0x000000F0

/**
 * @brief Returns the current value of the CNTRL register.
 * The CNTRL register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the CNTRL register.
 */
__INLINE uint32_t la_cntrl_get(struct siwifi_hw * siwifi_hw)
{
    return REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the CNTRL register to a value.
 * The CNTRL register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_cntrl_set(struct siwifi_hw * siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// LA_CLK_SEL field bit
#define LA_CLK_SEL_BIT              ((uint32_t)0x00010000)
/// LA_CLK_SEL field position
#define LA_CLK_SEL_POS              16
/// LA_CONT_MODE field bit
#define LA_CONT_MODE_BIT            ((uint32_t)0x00008000)
/// LA_CONT_MODE field position
#define LA_CONT_MODE_POS            15
/// TRIGGER_INT_EN field bit
#define LA_TRIGGER_INT_EN_BIT       ((uint32_t)0x00004000)
/// TRIGGER_INT_EN field position
#define LA_TRIGGER_INT_EN_POS       14
/// TRIGGER_INT_CLEAR field bit
#define LA_TRIGGER_INT_CLEAR_BIT    ((uint32_t)0x00002000)
/// TRIGGER_INT_CLEAR field position
#define LA_TRIGGER_INT_CLEAR_POS    13
/// TRIGGER_INT_SET field bit
#define LA_TRIGGER_INT_SET_BIT      ((uint32_t)0x00001000)
/// TRIGGER_INT_SET field position
#define LA_TRIGGER_INT_SET_POS      12
/// EXT_TRIGGER_EN field mask
#define LA_EXT_TRIGGER_EN_MASK      ((uint32_t)0x000000F0)
/// EXT_TRIGGER_EN field LSB position
#define LA_EXT_TRIGGER_EN_LSB       4
/// EXT_TRIGGER_EN field width
#define LA_EXT_TRIGGER_EN_WIDTH     ((uint32_t)0x00000004)
/// SW_TRIGGER field bit
#define LA_SW_TRIGGER_BIT           ((uint32_t)0x00000008)
/// SW_TRIGGER field position
#define LA_SW_TRIGGER_POS           3
/// RESET field bit
#define LA_RESET_BIT                ((uint32_t)0x00000004)
/// RESET field position
#define LA_RESET_POS                2
/// STOP field bit
#define LA_STOP_BIT                 ((uint32_t)0x00000002)
/// STOP field position
#define LA_STOP_POS                 1
/// START field bit
#define LA_START_BIT                ((uint32_t)0x00000001)
/// START field position
#define LA_START_POS                0

/// TRIGGER_INT_EN field reset value
#define LA_TRIGGER_INT_EN_RST       0x0
/// TRIGGER_INT_CLEAR field reset value
#define LA_TRIGGER_INT_CLEAR_RST    0x0
/// TRIGGER_INT_SET field reset value
#define LA_TRIGGER_INT_SET_RST      0x0
/// EXT_TRIGGER_EN field reset value
#define LA_EXT_TRIGGER_EN_RST       0xF
/// SW_TRIGGER field reset value
#define LA_SW_TRIGGER_RST           0x0
/// RESET field reset value
#define LA_RESET_RST                0x0
/// STOP field reset value
#define LA_STOP_RST                 0x0
/// START field reset value
#define LA_START_RST                0x0

/**
 * @brief Constructs a value for the CNTRL register given values for its fields
 * and writes the value to the register.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggerinten - The value to use for the trigger_int_en field.
 * @param[in] triggerintclear - The value to use for the trigger_int_clear field.
 * @param[in] triggerintset - The value to use for the trigger_int_set field.
 * @param[in] exttriggeren - The value to use for the ext_trigger_en field.
 * @param[in] swtrigger - The value to use for the sw_trigger field.
 * @param[in] reset - The value to use for the reset field.
 * @param[in] stop - The value to use for the stop field.
 * @param[in] start - The value to use for the start field.
 */
__INLINE void la_cntrl_pack(struct siwifi_hw *siwifi_hw, uint8_t triggerinten, uint8_t triggerintclear, uint8_t triggerintset, uint8_t exttriggeren, uint8_t swtrigger, uint8_t reset, uint8_t stop, uint8_t start)
{
    ASSERT_ERR((((uint32_t)triggerinten << 14) & ~((uint32_t)0x00004000)) == 0);
    ASSERT_ERR((((uint32_t)triggerintclear << 13) & ~((uint32_t)0x00002000)) == 0);
    ASSERT_ERR((((uint32_t)triggerintset << 12) & ~((uint32_t)0x00001000)) == 0);
    ASSERT_ERR((((uint32_t)exttriggeren << 4) & ~((uint32_t)0x000000F0)) == 0);
    ASSERT_ERR((((uint32_t)swtrigger << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)reset << 2) & ~((uint32_t)0x00000004)) == 0);
    ASSERT_ERR((((uint32_t)stop << 1) & ~((uint32_t)0x00000002)) == 0);
    ASSERT_ERR((((uint32_t)start << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb),  ((uint32_t)triggerinten << 14) | ((uint32_t)triggerintclear << 13) | ((uint32_t)triggerintset << 12) | ((uint32_t)exttriggeren << 4) | ((uint32_t)swtrigger << 3) | ((uint32_t)reset << 2) | ((uint32_t)stop << 1) | ((uint32_t)start << 0));
}

/**
 * @brief Unpacks CNTRL's fields from current value of the CNTRL register.
 *
 * Reads the CNTRL register and populates all the _field variables with the corresponding
 * values from the register.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[out] triggerinten - Will be populated with the current value of this field from the register.
 * @param[out] triggerintclear - Will be populated with the current value of this field from the register.
 * @param[out] triggerintset - Will be populated with the current value of this field from the register.
 * @param[out] exttriggeren - Will be populated with the current value of this field from the register.
 * @param[out] swtrigger - Will be populated with the current value of this field from the register.
 * @param[out] reset - Will be populated with the current value of this field from the register.
 * @param[out] stop - Will be populated with the current value of this field from the register.
 * @param[out] start - Will be populated with the current value of this field from the register.
 */
__INLINE void la_cntrl_unpack(struct siwifi_hw *siwifi_hw, uint8_t* triggerinten, uint8_t* triggerintclear, uint8_t* triggerintset, uint8_t* exttriggeren, uint8_t* swtrigger, uint8_t* reset, uint8_t* stop, uint8_t* start)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));

    *triggerinten = (localVal & ((uint32_t)0x00004000)) >> 14;
    *triggerintclear = (localVal & ((uint32_t)0x00002000)) >> 13;
    *triggerintset = (localVal & ((uint32_t)0x00001000)) >> 12;
    *exttriggeren = (localVal & ((uint32_t)0x000000F0)) >> 4;
    *swtrigger = (localVal & ((uint32_t)0x00000008)) >> 3;
    *reset = (localVal & ((uint32_t)0x00000004)) >> 2;
    *stop = (localVal & ((uint32_t)0x00000002)) >> 1;
    *start = (localVal & ((uint32_t)0x00000001)) >> 0;
}

/**
 * @brief Returns the current value of the trigger_int_en field in the CNTRL register.
 *
 * The CNTRL register will be read and the trigger_int_en field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_int_en field in the CNTRL register.
 */
__INLINE uint8_t la_trigger_int_en_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00004000)) >> 14);
}

/**
 * @brief Sets the trigger_int_en field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggerinten - The value to set the field to.
 */
__INLINE void la_trigger_int_en_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerinten)
{
    ASSERT_ERR((((uint32_t)triggerinten << 14) & ~((uint32_t)0x00004000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00004000)) | ((uint32_t)triggerinten << 14));
}

/**
 * @brief Returns the current value of the trigger_int_clear field in the CNTRL register.
 *
 * The CNTRL register will be read and the trigger_int_clear field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_int_clear field in the CNTRL register.
 */
__INLINE uint8_t la_trigger_int_clear_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00002000)) >> 13);
}

/**
 * @brief Sets the trigger_int_clear field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggerintclear - The value to set the field to.
 */
__INLINE void la_trigger_int_clear_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerintclear)
{
    ASSERT_ERR((((uint32_t)triggerintclear << 13) & ~((uint32_t)0x00002000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00002000)) | ((uint32_t)triggerintclear << 13));
}

/**
 * @brief Returns the current value of the trigger_int_set field in the CNTRL register.
 *
 * The CNTRL register will be read and the trigger_int_set field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_int_set field in the CNTRL register.
 */
__INLINE uint8_t la_trigger_int_set_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00001000)) >> 12);
}

/**
 * @brief Sets the trigger_int_set field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggerintset - The value to set the field to.
 */
__INLINE void la_trigger_int_set_setf(struct siwifi_hw *siwifi_hw, uint8_t triggerintset)
{
    ASSERT_ERR((((uint32_t)triggerintset << 12) & ~((uint32_t)0x00001000)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00001000)) | ((uint32_t)triggerintset << 12));
}

/**
 * @brief Returns the current value of the ext_trigger_en field in the CNTRL register.
 *
 * The CNTRL register will be read and the ext_trigger_en field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the ext_trigger_en field in the CNTRL register.
 */
__INLINE uint8_t la_ext_trigger_en_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x000000F0)) >> 4);
}

/**
 * @brief Sets the ext_trigger_en field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] exttriggeren - The value to set the field to.
 */
__INLINE void la_ext_trigger_en_setf(struct siwifi_hw *siwifi_hw, uint8_t exttriggeren)
{
    ASSERT_ERR((((uint32_t)exttriggeren << 4) & ~((uint32_t)0x000000F0)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x000000F0)) | ((uint32_t)exttriggeren << 4));
}

/**
 * @brief Returns the current value of the sw_trigger field in the CNTRL register.
 *
 * The CNTRL register will be read and the sw_trigger field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the sw_trigger field in the CNTRL register.
 */
__INLINE uint8_t la_sw_trigger_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}

/**
 * @brief Sets the sw_trigger field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] swtrigger - The value to set the field to.
 */
__INLINE void la_sw_trigger_setf(struct siwifi_hw *siwifi_hw, uint8_t swtrigger)
{
    ASSERT_ERR((((uint32_t)swtrigger << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000008)) | ((uint32_t)swtrigger << 3));
}

/**
 * @brief Returns the current value of the reset field in the CNTRL register.
 *
 * The CNTRL register will be read and the reset field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the reset field in the CNTRL register.
 */
__INLINE uint8_t la_reset_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}

/**
 * @brief Sets the reset field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] reset - The value to set the field to.
 */
__INLINE void la_reset_setf(struct siwifi_hw *siwifi_hw, uint8_t reset)
{
    ASSERT_ERR((((uint32_t)reset << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000004)) | ((uint32_t)reset << 2));
}

/**
 * @brief Returns the current value of the stop field in the CNTRL register.
 *
 * The CNTRL register will be read and the stop field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the stop field in the CNTRL register.
 */
__INLINE uint8_t la_stop_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}

/**
 * @brief Sets the stop field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] stop - The value to set the field to.
 */
__INLINE void la_stop_setf(struct siwifi_hw *siwifi_hw, uint8_t stop)
{
    ASSERT_ERR((((uint32_t)stop << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000002)) | ((uint32_t)stop << 1));
}

/**
 * @brief Returns the current value of the start field in the CNTRL register.
 *
 * The CNTRL register will be read and the start field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the start field in the CNTRL register.
 */
__INLINE uint8_t la_start_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}

/**
 * @brief Sets the start field of the CNTRL register.
 *
 * The CNTRL register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] start - The value to set the field to.
 */
__INLINE void la_start_setf(struct siwifi_hw *siwifi_hw, uint8_t start)
{
    ASSERT_ERR((((uint32_t)start << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb), (REG_PL_RD(LA_CNTRL_ADDR(siwifi_hw->mod_params->is_hb)) & ~((uint32_t)0x00000001)) | ((uint32_t)start << 0));
}

/// @}

/**
 * @name STATUS register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  23:08            writeaddr   0x0
 *  07:04   ext_trigger_status   0x0
 *     03    sw_trigger_status   0
 *     02   internal_trigger_status   0
 *     01            triggered   0
 *     00              started   0
 * </pre>
 *
 * @{
 */

/// Address of the STATUS register
#define LA_STATUS_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0010)
/// Offset of the STATUS register from the base address
#define LA_STATUS_OFFSET 0x00000010
/// Index of the STATUS register
#define LA_STATUS_INDEX  0x00000004
/// Reset value of the STATUS register
#define LA_STATUS_RESET  0x00000000

/**
 * @brief Returns the current value of the STATUS register.
 * The STATUS register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the STATUS register.
 */
__INLINE uint32_t la_status_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// WRITEADDR field mask
#define LA_WRITEADDR_MASK                 ((uint32_t)0x00FFFF00)
/// WRITEADDR field LSB position
#define LA_WRITEADDR_LSB                  8
/// WRITEADDR field width
#define LA_WRITEADDR_WIDTH                ((uint32_t)0x00000010)
/// EXT_TRIGGER_STATUS field mask
#define LA_EXT_TRIGGER_STATUS_MASK        ((uint32_t)0x000000F0)
/// EXT_TRIGGER_STATUS field LSB position
#define LA_EXT_TRIGGER_STATUS_LSB         4
/// EXT_TRIGGER_STATUS field width
#define LA_EXT_TRIGGER_STATUS_WIDTH       ((uint32_t)0x00000004)
/// SW_TRIGGER_STATUS field bit
#define LA_SW_TRIGGER_STATUS_BIT          ((uint32_t)0x00000008)
/// SW_TRIGGER_STATUS field position
#define LA_SW_TRIGGER_STATUS_POS          3
/// INTERNAL_TRIGGER_STATUS field bit
#define LA_INTERNAL_TRIGGER_STATUS_BIT    ((uint32_t)0x00000004)
/// INTERNAL_TRIGGER_STATUS field position
#define LA_INTERNAL_TRIGGER_STATUS_POS    2
/// TRIGGERED field bit
#define LA_TRIGGERED_BIT                  ((uint32_t)0x00000002)
/// TRIGGERED field position
#define LA_TRIGGERED_POS                  1
/// STARTED field bit
#define LA_STARTED_BIT                    ((uint32_t)0x00000001)
/// STARTED field position
#define LA_STARTED_POS                    0

/// WRITEADDR field reset value
#define LA_WRITEADDR_RST                  0x0
/// EXT_TRIGGER_STATUS field reset value
#define LA_EXT_TRIGGER_STATUS_RST         0x0
/// SW_TRIGGER_STATUS field reset value
#define LA_SW_TRIGGER_STATUS_RST          0x0
/// INTERNAL_TRIGGER_STATUS field reset value
#define LA_INTERNAL_TRIGGER_STATUS_RST    0x0
/// TRIGGERED field reset value
#define LA_TRIGGERED_RST                  0x0
/// STARTED field reset value
#define LA_STARTED_RST                    0x0

/**
 * @brief Unpacks STATUS's fields from current value of the STATUS register.
 *
 * Reads the STATUS register and populates all the _field variables with the corresponding
 * values from the register.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[out] writeaddr - Will be populated with the current value of this field from the register.
 * @param[out] exttriggerstatus - Will be populated with the current value of this field from the register.
 * @param[out] swtriggerstatus - Will be populated with the current value of this field from the register.
 * @param[out] internaltriggerstatus - Will be populated with the current value of this field from the register.
 * @param[out] triggered - Will be populated with the current value of this field from the register.
 * @param[out] started - Will be populated with the current value of this field from the register.
 */
__INLINE void la_status_unpack(struct siwifi_hw *siwifi_hw, uint16_t* writeaddr, uint8_t* exttriggerstatus, uint8_t* swtriggerstatus, uint8_t* internaltriggerstatus, uint8_t* triggered, uint8_t* started)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));

    *writeaddr = (localVal & ((uint32_t)0x00FFFF00)) >> 8;
    *exttriggerstatus = (localVal & ((uint32_t)0x000000F0)) >> 4;
    *swtriggerstatus = (localVal & ((uint32_t)0x00000008)) >> 3;
    *internaltriggerstatus = (localVal & ((uint32_t)0x00000004)) >> 2;
    *triggered = (localVal & ((uint32_t)0x00000002)) >> 1;
    *started = (localVal & ((uint32_t)0x00000001)) >> 0;
}

/**
 * @brief Returns the current value of the writeaddr field in the STATUS register.
 *
 * The STATUS register will be read and the writeaddr field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the writeaddr field in the STATUS register.
 */
__INLINE uint16_t la_writeaddr_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00FFFF00)) >> 8);
}

/**
 * @brief Returns the current value of the ext_trigger_status field in the STATUS register.
 *
 * The STATUS register will be read and the ext_trigger_status field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the ext_trigger_status field in the STATUS register.
 */
__INLINE uint8_t la_ext_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x000000F0)) >> 4);
}

/**
 * @brief Returns the current value of the sw_trigger_status field in the STATUS register.
 *
 * The STATUS register will be read and the sw_trigger_status field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the sw_trigger_status field in the STATUS register.
 */
__INLINE uint8_t la_sw_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}

/**
 * @brief Returns the current value of the internal_trigger_status field in the STATUS register.
 *
 * The STATUS register will be read and the internal_trigger_status field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the internal_trigger_status field in the STATUS register.
 */
__INLINE uint8_t la_internal_trigger_status_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000004)) >> 2);
}

/**
 * @brief Returns the current value of the triggered field in the STATUS register.
 *
 * The STATUS register will be read and the triggered field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the triggered field in the STATUS register.
 */
__INLINE uint8_t la_triggered_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000002)) >> 1);
}

/**
 * @brief Returns the current value of the started field in the STATUS register.
 *
 * The STATUS register will be read and the started field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the started field in the STATUS register.
 */
__INLINE uint8_t la_started_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_STATUS_ADDR(siwifi_hw->mod_params->is_hb));
    return ((localVal & ((uint32_t)0x00000001)) >> 0);
}

/// @}

/**
 * @name SAMPLING_MASK_LOW register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    sampling_mask_low   0x0
 * </pre>
 *
 * @{
 */

/// Address of the SAMPLING_MASK_LOW register
#define LA_SAMPLING_MASK_LOW_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0014)
/// Offset of the SAMPLING_MASK_LOW register from the base address
#define LA_SAMPLING_MASK_LOW_OFFSET 0x00000014
/// Index of the SAMPLING_MASK_LOW register
#define LA_SAMPLING_MASK_LOW_INDEX  0x00000005
/// Reset value of the SAMPLING_MASK_LOW register
#define LA_SAMPLING_MASK_LOW_RESET  0x00000000

/**
 * @brief Returns the current value of the SAMPLING_MASK_LOW register.
 * The SAMPLING_MASK_LOW register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the SAMPLING_MASK_LOW register.
 */
__INLINE uint32_t la_sampling_mask_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the SAMPLING_MASK_LOW register to a value.
 * The SAMPLING_MASK_LOW register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_sampling_mask_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// SAMPLING_MASK_LOW field mask
#define LA_SAMPLING_MASK_LOW_MASK   ((uint32_t)0xFFFFFFFF)
/// SAMPLING_MASK_LOW field LSB position
#define LA_SAMPLING_MASK_LOW_LSB    0
/// SAMPLING_MASK_LOW field width
#define LA_SAMPLING_MASK_LOW_WIDTH  ((uint32_t)0x00000020)

/// SAMPLING_MASK_LOW field reset value
#define LA_SAMPLING_MASK_LOW_RST    0x0

/**
 * @brief Returns the current value of the sampling_mask_low field in the SAMPLING_MASK_LOW register.
 *
 * The SAMPLING_MASK_LOW register will be read and the sampling_mask_low field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the sampling_mask_low field in the SAMPLING_MASK_LOW register.
 */
__INLINE uint32_t la_sampling_mask_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the sampling_mask_low field of the SAMPLING_MASK_LOW register.
 *
 * The SAMPLING_MASK_LOW register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] samplingmasklow - The value to set the field to.
 */
__INLINE void la_sampling_mask_low_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmasklow)
{
    ASSERT_ERR((((uint32_t)samplingmasklow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmasklow << 0);
}

/// @}

/**
 * @name SAMPLING_MASK_MED register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    sampling_mask_med   0x0
 * </pre>
 *
 * @{
 */

/// Address of the SAMPLING_MASK_MED register
#define LA_SAMPLING_MASK_MED_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0018)
/// Offset of the SAMPLING_MASK_MED register from the base address
#define LA_SAMPLING_MASK_MED_OFFSET 0x00000018
/// Index of the SAMPLING_MASK_MED register
#define LA_SAMPLING_MASK_MED_INDEX  0x00000006
/// Reset value of the SAMPLING_MASK_MED register
#define LA_SAMPLING_MASK_MED_RESET  0x00000000

/**
 * @brief Returns the current value of the SAMPLING_MASK_MED register.
 * The SAMPLING_MASK_MED register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the SAMPLING_MASK_MED register.
 */
__INLINE uint32_t la_sampling_mask_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the SAMPLING_MASK_MED register to a value.
 * The SAMPLING_MASK_MED register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_sampling_mask_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// SAMPLING_MASK_MED field mask
#define LA_SAMPLING_MASK_MED_MASK   ((uint32_t)0xFFFFFFFF)
/// SAMPLING_MASK_MED field LSB position
#define LA_SAMPLING_MASK_MED_LSB    0
/// SAMPLING_MASK_MED field width
#define LA_SAMPLING_MASK_MED_WIDTH  ((uint32_t)0x00000020)

/// SAMPLING_MASK_MED field reset value
#define LA_SAMPLING_MASK_MED_RST    0x0

/**
 * @brief Returns the current value of the sampling_mask_med field in the SAMPLING_MASK_MED register.
 *
 * The SAMPLING_MASK_MED register will be read and the sampling_mask_med field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the sampling_mask_med field in the SAMPLING_MASK_MED register.
 */
__INLINE uint32_t la_sampling_mask_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the sampling_mask_med field of the SAMPLING_MASK_MED register.
 *
 * The SAMPLING_MASK_MED register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] samplingmaskmed - The value to set the field to.
 */
__INLINE void la_sampling_mask_med_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmaskmed)
{
    ASSERT_ERR((((uint32_t)samplingmaskmed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmaskmed << 0);
}

/// @}

/**
 * @name SAMPLING_MASK_HIGH register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00   sampling_mask_high   0x0
 * </pre>
 *
 * @{
 */

/// Address of the SAMPLING_MASK_HIGH register
#define LA_SAMPLING_MASK_HIGH_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x001C)
/// Offset of the SAMPLING_MASK_HIGH register from the base address
#define LA_SAMPLING_MASK_HIGH_OFFSET 0x0000001C
/// Index of the SAMPLING_MASK_HIGH register
#define LA_SAMPLING_MASK_HIGH_INDEX  0x00000007
/// Reset value of the SAMPLING_MASK_HIGH register
#define LA_SAMPLING_MASK_HIGH_RESET  0x00000000

/**
 * @brief Returns the current value of the SAMPLING_MASK_HIGH register.
 * The SAMPLING_MASK_HIGH register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the SAMPLING_MASK_HIGH register.
 */
__INLINE uint32_t la_sampling_mask_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the SAMPLING_MASK_HIGH register to a value.
 * The SAMPLING_MASK_HIGH register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_sampling_mask_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// SAMPLING_MASK_HIGH field mask
#define LA_SAMPLING_MASK_HIGH_MASK   ((uint32_t)0xFFFFFFFF)
/// SAMPLING_MASK_HIGH field LSB position
#define LA_SAMPLING_MASK_HIGH_LSB    0
/// SAMPLING_MASK_HIGH field width
#define LA_SAMPLING_MASK_HIGH_WIDTH  ((uint32_t)0x00000020)

/// SAMPLING_MASK_HIGH field reset value
#define LA_SAMPLING_MASK_HIGH_RST    0x0

/**
 * @brief Returns the current value of the sampling_mask_high field in the SAMPLING_MASK_HIGH register.
 *
 * The SAMPLING_MASK_HIGH register will be read and the sampling_mask_high field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the sampling_mask_high field in the SAMPLING_MASK_HIGH register.
 */
__INLINE uint32_t la_sampling_mask_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the sampling_mask_high field of the SAMPLING_MASK_HIGH register.
 *
 * The SAMPLING_MASK_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] samplingmaskhigh - The value to set the field to.
 */
__INLINE void la_sampling_mask_high_setf(struct siwifi_hw *siwifi_hw, uint32_t samplingmaskhigh)
{
    ASSERT_ERR((((uint32_t)samplingmaskhigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_SAMPLING_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)samplingmaskhigh << 0);
}

/// @}

/**
 * @name TRIGGER_MASK_LOW register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00     trigger_mask_med   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_MASK_LOW register
#define LA_TRIGGER_MASK_LOW_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0020)
/// Offset of the TRIGGER_MASK_LOW register from the base address
#define LA_TRIGGER_MASK_LOW_OFFSET 0x00000020
/// Index of the TRIGGER_MASK_LOW register
#define LA_TRIGGER_MASK_LOW_INDEX  0x00000008
/// Reset value of the TRIGGER_MASK_LOW register
#define LA_TRIGGER_MASK_LOW_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_MASK_LOW register.
 * The TRIGGER_MASK_LOW register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_MASK_LOW register.
 */
__INLINE uint32_t la_trigger_mask_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_MASK_LOW register to a value.
 * The TRIGGER_MASK_LOW register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_mask_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_MASK_LOW field mask
#define LA_TRIGGER_MASK_LOW_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_MASK_LOW field LSB position
#define LA_TRIGGER_MASK_LOW_LSB    0
/// TRIGGER_MASK_LOW field width
#define LA_TRIGGER_MASK_LOW_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_MASK_LOW field reset value
#define LA_TRIGGER_MASK_LOW_RST    0x0

/**
 * @brief Returns the current value of the trigger_mask_low field in the TRIGGER_MASK_LOW register.
 *
 * The TRIGGER_MASK_LOW register will be read and the trigger_mask_low field's value will be returned.
 *
 * @return The current value of the trigger_mask_low field in the TRIGGER_MASK_LOW register.
 */
__INLINE uint32_t la_trigger_mask_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_mask_low field of the TRIGGER_MASK_LOW register.
 *
 * The TRIGGER_MASK_LOW register will be read, modified to contain the new field value, and written.
 *
 * @param[in] triggermasklow - The value to set the field to.
 */
__INLINE void la_trigger_mask_low_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermasklow)
{
    ASSERT_ERR((((uint32_t)triggermasklow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermasklow << 0);
}

/// @}

/**
 * @name TRIGGER_MASK_MED register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00     trigger_mask_low   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_MASK_MED register
#define LA_TRIGGER_MASK_MED_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0024)
/// Offset of the TRIGGER_MASK_MED register from the base address
#define LA_TRIGGER_MASK_MED_OFFSET 0x00000024
/// Index of the TRIGGER_MASK_MED register
#define LA_TRIGGER_MASK_MED_INDEX  0x00000009
/// Reset value of the TRIGGER_MASK_MED register
#define LA_TRIGGER_MASK_MED_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_MASK_MED register.
 * The TRIGGER_MASK_MED register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_MASK_MED register.
 */
__INLINE uint32_t la_trigger_mask_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_MASK_MED register to a value.
 * The TRIGGER_MASK_MED register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_mask_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_MASK_MED field mask
#define LA_TRIGGER_MASK_MED_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_MASK_MED field LSB position
#define LA_TRIGGER_MASK_MED_LSB    0
/// TRIGGER_MASK_MED field width
#define LA_TRIGGER_MASK_MED_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_MASK_MED field reset value
#define LA_TRIGGER_MASK_MED_RST    0x0

/**
 * @brief Returns the current value of the trigger_mask_med field in the TRIGGER_MASK_MED register.
 *
 * The TRIGGER_MASK_MED register will be read and the trigger_mask_med field's value will be returned.
 *
 * @return The current value of the trigger_mask_med field in the TRIGGER_MASK_MED register.
 */
__INLINE uint32_t la_trigger_mask_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_mask_med field of the TRIGGER_MASK_MED register.
 *
 * The TRIGGER_MASK_MED register will be read, modified to contain the new field value, and written.
 *
 * @param[in] triggermaskmed - The value to set the field to.
 */
__INLINE void la_trigger_mask_med_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermaskmed)
{
    ASSERT_ERR((((uint32_t)triggermaskmed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermaskmed << 0);
}

/// @}

/**
 * @name TRIGGER_MASK_HIGH register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    trigger_mask_high   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_MASK_HIGH register
#define LA_TRIGGER_MASK_HIGH_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0028)
/// Offset of the TRIGGER_MASK_HIGH register from the base address
#define LA_TRIGGER_MASK_HIGH_OFFSET 0x00000028
/// Index of the TRIGGER_MASK_HIGH register
#define LA_TRIGGER_MASK_HIGH_INDEX  0x0000000A
/// Reset value of the TRIGGER_MASK_HIGH register
#define LA_TRIGGER_MASK_HIGH_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_MASK_HIGH register.
 * The TRIGGER_MASK_HIGH register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_MASK_HIGH register.
 */
__INLINE uint32_t la_trigger_mask_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_MASK_HIGH register to a value.
 * The TRIGGER_MASK_HIGH register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_mask_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_MASK_HIGH field mask
#define LA_TRIGGER_MASK_HIGH_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_MASK_HIGH field LSB position
#define LA_TRIGGER_MASK_HIGH_LSB    0
/// TRIGGER_MASK_HIGH field width
#define LA_TRIGGER_MASK_HIGH_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_MASK_HIGH field reset value
#define LA_TRIGGER_MASK_HIGH_RST    0x0

/**
 * @brief Returns the current value of the trigger_mask_high field in the TRIGGER_MASK_HIGH register.
 *
 * The TRIGGER_MASK_HIGH register will be read and the trigger_mask_high field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_mask_high field in the TRIGGER_MASK_HIGH register.
 */
__INLINE uint32_t la_trigger_mask_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_mask_high field of the TRIGGER_MASK_HIGH register.
 *
 * The TRIGGER_MASK_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggermaskhigh - The value to set the field to.
 */
__INLINE void la_trigger_mask_high_setf(struct siwifi_hw *siwifi_hw, uint32_t triggermaskhigh)
{
    ASSERT_ERR((((uint32_t)triggermaskhigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_MASK_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggermaskhigh << 0);
}

/// @}

/**
 * @name TRIGGER_VALUE_LOW register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    trigger_value_low   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_VALUE_LOW register
#define LA_TRIGGER_VALUE_LOW_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x002C)
/// Offset of the TRIGGER_VALUE_LOW register from the base address
#define LA_TRIGGER_VALUE_LOW_OFFSET 0x0000002C
/// Index of the TRIGGER_VALUE_LOW register
#define LA_TRIGGER_VALUE_LOW_INDEX  0x0000000B
/// Reset value of the TRIGGER_VALUE_LOW register
#define LA_TRIGGER_VALUE_LOW_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_VALUE_LOW register.
 * The TRIGGER_VALUE_LOW register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_VALUE_LOW register.
 */
__INLINE uint32_t la_trigger_value_low_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_VALUE_LOW register to a value.
 * The TRIGGER_VALUE_LOW register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_value_low_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_VALUE_LOW field mask
#define LA_TRIGGER_VALUE_LOW_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_VALUE_LOW field LSB position
#define LA_TRIGGER_VALUE_LOW_LSB    0
/// TRIGGER_VALUE_LOW field width
#define LA_TRIGGER_VALUE_LOW_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_VALUE_LOW field reset value
#define LA_TRIGGER_VALUE_LOW_RST    0x0

/**
 * @brief Returns the current value of the trigger_value_low field in the TRIGGER_VALUE_LOW register.
 *
 * The TRIGGER_VALUE_LOW register will be read and the trigger_value_low field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_value_low field in the TRIGGER_VALUE_LOW register.
 */
__INLINE uint32_t la_trigger_value_low_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_value_low field of the TRIGGER_VALUE_LOW register.
 *
 * The TRIGGER_VALUE_LOW register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggervaluelow - The value to set the field to.
 */
__INLINE void la_trigger_value_low_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluelow)
{
    ASSERT_ERR((((uint32_t)triggervaluelow << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_LOW_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluelow << 0);
}

/// @}

/**
 * @name TRIGGER_VALUE_MED register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00    trigger_value_med   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_VALUE_MED register
#define LA_TRIGGER_VALUE_MED_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0030)
/// Offset of the TRIGGER_VALUE_MED register from the base address
#define LA_TRIGGER_VALUE_MED_OFFSET 0x00000030
/// Index of the TRIGGER_VALUE_MED register
#define LA_TRIGGER_VALUE_MED_INDEX  0x0000000C
/// Reset value of the TRIGGER_VALUE_MED register
#define LA_TRIGGER_VALUE_MED_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_VALUE_MED register.
 * The TRIGGER_VALUE_MED register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_VALUE_MED register.
 */
__INLINE uint32_t la_trigger_value_med_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_VALUE_MED register to a value.
 * The TRIGGER_VALUE_MED register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_value_med_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_VALUE_MED field mask
#define LA_TRIGGER_VALUE_MED_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_VALUE_MED field LSB position
#define LA_TRIGGER_VALUE_MED_LSB    0
/// TRIGGER_VALUE_MED field width
#define LA_TRIGGER_VALUE_MED_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_VALUE_MED field reset value
#define LA_TRIGGER_VALUE_MED_RST    0x0

/**
 * @brief Returns the current value of the trigger_value_med field in the TRIGGER_VALUE_MED register.
 *
 * The TRIGGER_VALUE_MED register will be read and the trigger_value_med field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_value_med field in the TRIGGER_VALUE_MED register.
 */
__INLINE uint32_t la_trigger_value_med_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_value_med field of the TRIGGER_VALUE_MED register.
 *
 * The TRIGGER_VALUE_MED register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggervaluemed - The value to set the field to.
 */
__INLINE void la_trigger_value_med_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluemed)
{
    ASSERT_ERR((((uint32_t)triggervaluemed << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_MED_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluemed << 0);
}

/// @}

/**
 * @name TRIGGER_VALUE_HIGH register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  31:00   trigger_value_high   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_VALUE_HIGH register
#define LA_TRIGGER_VALUE_HIGH_ADDR(band)  (REG_LA_BASE_ADDR(band)+0x0034)
/// Offset of the TRIGGER_VALUE_HIGH register from the base address
#define LA_TRIGGER_VALUE_HIGH_OFFSET 0x00000034
/// Index of the TRIGGER_VALUE_HIGH register
#define LA_TRIGGER_VALUE_HIGH_INDEX  0x0000000D
/// Reset value of the TRIGGER_VALUE_HIGH register
#define LA_TRIGGER_VALUE_HIGH_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_VALUE_HIGH register.
 * The TRIGGER_VALUE_HIGH register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_VALUE_HIGH register.
 */
__INLINE uint32_t la_trigger_value_high_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_VALUE_HIGH register to a value.
 * The TRIGGER_VALUE_HIGH register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_value_high_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_VALUE_HIGH field mask
#define LA_TRIGGER_VALUE_HIGH_MASK   ((uint32_t)0xFFFFFFFF)
/// TRIGGER_VALUE_HIGH field LSB position
#define LA_TRIGGER_VALUE_HIGH_LSB    0
/// TRIGGER_VALUE_HIGH field width
#define LA_TRIGGER_VALUE_HIGH_WIDTH  ((uint32_t)0x00000020)

/// TRIGGER_VALUE_HIGH field reset value
#define LA_TRIGGER_VALUE_HIGH_RST    0x0

/**
 * @brief Returns the current value of the trigger_value_high field in the TRIGGER_VALUE_HIGH register.
 *
 * The TRIGGER_VALUE_HIGH register will be read and the trigger_value_high field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_value_high field in the TRIGGER_VALUE_HIGH register.
 */
__INLINE uint32_t la_trigger_value_high_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0xFFFFFFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_value_high field of the TRIGGER_VALUE_HIGH register.
 *
 * The TRIGGER_VALUE_HIGH register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggervaluehigh - The value to set the field to.
 */
__INLINE void la_trigger_value_high_setf(struct siwifi_hw *siwifi_hw, uint32_t triggervaluehigh)
{
    ASSERT_ERR((((uint32_t)triggervaluehigh << 0) & ~((uint32_t)0xFFFFFFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_VALUE_HIGH_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggervaluehigh << 0);
}

/// @}

/**
 * @name TRIGGER_POINT register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00        trigger_point   0x0
 * </pre>
 *
 * @{
 */

/// Address of the TRIGGER_POINT register
#define LA_TRIGGER_POINT_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x0038)
/// Offset of the TRIGGER_POINT register from the base address
#define LA_TRIGGER_POINT_OFFSET 0x00000038
/// Index of the TRIGGER_POINT register
#define LA_TRIGGER_POINT_INDEX  0x0000000E
/// Reset value of the TRIGGER_POINT register
#define LA_TRIGGER_POINT_RESET  0x00000000

/**
 * @brief Returns the current value of the TRIGGER_POINT register.
 * The TRIGGER_POINT register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the TRIGGER_POINT register.
 */
__INLINE uint32_t la_trigger_point_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb));
}

/**
 * @brief Sets the TRIGGER_POINT register to a value.
 * The TRIGGER_POINT register will be written.
 * @param[in] elt_idx Index of the HW block
 * @param value - The value to write.
 */
__INLINE void la_trigger_point_set(struct siwifi_hw *siwifi_hw, uint32_t value)
{
    REG_PL_WR(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb), value);
}

// field definitions
/// TRIGGER_POINT field mask
#define LA_TRIGGER_POINT_MASK   ((uint32_t)0x0000FFFF)
/// TRIGGER_POINT field LSB position
#define LA_TRIGGER_POINT_LSB    0
/// TRIGGER_POINT field width
#define LA_TRIGGER_POINT_WIDTH  ((uint32_t)0x00000010)

/// TRIGGER_POINT field reset value
#define LA_TRIGGER_POINT_RST    0x0

/**
 * @brief Returns the current value of the trigger_point field in the TRIGGER_POINT register.
 *
 * The TRIGGER_POINT register will be read and the trigger_point field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the trigger_point field in the TRIGGER_POINT register.
 */
__INLINE uint16_t la_trigger_point_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief Sets the trigger_point field of the TRIGGER_POINT register.
 *
 * The TRIGGER_POINT register will be read, modified to contain the new field value, and written.
 *
 * @param[in] elt_idx Index of the HW block
 * @param[in] triggerpoint - The value to set the field to.
 */
__INLINE void la_trigger_point_setf(struct siwifi_hw *siwifi_hw, uint16_t triggerpoint)
{
    ASSERT_ERR((((uint32_t)triggerpoint << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(LA_TRIGGER_POINT_ADDR(siwifi_hw->mod_params->is_hb), (uint32_t)triggerpoint << 0);
}

/// @}

/**
 * @name FIRSTSAMPLE register definitions
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00          firstsample   0x0
 * </pre>
 *
 * @{
 */

/// Address of the FIRSTSAMPLE register
#define LA_FIRSTSAMPLE_ADDR(band)   (REG_LA_BASE_ADDR(band)+0x003C)
/// Offset of the FIRSTSAMPLE register from the base address
#define LA_FIRSTSAMPLE_OFFSET 0x0000003C
/// Index of the FIRSTSAMPLE register
#define LA_FIRSTSAMPLE_INDEX  0x0000000F
/// Reset value of the FIRSTSAMPLE register
#define LA_FIRSTSAMPLE_RESET  0x00000000

/**
 * @brief Returns the current value of the FIRSTSAMPLE register.
 * The FIRSTSAMPLE register will be read and its value returned.
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the FIRSTSAMPLE register.
 */
__INLINE uint32_t la_firstsample_get(struct siwifi_hw *siwifi_hw)
{
    return REG_PL_RD(LA_FIRSTSAMPLE_ADDR(siwifi_hw->mod_params->is_hb));
}

// field definitions
/// FIRSTSAMPLE field mask
#define LA_FIRSTSAMPLE_MASK   ((uint32_t)0x0000FFFF)
/// FIRSTSAMPLE field LSB position
#define LA_FIRSTSAMPLE_LSB    0
/// FIRSTSAMPLE field width
#define LA_FIRSTSAMPLE_WIDTH  ((uint32_t)0x00000010)

/// FIRSTSAMPLE field reset value
#define LA_FIRSTSAMPLE_RST    0x0

/**
 * @brief Returns the current value of the firstsample field in the FIRSTSAMPLE register.
 *
 * The FIRSTSAMPLE register will be read and the firstsample field's value will be returned.
 *
 * @param[in] elt_idx Index of the HW block
 * @return The current value of the firstsample field in the FIRSTSAMPLE register.
 */
__INLINE uint16_t la_firstsample_getf(struct siwifi_hw *siwifi_hw)
{
    uint32_t localVal = REG_PL_RD(LA_FIRSTSAMPLE_ADDR(siwifi_hw->mod_params->is_hb));
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

/// @}


#endif // _REG_LA_H_

/// @}
