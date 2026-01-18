#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "lcd_spi.h"


/* LCD SPI 写数据完成回调函数 */
static lcd_spi_send_finish_callback_t lcd_spi_send_finish_callback;


/**
 * @brief LCD SPI GPIO 初始化
 *
 */
static void lcd_spi_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* SPI1 SCK */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);  /* idle时候为高电平 */

    /* SPI1 MOSI */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOA, GPIO_Pin_7, Bit_SET);  /* idle时候为高电平 */

    /* 不需要初始化SPI1 MISO，因为我们只是写数据到LCD，不需要读数据 */
    /* 不需要初始化SPI1 NSS，因为我们使用软件NSS */
}

/**
 * @brief LCD SPI NVIC 初始化，配置SPI1的DMA发送完成中断
 *
 */
static void lcd_spi_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;   /* 优先级设置为5（随意设置均可），取值范围从0~15中任意值 */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief LCD 硬件SPI初始化
 *
 */
static void lcd_spi_lowlevel_init(void)
{
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;   /* 只发送数据，不接收数据 */
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;     /* 模式3，时钟空闲时为高电平 */
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;     /* 软件NSS */
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  /* spi clock = 72MHz / 4 = 18MHz */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);    /* 使能SPI1的DMA发送请求 */
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief 初始化LCD的硬件SPI，使用DMA+中断方式
 *
 */
void lcd_spi_init(void)
{
    lcd_spi_gpio_init();
    lcd_spi_nvic_init();
    lcd_spi_lowlevel_init();
}

/**
 * @brief 使用硬件SPI发送数据
 *
 * @param data 待发送数据
 * @param length 待发送数据长度
 */
void lcd_spi_write(uint8_t *data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
        SPI_I2S_SendData(SPI1, data[i]);
    }
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}

/**
 * @brief 使用硬件SPI发送数据，使用DMA方式
 *
 * @param data
 * @param length
 */
void lcd_spi_write_async(uint8_t *data, uint16_t length)
{
    DMA_DeInit(DMA1_Channel3);
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR; /* 目的地址 */
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)data;  /* 源地址 */
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  /* 从内存到外设 */
    DMA_InitStructure.DMA_BufferSize = length;  /* DMA发起length次传输 */
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;    /* 外设地址不自增 */
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;     /* 内存地址自增 */
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; /* 外设单次传输1字节 */
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;  /* 内存数据宽度为8位 */
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;   /* 正常模式，不是循环模式 */
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;   /* 中等优先级 */
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;    /* 不是内存到内存传输模式 */
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);   /* 使能DMA1通道3传输完成中断 */
    DMA_Cmd(DMA1_Channel3, ENABLE);     /* 启动DMA传输，开始发送数据 */
}

/**
 * @brief 注册LCD SPI-DMA发送完成回调函数
 *
 * @param callback 回调函数
 */
void lcd_spi_send_finish_register(lcd_spi_send_finish_callback_t callback)
{
    lcd_spi_send_finish_callback = callback;
}

/**
 * @brief DMA1通道3中断服务函数
 *
 */
void DMA1_Channel3_IRQHandler(void)
{
    /* 当DMA1通道3传输完成时，会进入这个中断服务函数 */
    if (DMA_GetITStatus(DMA1_IT_TC3) == SET)
    {
        /* 等待SPI1发送完成，通常TC标志位置位之后BSY已经被清零 */
        /* 下面这句代码参考了SDK里面的官方sample代码，出于稳定考虑，我也加进来 */
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

        /* 如果注册了回调函数，即lcd_spi_send_finish_callback不为null，调用回调函数 */
        if (lcd_spi_send_finish_callback)
            lcd_spi_send_finish_callback();

        /* 清除DMA1通道3传输完成中断标志位，TC标志位不会自动复位 */
        DMA_ClearITPendingBit(DMA1_IT_TC3);
    }
}
