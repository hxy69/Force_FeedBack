#include "speed_loop.h"
#include "control_config.h"
#include "pid_controller.h"
#include "bsp_brush_motor.h"
#include "bsp_encoder.h"
#include <math.h> 

/* --- 全局变量定义 --- */
PID_Handle_t hspeed_pid;                // 定义 PID 句柄实例

/**
 * @brief  速度环初始化
 */
void SpeedLoop_Init(void)
{
    // 初始化 PID 参数：比例、积分、微分、输出上限、积分上限
    PID_Init(&hspeed_pid, 
             SPEED_PID_KP, 
             SPEED_PID_KI, 
             SPEED_PID_KD, 
             PID_OUTPUT_MAX, 
             PID_INTEGRAL_MAX);
             
    BrushMotor_Stop(); // 初始状态强制电机停止
}

/**
 * @brief  设置目标速度 (RPM)
 * @param  target_rpm: 目标转速
 */
void SpeedLoop_SetTargetRPM(float target_rpm)
{
    hspeed_pid.Target = target_rpm;
}

/**
 * @brief  获取当前目标转速
 */
float SpeedLoop_GetTargetRPM(void) 
{ 
    return hspeed_pid.Target; 
}

/**
 * @brief  获取当前实际转速 (PID 内部记录值)
 */
float SpeedLoop_GetActualRPM(void) 
{ 
    return hspeed_pid.Actual; 
}

/**
 * @brief  核心闭环任务 (需放入 TIM6 等 10ms 定时器中断调用)
 */
void SpeedLoop_Task(void)
{
    // 1. 获取反馈并纠正方向
    // 注意：发现原来BSP_Encoder_GetSpeedRPM的返回值与实际旋转方向相反，所以改变原来的函数乘以-1
    float raw_rpm = BSP_Encoder_GetSpeedRPM(ENCODER_PM1); 

    // 2. 计算 PID 输出
    // PID_MODE_INCREMENTAL: 增量式 (推荐用于速度控制)
    // PID_MODE_POSITIONAL: 位置式
    // float pid_output = PID_Compute(&hspeed_pid, hspeed_pid.Target, raw_rpm, PID_MODE_INCREMENTAL);//增量式使用本行代码
    // 位置式使用下面的代码：
    float pid_output = PID_Compute(&hspeed_pid, hspeed_pid.Target, raw_rpm, PID_MODE_POSITIONAL);

    // 3. 执行电机驱动 (将 PID 计算的 float 结果转为 PWM 和 方向)
    uint32_t duty_cycle = 0;
    
    if (pid_output >= 0)
    {
        BrushMotor_SetDirection(MOTOR_FORWARD); // 设定为正向
        duty_cycle = (uint32_t)pid_output;      // 转换为占空比数值
    }
    else
    {
        BrushMotor_SetDirection(MOTOR_REVERSE); // 设定为反向
        duty_cycle = (uint32_t)(-pid_output);   // 取绝对值作为占空比
    }
    
    // 设置 PWM (调用 bsp_brush_motor.c 接口)
    BrushMotor_SetSpeed(duty_cycle);
}
