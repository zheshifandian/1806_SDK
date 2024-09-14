/*******************************************************************************
*                                                                              *
*  Copyright (c), 2023, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

typedef unsigned int    uint32_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef int             int32_t;

#define SMI_ST_CODE   0x1
#define SMI_OP_READ   0x2
#define SMI_OP_WRITE  0x1
#define SMI_TA_CODE   0x2

/* porting the follow macro or function according to the platform */
#define GPIO_DIR_IN     0
#define GPIO_DIR_OUT    1

/* GPIO used for SMI data */
#define GPIO_MDIO       0
/* GPIO used for SMI clock */
#define GPIO_MDC        1
#define DELAY           100

static void smi_lock(void)
{
    return;
}

static void smi_unlock(void)
{
    return;
}

static inline void smi_delay(uint32_t delay)
{
    /*
     According to the protocol requirements, it is recommended that
     the minimum value of MDC High Pulse Width is 160ns
     the minimum value of MDC Low Pulse Width is 160ns
     the minimum value of MDC Period is 400ns
    */
    return;
}

static int32_t gpio_set_dir(uint32_t gpioId, uint8_t dir)
{
    return 0;
}

static int32_t gpio_write_bit(uint32_t gpioId, uint16_t data)
{
    return 0;
}

static int32_t gpio_read_bit(uint32_t gpioId, uint16_t *data)
{
    return 0;
}
/* porting the above macro or function according to the platform */

void smi_write_bits(uint16_t data, uint32_t len)
{
    gpio_set_dir(GPIO_MDC, GPIO_DIR_OUT);
    gpio_set_dir(GPIO_MDIO, GPIO_DIR_OUT);
    for( ; len > 0; len--)
    {
        gpio_write_bit(GPIO_MDC, 0);
        smi_delay(DELAY);
        if (data & (1 << (len - 1)))
            gpio_write_bit(GPIO_MDIO, 1);
        else
            gpio_write_bit(GPIO_MDIO, 0);
        smi_delay(DELAY);

        gpio_write_bit(GPIO_MDC, 1);
        smi_delay(DELAY);
    }
    gpio_write_bit(GPIO_MDC, 0);
}

void smi_read_bits(uint32_t len, uint16_t *data)
{
    uint16_t tmp = 0;

    gpio_set_dir(GPIO_MDIO, GPIO_DIR_IN);
    for (*data = 0; len > 0; len--)
    {
        gpio_read_bit(GPIO_MDIO, &tmp);
        *data |= ((tmp & 0x1) << (len - 1));
        smi_delay(DELAY);
        gpio_write_bit(GPIO_MDC, 1);
        smi_delay(DELAY);
        gpio_write_bit(GPIO_MDC, 0);
        smi_delay(DELAY);
    }
}

uint32_t smi_write_cl22(uint8_t phyAddr, uint8_t regAddr, uint16_t regVal)
{
    smi_lock();
    smi_write_bits(0xffff, 16);
    smi_write_bits(0xffff, 16);
    smi_write_bits(SMI_ST_CODE, 2);
    smi_write_bits(SMI_OP_WRITE, 2);
    smi_write_bits(phyAddr, 5);
    smi_write_bits(regAddr, 5);
    smi_write_bits(SMI_TA_CODE, 2);
    smi_write_bits(regVal, 16);
    smi_unlock();

    return 0;
}

uint32_t smi_read_cl22(uint8_t phyAddr, uint8_t regAddr, uint16_t *pRegVal)
{
    smi_lock();
    smi_write_bits(0xffff, 16);
    smi_write_bits(0xffff, 16);
    smi_write_bits(SMI_ST_CODE, 2);
    smi_write_bits(SMI_OP_READ, 2);
    smi_write_bits(phyAddr, 5);
    smi_write_bits(regAddr, 5);
    smi_read_bits(2, pRegVal);
    smi_read_bits(16, pRegVal);
    smi_unlock();

    return 0;
}
