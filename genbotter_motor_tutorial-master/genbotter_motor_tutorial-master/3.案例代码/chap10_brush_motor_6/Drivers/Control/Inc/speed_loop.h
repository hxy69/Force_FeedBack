/**
 * @file    speed_loop.h
 * @brief   电机速度环控制头文件
 * @author  Dr. GAO
 * @date    2025-2-24
 * @version V1.0
 * @note    适配GenBotter Motor-1开发板，基于PID实现速度闭环控制
 */
#ifndef __SPEED_LOOP_H
#define __SPEED_LOOP_H

/**
 * @brief 初始化速度环控制器
 * @note 内部会初始化PID控制器、转速传感器等，需在系统启动时调用一次
 */
void SpeedLoop_Init(void);

/**
 * @brief 设置速度环目标转速
 * @param target_rpm 目标转速（单位：RPM），需在合理范围（0~200）
 */
void SpeedLoop_SetTargetRPM(float target_rpm); // 设置目标转速

/**
 * @brief 获取当前设置的目标转速
 * @return 当前目标转速（单位：RPM）
 */
float SpeedLoop_GetTargetRPM(void); // 获取目标转速

/**
 * @brief 获取电机实际转速
 * @return 传感器实测转速（单位：RPM），为滤波后的值（若有滤波）
 */
float SpeedLoop_GetActualRPM(void); // 获取实际转速

/**
 * @brief 速度环控制任务（核心）
 * @note 需在定时器中断或主循环中定期调用，调用周期建议10ms（100Hz）
 *       内部流程：读取实际转速→执行PID计算→输出控制信号（如PWM）
 */
void SpeedLoop_Task(void); // 速度环控制任务，需在定时器中断或主循环中定期调用

#endif
