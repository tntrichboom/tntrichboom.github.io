#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief 微妙级延时
 *
 * @param us 延时时间，单位微秒
 */
void delay_us(uint32_t us)
{
    /* 使能SysTick，时钟源为AHB时钟，即CPU时钟，计数值为us * (CPU频率 / 1000000) - 1 */
    SysTick->LOAD = us * (SystemCoreClock / 1000000) - 1;
    SysTick->VAL = 0;   /* 清空计数值 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; /* 使能SysTick，启动计数 */
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0); /* 等待计数完成 */
    SysTick->CTRL = 0;  /* 关闭SysTick */
}

/**
 * @brief 毫秒级延时
 *
 * @param ms 延时时间，单位毫秒
 */
void delay_ms(uint32_t ms)
{
    /* 循环调用微妙级延时函数，延时ms毫秒 */
    while (ms--)
    {
        delay_us(1000);
    }
}
