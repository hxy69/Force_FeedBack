/**
 * @file    bsp_delay.c
 * @brief   微秒级延时函数源文件（基于DWT Cycle Counter）
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @note    该文件适用于GenBotter Motor-1电机开发板，STM32F407IGT6。
 *          使用DWT硬件周期计数器，不受中断影响，精度为1个CPU时钟周期（~6ns@168MHz）。
 */
#include "bsp_delay.h"


/**
  * @brief  初始化DWT Cycle Counter，用于delay_us()
  * @note   必须在 SystemClock_Config() 之后调用，否则 SystemCoreClock 未就绪
  */
void DWT_Init(void)
{
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
  * @brief  微秒级延时（阻塞）
  * @param  us: 延时的微秒数
  * @note   基于 DWT->CYCCNT 轮询实现，不受中断影响，但也不支持嵌套精确计时。
  */
void delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - startTick) < delayTicks)
    {
        __NOP();
    }
}
