/**
 * @file    user_config.h
 * @brief   双环控制用户配置参数
 * @author  Dr. GAO
 * @date    2026-07-20
 * @version V1.0
 * @website https://genbotter.taobao.com
 *
 * @note    本文件集中管理所有可调参数，方便学生对照课件调参。
 *          修改参数后重新编译即可生效，无需改动控制逻辑代码。
 *
 *          三步调参法（对照 chap13-03 课件）：
 *          第一步：CURRENT_ONLY 模式，只调 CURRENT_KP/CURRENT_KI
 *          第二步：FULL 模式，先调 SPEED_KP（KI=KD=0）
 *          第三步：加 SPEED_KI 消静差，可选 SPEED_KD 抑超调
 */

#ifndef __USER_CONFIG_H
#define __USER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 1. 系统调度参数
 *============================================================================*/

/**
 * @brief 速度环分频比 = 控制周期倍数
 * @note  电流环 = TIM1 溢出频率 = 1kHz（1ms）
 *        速度环 = 电流环 / SPEED_DIVIDER = 1kHz / 10 = 100Hz（10ms）
 *
 *        设计要求：内环比外环快 5~10 倍以上
 *        1kHz / 100Hz = 10 倍 ✓
 */
#define SPEED_DIVIDER   10u

/*==============================================================================
 * 2. 电流环 PI 参数（内环，1kHz）
 *============================================================================*/

/**
 * @brief 比例系数 Kp（mA → PWM%）
 * @note  100mA误差→0.8%占空比。P项指方向，积分主导输出
 */
#define CURRENT_KP               0.008f

/**
 * @brief 积分系数 Ki（mA → PWM%/步）
 * @note  100mA误差时：100ms→5%，500ms→25%，1s→50%
 */
#define CURRENT_KI               0.0005f

/**
 * @brief PID 输出限幅（PWM 占空比百分比）
 * @note  电流 PI 输出直接映射到 PWM 占空比 0~100%
 */
#define CURRENT_OUT_MAX          100.0f
#define CURRENT_OUT_MIN         -100.0f

/**
 * @brief 积分限幅（防止电流环积分 windup积分饱和）
 * @note  积分限幅 ≥ out_max / Ki，否则输出被锁死
 */
#define CURRENT_INTEGRAL_LIMIT   200000.0f  /**< 100/0.0005=200000 */

/**
 * @brief 电流环死区（PWM 百分比）
 * @note  |输出| < 死区 → PWM=0（不转），避免零附近抖动
 */
#define CURRENT_DEADZONE         2.0f

/*==============================================================================
 * 3. 速度环 PID 参数（外环，100Hz）
 *============================================================================*/

/**
 * @brief 比例系数 Kp（rpm → mA）
 * @note  5rpm误差→15mA，20rpm→60mA。温和响应，不撞限幅
 */
#define SPEED_KP                 3.0f

/**
 * @brief 积分系数 Ki（rpm→mA·步）
 * @note  5rpm误差：1s积7.5mA，10s积75mA。配合Kp=3.0消静差
 */
#define SPEED_KI                 0.015f

/**
 * @brief 微分系数 Kd（rpm/步 → mA）
 * @note  轻阻尼，±5rpm 噪声 → ±10mA，不过度
 */
#define SPEED_KD                 1.0f

/**
 * @brief 速度环输出限幅（电流给定值，mA）
 * @note  决定了最大加速/减速电流
 *        Motor-1 空载 140mA@100%占空比，堵转不超过 350mA
 *        设 350mA 即够用，硬件安全限 CURRENT_MAX_MA=350mA
 */
#define SPEED_OUT_MAX            350.0f
#define SPEED_OUT_MIN           -350.0f

/**
 * @brief 速度环积分限幅
 * @note  积分限幅 ≥ out_max / Ki，否则输出锁死
 *        0.3 / 0.001 = 300，设 500 留裕量
 */
#define SPEED_INTEGRAL_LIMIT     23333.0f  /**< 350/0.015≈23333 */

/*==============================================================================
 * 4. 电流安全限幅（硬件保护）
 *============================================================================*/

/**
 * @brief 电流给定安全范围（不论速度环输出多大，电流不超此值）
 * @note  这是双环控制的核心保护：外层 PID 输出被钳位，物理上不会过流
 *        Motor-1 有刷电机最大堵转电流约 500mA，设为 500mA 提供硬件保护
 */
#define CURRENT_MAX_MA           500.0f  /**< 硬件保护：电流给定最大 ±500mA */
#define CURRENT_MIN_MA          -500.0f

/*==============================================================================
 * 5. ADC 电流采样参数
 *============================================================================*/

#define CURRENT_OVERSAMPLE_COUNT  16u    /**< 过采样次数（ADC 16kHz×16→1kHz有效） */
#define CURRENT_LOWPASS_ALPHA     0.15f  /**< τ≈12.5ms，加强平滑减少卡顿 */
#define ADC_TO_MA_FACTOR          6.714f /**< ADC阶梯→mA换算（33mΩ×30倍×3.3V/4095） */

/*==============================================================================
 * 6. 串口调试开关
 *============================================================================*/

/**
 * @brief 串口调试输出使能（1=开，0=关）
 * @note  开启后每 ~500ms 输出速度/电流/误差到 USART1
 *        配合 VOFA+ FireWater 协议可实时查看波形
 */
#define SERIAL_DEBUG_ENABLE      1

#ifdef __cplusplus
}
#endif

#endif /* __USER_CONFIG_H */
