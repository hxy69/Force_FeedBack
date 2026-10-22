#include "delay.h"


/**
  * @brief  Initialize DWT Cycle Counter for delay_us()
  * @note   Must be called after SystemClock_Config()
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
  * @brief  Microsecond delay using DWT cycle counter
  * @param  us: delay time in microseconds
  * @note   SystemCoreClock must be correctly configured
  */
void delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - startTick) < delayTicks)
    {
        __NOP(); // 可选：防止编译器过度优化
    }
}
