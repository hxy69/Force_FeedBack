#include "speed_loop.h"
#include "control_config.h"
#include "pid_controller.h"
#include "bsp_brush_motor.h"
#include "bsp_encoder.h"
#include <math.h> 

// 相对于brush_motor_7新增
#include "vofa_plus.h" // 引入VOFA+头文件，集成调试功能

/* --- 全局变量定义 --- */
PID_Handle_t hspeed_pid;                // 定义 PID 句柄实例
static float filtered_rpm = 0.0f;       // 一阶低通滤波器状态变量，存储上一次滤波结果，初始为0
static float target_rpm = 0.0f;         // 相对brush_motor_7新增：当前目标转速，供外部访问（如VOFA+显示）
float final_output = 0.0f;              // 相对brush_motor_7新增：全局变量，存储死区补偿+限幅后的最终输出，供VOFA+显示

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
    
    filtered_rpm = 0.0f; // 初始化滤波器状态，避免初始值异常
    BrushMotor_Stop();   // 初始状态强制电机停止
    target_rpm = 0.0f;   // 初始化目标转速
    final_output = 0.0f; // 初始化最终输出
}

/**
 * @brief  设置目标速度 (RPM)
 * @param  target_rpm: 目标转速
 */
void SpeedLoop_SetTargetRPM(float rpm)
{
    target_rpm = target_rpm; // 同步更新全局变量
    hspeed_pid.Target = rpm; // 更新 PID 目标值
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

    // 2. 一阶低通滤波（相对brush_motor_6新增）：滤除编码器高频噪声，消除电机滋滋声
    // 公式：filtered = α * raw + (1 - α) * filtered_prev，α：0.1~0.3，越小越平滑但响应越慢，平衡响应和稳定性
    filtered_rpm = RPM_FILTER_ALPHA * raw_rpm + (1.0f - RPM_FILTER_ALPHA) * filtered_rpm;

    // 3. PID 计算（位置式，调用优化后的PID_Compute函数）
    float pid_output = PID_Compute(&hspeed_pid, hspeed_pid.Target, filtered_rpm, PID_MODE_POSITIONAL);

    // 4. 死区补偿（相对brush_motor_6新增）：小输出可能无法克服电机静摩擦，导致实际转速为0，需要补偿
    if (fabsf(pid_output) > PID_DEADZONE_THRESH){
        final_output = pid_output + MOTOR_DEAD_ZONE; // 正向旋转，叠加正补偿
    }else if (pid_output < -PID_DEADZONE_THRESH){
        final_output = pid_output - MOTOR_DEAD_ZONE; // 反向旋转，叠加负补偿
    }else{
        final_output = 0.0f; // 输出在死区范围内，直接设为0，防止微小输出导致电机发热
    }

    // 5. 补偿后二次限幅（保护电机）：防止补偿值导致PWM占空比超出上限
    if(final_output > PID_OUTPUT_MAX){
        final_output = PID_OUTPUT_MAX;
    }else if(final_output < PID_OUTPUT_MIN){
        final_output = PID_OUTPUT_MIN;
    }

    // 6. 执行电机驱动 (将 PID 计算的 float 结果转为 PWM 和 方向)
    uint32_t duty_cycle = 0;
    
    if (final_output >= 0)
    {
        BrushMotor_SetDirection(MOTOR_FORWARD); // 设定为正向
        duty_cycle = (uint8_t)final_output;      // 转换为占空比数值
    }
    else
    {
        BrushMotor_SetDirection(MOTOR_REVERSE); // 设定为反向
        duty_cycle = (uint8_t)(-final_output);   // 取绝对值作为占空比
    }
    
    BrushMotor_SetSpeed(duty_cycle); // 设置占空比
}

/**
 * @brief  获取当前速度环调试数据
 * @return 包含目标转速、实际转速、PID输出、最终输出的结构体
 */
SpeedLoop_DebugData_t SpeedLoop_GetDebugData(void)
{
    SpeedLoop_DebugData_t debug_data;
    debug_data.target_rpm = hspeed_pid.Target; // 目标转速
    debug_data.actual_rpm = hspeed_pid.Actual; // 实际转速（滤波后的值）
    debug_data.pid_output = hspeed_pid.Output; // PID原始输出
    debug_data.final_output = final_output;     // 死区补偿+限幅后的最终输出
    return debug_data;
}
