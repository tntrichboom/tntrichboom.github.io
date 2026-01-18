#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "main.h"


/* SCL: PB6, SDA: PB7 */
#define SCL_PORT    GPIOB
#define SCL_PIN     GPIO_Pin_6
#define SDA_PORT    GPIOB
#define SDA_PIN     GPIO_Pin_7

#define SCL_HIGH()  GPIO_SetBits(SCL_PORT, SCL_PIN)
#define SCL_LOW()   GPIO_ResetBits(SCL_PORT, SCL_PIN)
#define SDA_HIGH()  GPIO_SetBits(SDA_PORT, SDA_PIN)
#define SDA_LOW()   GPIO_ResetBits(SDA_PORT, SDA_PIN)
#define SDA_READ()  GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN)
#define DELAY()     i2c_delay()

/**
 * @brief I2C延迟，用于模拟I2C的速度
 *
 */
static void i2c_delay(void)
{
   
    delay_us(5);
}

/**
 * @brief I2C开始信号，在SCL高电平期间，SDA产生一个下降沿，其后SCL拉低
 *
 */
static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    DELAY();
    SDA_LOW();
    DELAY();
    SCL_LOW();
}

/**
 * @brief I2C停止信号，在SCL高电平期间，SDA产生一个上升沿
 *
 */
static void i2c_stop(void)
{
    SDA_LOW();
    DELAY();
    SCL_HIGH();
    DELAY();
    SDA_HIGH();
    DELAY();
}

/**
 * @brief I2C写一个字节
 *
 * @param data 要写入的数据
 * @return true 写入成功
 * @return false 写入失败
 */
static bool i2c_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        /* MSB先发送 */
        if (data & 0x80)
            SDA_HIGH();
        else
            SDA_LOW();
        DELAY();
        /* SCL拉高时，SDA数据被读取 */
        SCL_HIGH();
        DELAY();
        /* SCL拉低，准备发送下一位数据 */
        SCL_LOW();
        data <<= 1;
    }
    /* SDA拉高，等待ACK */
    SDA_HIGH();
    DELAY();
    /* SCL拉高，读取ACK */
    SCL_HIGH();
    DELAY();
    /* 检测ACK */
    if (SDA_READ())
    {
        SCL_LOW();
        return false;
    }
    SCL_LOW();
    DELAY();
    return true;
}

/**
 * @brief I2C读一个字节
 *
 * @param ack 是否发送ACK应答位
 * @return uint8_t 读取到的数据
 */
static uint8_t i2c_read_byte(bool ack)
{
    uint8_t data = 0;
    /* 读取数据，SDA先拉高，等待从机发送数据 */
    SDA_HIGH();
    for (uint8_t i = 0; i < 8; i++)
    {
        /* 数据左移，最先收到的数据先加载到最后一位，随着8次左移，就被移到最高位 */
        data <<= 1;
        /* SCL拉高，读取数据 */
        SCL_HIGH();
        DELAY();
        /* 读取数据 */
        if (SDA_READ())
            data |= 0x01;
        /* SCL拉低，准备读取下一位数据 */
        SCL_LOW();
        DELAY();
    }
    /* 发送ACK */
    if (ack)
        SDA_LOW();
    else
        SDA_HIGH();
    DELAY();
    /* SCL拉高，发送ACK */
    SCL_HIGH();
    DELAY();
    SCL_LOW();
    SDA_HIGH();
    DELAY();
    return data;
}

/**
 * @brief I2C的IO初始化
 *
 */
static void i2c_io_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

/**
 * @brief I2C初始化
 *
 */
void swi2c_init(void)
{
    i2c_io_init();
}

/**
 * @brief I2C写数据
 *
 * @param addr 从机地址
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @param length 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool swi2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    /* 发送开始信号 */
    i2c_start();

    /* 发送从机地址 */
    if (!i2c_write_byte(addr << 1))
    {
        i2c_stop();
        return false;
    }

    /* 发送寄存器地址 */
    i2c_write_byte(reg);

    /* 发送数据 */
    for (uint16_t i = 0; i < length; i++)
    {
        i2c_write_byte(data[i]);
    }

    /* 发送停止信号 */
    i2c_stop();

    return true;
}

/**
 * @brief I2C读数据
 *
 * @param addr 从机地址
 * @param reg 寄存器地址
 * @param data 读取到的数据
 * @param length 数据长度
 * @return true 读取成功
 * @return false 读取失败
 */
bool swi2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    /* 发送开始信号 */
    i2c_start();

    /* 发送从机地址 */
    if (!i2c_write_byte(addr << 1))
    {
        i2c_stop();
        return false;
    }

    /* 发送寄存器地址 */
    i2c_write_byte(reg);

    /* 发送重复开始信号 */
    i2c_start();

    /* 发送从机地址 */
    i2c_write_byte((addr << 1) | 0x01);

    /* 读取数据 */
    for (uint16_t i = 0; i < length; i++)
    {
        data[i] = i2c_read_byte(i == length - 1 ? false : true);
    }

    /* 发送停止信号 */
    i2c_stop();

    return true;
}
