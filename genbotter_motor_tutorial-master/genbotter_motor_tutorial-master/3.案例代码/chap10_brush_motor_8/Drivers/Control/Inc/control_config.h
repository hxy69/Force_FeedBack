/**
 * @file    control_config.h
 * @brief   速度环PID控制配置文件
 * @author  Dr. GAO
 * @date    2025-3-2
 * @version V1.1
 * @website https://genbotter.taobao.com
 * @bilibili https://space.bilibili.com/486637340
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，PID速度环控制的配置文件
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __CONTROL_CONFIG_H
#define __CONTROL_CONFIG_H

/* ================= 硬件物理参数 ================= */
// 控制周期：10ms (必须与 main.c 中 TIM6 的判断逻辑一致)
#define CTRL_PERIOD_MS          10.0f 
#define CTRL_PERIOD_S           0.01f  

// 电机死区补偿 (0-100)
// 解释：PWM < 5% 时电机可能只有电流不转，需要切断防止发热
#define MOTOR_DEAD_ZONE         5.0f  

/* ================= PID 核心参数 ================= */
// 调试思路（有刷电机速度环）：
// 1. 先调KP：从0开始增大，直到电机响应快且无明显超调
// 2. 再调KI：增大KI消除静态误差，注意不要超调过大
// 3. 最后调KD：仅负载惯性极大时少量增加，否则设为0
#define SPEED_PID_KP            0.6f  
#define SPEED_PID_KI            0.2f 
#define SPEED_PID_KD            0.02f  // 速度环通常不需要 D，除非负载惯性极大

/* ================= 输出限制 ================= */
// 你的 bsp_brush_motor.c 里 SetSpeed 接收 0-100
#define PID_OUTPUT_MAX          100.0f
#define PID_OUTPUT_MIN          -100.0f

// 积分限幅 (Anti-Windup)
// 说明：此处是「积分项输出上限」（Ki*∑e(n)），而非积分累加值∑e(n)的上限
// 作用：防止堵转时I项无限累加，导致电机恢复后“飞车”
#define PID_INTEGRAL_MAX        90.0f

/* ================= 工程优化参数（相对于brush_motor_6新增）================= */
#define RPM_FILTER_ALPHA        0.2f    // 转速低通滤波系数(0.1~0.3)：越小越平滑，响应越慢
#define PID_INTEG_SEP_THRESH    30.0f   // 积分分离阈值(RPM)：误差超该值，停止积分
#define PID_DEADZONE_THRESH     0.1f    // 死区补偿触发阈值：PID输出超该值才补偿

#endif
