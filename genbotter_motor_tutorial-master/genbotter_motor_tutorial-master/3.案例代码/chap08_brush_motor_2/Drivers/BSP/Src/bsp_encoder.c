/**
 * @file    bsp_encoder.c
 * @brief   直流有刷电机编码器测速*.c文件
 * @author  Dr. GAO
 * @date    2025-11-07
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1或PM2
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#include "bsp_encoder.h"
#include <math.h>

// 不确定是否启用htimX，所以弱定义
__weak TIM_HandleTypeDef htim3 = {0};
__weak TIM_HandleTypeDef htim2 = {0};
__weak TIM_HandleTypeDef htim6 = {0};

// 编码器状态
typedef struct
{
    int32_t overflow_count;  // 溢出计数
    int32_t last_count;      // 上次计数（用于速度计算）
    int32_t total_count;     // 总计数（停止时也保持）
    float rpm;               // 当前转速(RPM)
    uint8_t running;         // 编码器运行状态
    uint8_t available;       // 编码器是否可用
    TIM_HandleTypeDef *htim; // 对应的定时器句柄
} Encoder_t;

static Encoder_t encoders[2]; // PM1和PM2编码器


/**
 * @brief  Check if the timer is valid.
 * @param  htim Timer handle.
 * @return 1 if the timer is valid, 0 otherwise.
 * @note   A timer is valid if it is not NULL and its instance is not NULL.
 */
static uint8_t IsTimerValid(TIM_HandleTypeDef *htim)
{
    return (htim != NULL && htim->Instance != NULL);
}

/**
 * @brief   初始化编码器模块
 * @note   该函数将初始化PM1和PM2编码器，
 *          并启动速度计算定时器（50ms中断）
 * @return  无
 */
void BSP_Encoder_Init(void)
{
    // 初始化PM1编码器
    encoders[ENCODER_PM1].htim = &htim3;
    encoders[ENCODER_PM1].available = IsTimerValid(&htim3);
    encoders[ENCODER_PM1].running = 0;
    encoders[ENCODER_PM1].total_count = 0;
    encoders[ENCODER_PM1].last_count = 0;
    encoders[ENCODER_PM1].overflow_count = 0;
    encoders[ENCODER_PM1].rpm = 0.0f;

    // 初始化PM2编码器
    encoders[ENCODER_PM2].htim = &htim2;
    encoders[ENCODER_PM2].available = IsTimerValid(&htim2);
    encoders[ENCODER_PM2].running = 0;
    encoders[ENCODER_PM2].total_count = 0;
    encoders[ENCODER_PM2].last_count = 0;
    encoders[ENCODER_PM2].overflow_count = 0;
    encoders[ENCODER_PM2].rpm = 0.0f;

    // 启动速度计算定时器（50ms中断）
    if (IsTimerValid(&htim6))
    {
        HAL_TIM_Base_Start_IT(&htim6);
    }
}

// 检查编码器是否可用
uint8_t BSP_Encoder_IsAvailable(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        return encoders[encoder_id].available; // 返回可用状态
    }
    return 0;
}

/**
 * @brief   检查编码器是否正在运行
 * @param   encoder_id    编码器ID
 * @return  1表示正在运行，0表示未运行
 * @note   如果encoder_id无效，将返回0
 */
uint8_t BSP_Encoder_IsRunning(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        return encoders[encoder_id].running;
    }
    return 0;
}

/**
 * @brief   启动编码器
 * @param   encoder_id    编码器ID
 * @note   如果encoder_id无效，不做任何操作
 * @return  无
 */
void BSP_Encoder_Start(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        if (encoders[encoder_id].available && !encoders[encoder_id].running)
        {
            // 启动编码器定时器
            HAL_StatusTypeDef status = HAL_TIM_Encoder_Start(encoders[encoder_id].htim, TIM_CHANNEL_ALL);
            if (status == HAL_OK)
            {
                __HAL_TIM_ENABLE_IT(encoders[encoder_id].htim, TIM_IT_UPDATE);
                encoders[encoder_id].running = 1;

                // 注意：这里不重置任何计数器！
                // 计数器只在Init或显式调用Reset时重置
            }
        }
    }
}

/**
 * @brief   停止编码器
 * @param   encoder_id    编码器ID
 * @note   如果encoder_id无效，不做任何操作
 * @return  无
 */
void BSP_Encoder_Stop(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        if (encoders[encoder_id].available && encoders[encoder_id].running)
        {
            // 先更新总计数，再停止
            encoders[encoder_id].total_count = BSP_Encoder_GetCount(encoder_id);

            // 停止编码器定时器
            HAL_TIM_Encoder_Stop(encoders[encoder_id].htim, TIM_CHANNEL_ALL);
            __HAL_TIM_DISABLE_IT(encoders[encoder_id].htim, TIM_IT_UPDATE);
            encoders[encoder_id].running = 0;
        }
    }
}

/**
 * @brief   启动所有编码器
 * @note   该函数会依次调用BSP_Encoder_Start，详见BSP_Encoder_Start的注释
 */
void BSP_Encoder_StartAll(void)
{
    for (Encoder_ID_t i = ENCODER_PM1; i <= ENCODER_PM2; i++)
    {
        BSP_Encoder_Start(i);
    }
}

/**
 * @brief   停止所有编码器
 * @note   该函数会依次调用BSP_Encoder_Stop，详见BSP_Encoder_Stop的注释
 */
void BSP_Encoder_StopAll(void)
{
    for (Encoder_ID_t i = ENCODER_PM1; i <= ENCODER_PM2; i++)
    {
        BSP_Encoder_Stop(i);
    }
}

/**
 * @brief   重置编码器, 将启用的编码器的计数器清零
 * @param   encoder_id    编码器ID
 * @note   如果encoder_id无效，不做任何操作
 * @return  无
 */
void BSP_Encoder_Reset(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        if (encoders[encoder_id].available)
        {
            if (encoders[encoder_id].running)
            {
                __HAL_TIM_SET_COUNTER(encoders[encoder_id].htim, 0);
            }
            encoders[encoder_id].overflow_count = 0;
            encoders[encoder_id].last_count = 0;
            encoders[encoder_id].total_count = 0;
            encoders[encoder_id].rpm = 0.0f;
        }
    }
}

/**
 * @brief   获取编码器的总计数
 * @param   encoder_id    编码器ID
 * @return  编码器的总计数
 * @note   如果encoder_id无效，将返回0
 */
int32_t BSP_Encoder_GetCount(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        if (encoders[encoder_id].available)
        {
            if (encoders[encoder_id].running)
            {
                int32_t current_count = __HAL_TIM_GET_COUNTER(encoders[encoder_id].htim);
                return current_count + encoders[encoder_id].overflow_count * 65536;
            }
            else
            {
                // 停止时返回保存的总计数
                return encoders[encoder_id].total_count;
            }
        }
    }
    return 0;
}

/**
 * @brief   获取编码器的转速
 * @param   encoder_id    编码器ID
 * @return  编码器的转速
 * @note   如果encoder_id无效，将返回0
 */
float BSP_Encoder_GetSpeedRPM(Encoder_ID_t encoder_id)
{
    if (encoder_id <= ENCODER_PM2)
    {
        // 无论是否运行，都返回计算出的转速
        // 停止时rpm为0，这是正确的
        return encoders[encoder_id].rpm;
    }
    return 0.0f;
}

/**
 * @brief   处理编码器溢出
 * @param   htim    编码器定时器
 * @note   如果编码器ID无效，不做任何操作
 */
void BSP_Encoder_HandleOverflow(TIM_HandleTypeDef *htim)
{
    for (Encoder_ID_t i = ENCODER_PM1; i <= ENCODER_PM2; i++)
    {
        if (encoders[i].available && encoders[i].running &&
            encoders[i].htim->Instance == htim->Instance)
        {
            if (__HAL_TIM_IS_TIM_COUNTING_DOWN(encoders[i].htim))
            {
                encoders[i].overflow_count--;
            }
            else
            {
                encoders[i].overflow_count++;
            }
            break;
        }
    }
}

/**
 * 更新编码器速度
 * @brief   本函数更新所有编码器的速度值，应在定时器中断中周期调用
 * @param   void
 * @note   if the encoder_id is invalid, the function does not do anything
 * @return  the encoder's speed in RPM
 */
void BSP_Encoder_UpdateSpeed(void)
{
    for (Encoder_ID_t i = ENCODER_PM1; i <= ENCODER_PM2; i++)
    {
        if (encoders[i].available && encoders[i].running)
        {
            int32_t current_count = BSP_Encoder_GetCount(i);
            int32_t delta_count = current_count - encoders[i].last_count;

            if (delta_count != 0)
            {
                // 使用与正点原子相同的公式原理
                // RPM = (脉冲数 × (60000/采样时间ms)) ÷ 减速比 ÷ PPR
                encoders[i].rpm = (delta_count * (60000.0f / SPEED_UPDATE_MS)) / (GEAR_RATIO * ENCODER_PPR);
            }
            else
            {
                encoders[i].rpm = 0.0f;
            }

            encoders[i].last_count = current_count;
        }
        else
        {
            encoders[i].rpm = 0.0f;
        }
    }
}
