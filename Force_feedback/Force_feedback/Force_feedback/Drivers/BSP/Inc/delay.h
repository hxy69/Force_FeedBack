#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

    void DWT_Init(void);        /* 初始化DWT */
    void delay_us(uint32_t us); /* 微秒级延时函数 */

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
