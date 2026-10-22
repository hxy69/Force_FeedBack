/**
 * @file    bsp_current_sensor.c
 * @brief   直流有刷电机电流检测源文件
 * @author  Dr. GAO
 * @date    2026-05-31
 * @version V1.1
 * @website https://genbotter.taobao.com
 * @note    硬件带有约1.27V偏置电压，支持正负双向电流检测。
 *          上电时自动校准零漂，之后由TIM1 TRGO周期性触发ADC注入转换。
 */
#include "bsp_current_sensor.h"
#include "adc.h"
#include "tim.h"
#include "user_config.h"
#include "current_loop.h"

extern ADC_HandleTypeDef hadc1;
extern uint32_t adc_injected_data;
extern CurrentLoopCtrl current_loop;
extern uint8_t closed_loop_active;

/* 静态变量 */
static float current_offset_adc  = 0.0f;  /* 零漂偏移（ADC阶数） */
static float current_mA          = 0.0f;  /* 当前滤波后电流（mA） */
static float current_mA_prev     = 0.0f;  /* 上一次电流值，用于低通滤波 */

/* 过采样：累加 CURRENT_OVERSAMPLE_COUNT 次原始电流值后取平均 */
static float oversample_accum    = 0.0f;  /* 累加器（mA） */
static uint16_t oversample_cnt   = 0;     /* 已累积次数 */


/**
 * @brief 初始化电流传感器
 * @note  先以软件触发方式阻塞校准零漂，再开启硬件触发注入中断
 */
void BSP_CurrentSensor_Init(void)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 先阻塞式校准零漂，此时不依赖TIM1硬件触发 */
    BSP_CurrentSensor_CalibrateOffset();

    /* 校准完成后，开启注入中断，交给TIM1 TRGO周期性触发 */
    if (HAL_ADCEx_InjectedStart_IT(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 校准电流传感器零漂
 * @note  必须电机停止时调用。临时切换到软件触发模式，采集100次取平均。
 *        校准完成后恢复为TIM1 TRGO硬件触发。
 */
void BSP_CurrentSensor_CalibrateOffset(void)
{
    float sum_offset = 0.0f;
    uint16_t calibrate_count = 0;

    /* 关闭注入中断，切换为软件触发 */
    HAL_NVIC_DisableIRQ(ADC_IRQn);
    HAL_ADCEx_InjectedStop_IT(&hadc1);

    ADC_InjectionConfTypeDef sConfigInjected = {0};
    sConfigInjected.InjectedChannel      = ADC_CHANNEL_8;
    sConfigInjected.InjectedRank         = 1;
    sConfigInjected.InjectedNbrOfConversion = 1;
    sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_56CYCLES;
    sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_NONE;
    sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
    sConfigInjected.AutoInjectedConv     = DISABLE;
    sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
    sConfigInjected.InjectedOffset       = 0;
    HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected);

    /* 软件触发阻塞采集100次，取平均作为零漂 */
    while (calibrate_count < 100)
    {
        HAL_ADCEx_InjectedStart(&hadc1);
        if (HAL_ADCEx_InjectedPollForConversion(&hadc1, 10) == HAL_OK)
        {
            sum_offset += (float)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
            calibrate_count++;
        }
        HAL_ADCEx_InjectedStop(&hadc1);
    }

    if (calibrate_count > 0)
    {
        current_offset_adc = sum_offset / (float)calibrate_count;
    }

    /* 恢复TIM1 TRGO上升沿硬件触发 */
    sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
    HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected);

    /* 重新开启注入中断 */
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

/**
 * @brief 获取ADC注入原始值（未经零漂校正）
 */
uint32_t BSP_CurrentSensor_GetADCValue(void)
{
    return adc_injected_data;
}

/**
 * @brief 获取零漂偏移值
 */
float BSP_CurrentSensor_GetOffset(void)
{
    return current_offset_adc;
}

/**
 * @brief 更新电流计算（在ADC注入中断回调中周期性调用，由TIM1 TRGO 16kHz触发）
 * @note  过采样流程：
 *        1. 每次中断将原始电流值累加到 oversample_accum
 *        2. 累积 CURRENT_OVERSAMPLE_COUNT 次后求平均
 *        3. 对平均值做一阶低通滤波，更新 current_mA
 *        4. 立即调用 CurrentLoop_Update() 执行 PID 控制
 *
 *        电流更新频率 = 16kHz / N = 1000Hz（N=16）
 *        PID 控制频率 = 1000Hz（与电流更新同步，零延迟闭环）
 */
void BSP_CurrentSensor_Update(void)
{
    float adc_raw;
    float current_mA_temp;

    /* ---- Step 1: 单次 ADC 转换 → 电流值 ---- */
    adc_raw = (float)adc_injected_data - current_offset_adc;
    current_mA_temp = adc_raw * ADC_TO_MA_FACTOR;

    /* ---- Step 2: 过采样累加 ---- */
    oversample_accum += current_mA_temp;
    oversample_cnt++;

    /* ---- Step 3: 累积满 N 次 → 求平均 + 低通滤波 ---- */
    if (oversample_cnt >= CURRENT_OVERSAMPLE_COUNT)
    {
        float avg_current = oversample_accum / (float)CURRENT_OVERSAMPLE_COUNT;

        /* 一阶低通滤波（对平均值做，噪声已大幅降低） */
        current_mA = CURRENT_LOWPASS_ALPHA * avg_current
                   + (1.0f - CURRENT_LOWPASS_ALPHA) * current_mA_prev;

        current_mA_prev = current_mA;

        /* 重置累加器 */
        oversample_accum = 0.0f;
        oversample_cnt   = 0;

        /* ---- Step 4: 电流更新完毕 → 立即执行 PID 控制（ISR 内，零延迟） ---- */
        if (closed_loop_active)
        {
            CurrentLoop_Update(&current_loop);
        }
    }
}

/**
 * @brief 获取当前滤波后电流值
 * @retval 电流值（mA），正数为正向电流，负数为反向/回馈电流
 * @note  本板H桥+单采样电阻(R32)，ADC仅测量电流幅值（|I|），
 *        无法直接区分正反转电流方向。
 *        g_motor_current_direction（由 bsp_brush_motor 的 SetDirection 维护）
 *        提供方向符号（+1=正转，-1=反转），此处相乘还原真实的有符号电流。
 */
float BSP_CurrentSensor_GetCurrent(void)
{
    /* 本板H桥+单采样电阻(R32)，ADC无法区分正反转电流方向，
       由 bsp_brush_motor 的 g_motor_current_direction 提供方向符号 */
    extern int8_t g_motor_current_direction;
    return current_mA * g_motor_current_direction;
}
