#include "current_sensor.h"
#include "adc.h"
#include "tim.h"
#include "user_config.h"
#include "current_loop.h"
#include "brush_motor.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern uint32_t adc_injected_data[3];       /* ADC注入组采样值 */
extern CurrentLoopCtrl current_loop[3];
extern uint8_t closed_loop_active[3];
extern BrushMotorConfig motor[3];

static float current_offset_adc[3] = {0};   /* ADC零点校准 */
static float current_mA[3] = {0};
static float current_mA_prev[3] = {0};
static float oversample_accum[3] = {0};
static uint16_t oversample_cnt = 0; /* 3路同步采样，计数器共用一个 */


void BSP_CurrentSensor_Init(void)
{
    BSP_CurrentSensor_CalibrateOffset();
    if (HAL_ADCEx_InjectedStart_IT(&hadc1) != HAL_OK)
        Error_Handler();
    if (HAL_ADCEx_InjectedStart_IT(&hadc2) != HAL_OK)
        Error_Handler();
    if (HAL_ADCEx_InjectedStart_IT(&hadc3) != HAL_OK)
        Error_Handler();
}


/**
 * @brief  校准电流传感器零点偏移
 * @note   阻塞式采集100次ADC平均值作为零点偏移
 */
void BSP_CurrentSensor_CalibrateOffset(void)
{
    /*X：(ADC3/IN3), Y：(ADC2/IN6), Z：(ADC1/IN8) */
    ADC_HandleTypeDef *adcs[3] = {&hadc3, &hadc2, &hadc1};
    ADC_InjectionConfTypeDef sConfigInjected = {0};

    HAL_NVIC_DisableIRQ(ADC_IRQn);
    HAL_ADCEx_InjectedStop_IT(&hadc1);
    HAL_ADCEx_InjectedStop_IT(&hadc2);
    HAL_ADCEx_InjectedStop_IT(&hadc3);

    for (int ch = 0; ch < 3; ch++)
    {
        float sum_offset = 0.0f;
        uint16_t calibrate_count = 0;

        /* 切换到软件触发，通道保持与 adc.c 一致 */
        sConfigInjected.InjectedChannel = (ch == 0) ? ADC_CHANNEL_3 : (ch == 1) ? ADC_CHANNEL_6
                                                                                : ADC_CHANNEL_8;
        sConfigInjected.InjectedRank = 1;
        sConfigInjected.InjectedNbrOfConversion = 1;
        sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_28CYCLES;
        sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_NONE;
        sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
        sConfigInjected.AutoInjectedConv = DISABLE;
        sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
        sConfigInjected.InjectedOffset = 0;
        HAL_ADCEx_InjectedConfigChannel(adcs[ch], &sConfigInjected);

        /* 软件触发阻塞采集100次 */
        while (calibrate_count < 100)
        {
            HAL_ADCEx_InjectedStart(adcs[ch]);
            if (HAL_ADCEx_InjectedPollForConversion(adcs[ch], 10) == HAL_OK)
            {
                sum_offset += (float)HAL_ADCEx_InjectedGetValue(adcs[ch], ADC_INJECTED_RANK_1);
                calibrate_count++;
            }
            HAL_ADCEx_InjectedStop(adcs[ch]);
        }
        current_offset_adc[ch] = sum_offset / (float)calibrate_count;

        /* 恢复硬件触发 */
        sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
        HAL_ADCEx_InjectedConfigChannel(adcs[ch], &sConfigInjected);
    }

    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc2);
    HAL_ADCEx_InjectedStart_IT(&hadc3);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}


/**
 * @brief  更新电流传感器数据,包括减零漂、转mA、过采样累加、低通滤波、PID更新
 */
void BSP_CurrentSensor_Update(void)
{
    /* 3路同时减零漂、转mA、过采样累加 */
    for (int i = 0; i < 3; i++)
    {
        float adc_raw = (float)adc_injected_data[i] - current_offset_adc[i];   /* 校准后的原始值 */
        float current_mA_temp = adc_raw * ADC_TO_MA_FACTOR;                    /* 运放后的真实电流值 */
        oversample_accum[i] += current_mA_temp;
    }
    oversample_cnt++;

    if (oversample_cnt >= CURRENT_OVERSAMPLE_COUNT)
    {
        /* 16次满了，3路分别低通滤波 */
        for (int i = 0; i < 3; i++)
        {
            float avg = oversample_accum[i] / CURRENT_OVERSAMPLE_COUNT;
            current_mA[i] = CURRENT_LOWPASS_ALPHA * avg + (1.0f - CURRENT_LOWPASS_ALPHA) * current_mA_prev[i];
            current_mA_prev[i] = current_mA[i];
            oversample_accum[i] = 0.0f;
        }
        oversample_cnt = 0;

        /* 3路电流更新完，跑3路PID */
        for (int i = 0; i < 3; i++)
        {
            if (closed_loop_active[i])
            {
                CurrentLoop_Update(&current_loop[i]);
            }
        }
    }
}

/**
 * @brief  获取指定电流传感器的电流值（mA）
 * @param  id: 电流传感器ID (0, 1, 2)
 * @return 电流值 (mA)
 */
float BSP_CurrentSensor_GetCurrent(uint8_t id)
{
    if (id >= 3)
        return 0.0f;
    return current_mA[id] * motor[id].current_direction;
}


/**
 * @brief  获取指定电流传感器的零点偏移值
 * @param  id: 电流传感器ID (0, 1, 2)
 * @return 零点偏移值 (ADC单位)
 */
float BSP_CurrentSensor_GetOffsetById(uint8_t id)
{
    if (id >= 3)
    {
        return 0.0f;
    }
    return current_offset_adc[id];
}
