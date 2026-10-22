#include "bldc_motor.h"

#include "main.h"
// 保存定时器句柄，必须在 BLDC_Init 中初始化
static TIM_HandleTypeDef *bldc_htim = NULL;

// 保存当前换相步
static int step_index = 0;

// 电机当前状态的变量，
static BLDC_State_t bldc_state = BLDC_STOPPED;
static BLDC_Dir_t bldc_dir = BLDC_CW;

// 换相周期控制：单位为“TIM中断周期数”，例如每1ms中断一次，则50表示50ms换相一次
static uint32_t comm_interval_ms = 50; // 默认100ms（低速启动），假设tickhandler每1ms调用一次
static uint32_t comm_counter = 0;      // 换相计数器

// 需要在CubeMX中定义用户标签 PM1_CTRL_SD：
#define BLDC_ENABLE_PIN PM1_CTRL_SD_Pin
#define BLDC_ENABLE_PORT PM1_CTRL_SD_GPIO_Port

// 六步换相表 (UH, UL, VH, VL, WH, WL), (CH1, CH1N, CH2, CH2N, CH3, CH3N)
// 1=开通道, 0=关通道
static const uint8_t comm_table[6][6] = {
    {1, 0, 0, 1, 0, 0}, // Step0: U+ V-
    {1, 0, 0, 0, 0, 1}, // Step1: U+ W-
    {0, 0, 1, 0, 0, 1}, // Step2: V+ W-
    {0, 1, 1, 0, 0, 0}, // Step3: V+ U-
    {0, 1, 0, 0, 1, 0}, // Step4: W+ U-
    {0, 0, 0, 1, 1, 0}  // Step5: W+ V-
};

//-----------------------------------------
// 内部函数：关闭所有 PWM 通道
static void BLDC_AllChannelOff(void)
{
    if (bldc_htim == NULL)
        return;

    HAL_TIM_PWM_Stop(bldc_htim, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(bldc_htim, TIM_CHANNEL_1);

    HAL_TIM_PWM_Stop(bldc_htim, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(bldc_htim, TIM_CHANNEL_2);

    HAL_TIM_PWM_Stop(bldc_htim, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(bldc_htim, TIM_CHANNEL_3);
}

//-----------------------------------------
// 内部函数：配置某一步的通道
void BLDC_SetStep(uint8_t step)
{
    if (bldc_htim == NULL || step >= 6)
        return;

    uint16_t duty_cycle = 50; // 设置合适的占空比，太低可能导致磁场强度不够无法启动，根据Arr调整

    // 先关闭所有通道，防止残留
    BLDC_AllChannelOff();

    // 上桥臂调速
    if (comm_table[step][0])
    { // U上桥臂
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_1, duty_cycle);
        HAL_TIM_PWM_Start(bldc_htim, TIM_CHANNEL_1);
    }
    if (comm_table[step][2])
    { // V上桥臂
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_2, duty_cycle);
        HAL_TIM_PWM_Start(bldc_htim, TIM_CHANNEL_2);
    }
    if (comm_table[step][4])
    { // W上桥臂
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_3, duty_cycle);
        HAL_TIM_PWM_Start(bldc_htim, TIM_CHANNEL_3);
    }

    // 下桥臂（互补通道）
    if (comm_table[step][1]) { // UL
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_1, duty_cycle); // duty 随便，只要开启
        HAL_TIMEx_PWMN_Start(bldc_htim, TIM_CHANNEL_1);
    }
    if (comm_table[step][3]) { // VL
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_2, duty_cycle);
        HAL_TIMEx_PWMN_Start(bldc_htim, TIM_CHANNEL_2);
    }
    if (comm_table[step][5]) { // WL
        __HAL_TIM_SET_COMPARE(bldc_htim, TIM_CHANNEL_3, duty_cycle);
        HAL_TIMEx_PWMN_Start(bldc_htim, TIM_CHANNEL_3);
    }
}

// 启用电机驱动（拉高使能引脚）
static void BLDC_Enable(void)
{
    HAL_GPIO_WritePin(BLDC_ENABLE_PORT, BLDC_ENABLE_PIN, GPIO_PIN_SET);
}

// 禁用电机驱动（拉低使能引脚）
static void BLDC_Disable(void)
{
    HAL_GPIO_WritePin(BLDC_ENABLE_PORT, BLDC_ENABLE_PIN, GPIO_PIN_RESET);
}

// 初始化
void BLDC_Init(TIM_HandleTypeDef *htim)
{
    bldc_htim = htim;
    bldc_state = BLDC_STOPPED;
    bldc_dir = BLDC_CW;
    step_index = 0;
    comm_counter = 0;
//    comm_interval_ms = 10; // 默认50ms换相周期
    BLDC_AllChannelOff();
    BLDC_Disable();
}

// 设置换相间隔
void BLDC_SetStepInterval(uint32_t interval)
{
    if (interval < 10)
        interval = 10; // 默认最小10mS换相间隔
    comm_interval_ms = interval;
}

//-----------------------------------------
// 执行一步换相
static void BLDC_StepOnce()
{
    if (bldc_dir == BLDC_CW)
        step_index = (step_index + 1) % 6;
    else
        step_index = (step_index + 5) % 6;

    BLDC_SetStep(step_index);
}

//-----------------------------------------
// 设置状态
void BLDC_SetState(BLDC_State_t state, BLDC_Dir_t dir)
{
    bldc_state = state;
    bldc_dir = dir;

    if (state == BLDC_STOPPED)
    {
        BLDC_AllChannelOff();
        BLDC_Disable();
    }
    else
    {
        BLDC_Enable();
    }
}

//-----------------------------------------
// 获取状态
BLDC_State_t BLDC_GetState(void)
{
    return bldc_state;
}

// 获取方向
BLDC_Dir_t BLDC_GetDir(void)
{
    return bldc_dir;
}
//-----------------------------------------
// 定时器中断调用
void BLDC_TickHandler(void)
{
    switch (bldc_state)
    {
    case BLDC_STEP:
        // 执行单步换相
        if (comm_counter == 0)
        {
            BLDC_Enable();
            BLDC_StepOnce();
        }
        comm_counter++;
        // 维持电平直到铁芯运动到指定位置
        if (comm_counter >= comm_interval_ms)
        {
            bldc_state = BLDC_STOPPED; // 单步完成后回到STOP
            BLDC_Disable();
            comm_counter = 0;
        }
        break;

    case BLDC_RUN:
        comm_counter++;
        if (comm_counter >= comm_interval_ms)
        {
            comm_counter = 0;
            BLDC_Enable();
            BLDC_StepOnce();
        }
        break;

    case BLDC_STOPPED:
    default:
        // 不做事
        BLDC_Disable();
        comm_counter = 0; // 重置comm_counter
        break;
    }
}
