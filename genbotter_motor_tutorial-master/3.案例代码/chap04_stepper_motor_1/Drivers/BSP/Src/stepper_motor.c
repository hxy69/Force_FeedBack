#include "stepper_motor.h"

// 步进电机硬件资源的结构体数组
const StepperMotorHwRes stepper_hw_res[STEPPER_SUM] = {
    {ST1_EN_PORT, ST1_EN_PIN, ST1_DIR_PORT, ST1_DIR_PIN, ST1_STEP_PORT, ST1_STEP_PIN, ST1_TIM, ST1_TIM_CHANNEL},
    {ST2_EN_PORT, ST2_EN_PIN, ST2_DIR_PORT, ST2_DIR_PIN, ST2_STEP_PORT, ST2_STEP_PIN, ST2_TIM, ST2_TIM_CHANNEL},
    {ST3_EN_PORT, ST3_EN_PIN, ST3_DIR_PORT, ST3_DIR_PIN, ST3_STEP_PORT, ST3_STEP_PIN, ST3_TIM, ST3_TIM_CHANNEL},
    {ST4_EN_PORT, ST4_EN_PIN, ST4_DIR_PORT, ST4_DIR_PIN, ST4_STEP_PORT, ST4_STEP_PIN, ST4_TIM, ST4_TIM_CHANNEL}
};

// 步进电机的状态结构体数组
static StepperStatus stepper_status[STEPPER_SUM] ={
    {STEPPER_DISABLE, STEPPER_DIR_CW, 0, STEPPER_OK},
    {STEPPER_DISABLE, STEPPER_DIR_CW, 0, STEPPER_OK},
    {STEPPER_DISABLE, STEPPER_DIR_CW, 0, STEPPER_OK},
    {STEPPER_DISABLE, STEPPER_DIR_CW, 0, STEPPER_OK}
};

StepperErrorCode Stepper_Init(StepperID id){
    if(id >= STEPPER_SUM){
        return STEPPER_INVALID_ID;
    }

    // 初始化电机，硬件外设（GPIO等）的初始化工作已经在CubeMX生成的代码中完成

    // 初始化步进电机的默认状态
    stepper_status[id].enable = STEPPER_DISABLE;
    stepper_status[id].dir = STEPPER_DIR_CW;
    stepper_status[id].speed = 0;
    stepper_status[id].error = STEPPER_OK;

    // 使能步进电机
    Stepper_SetEnable(id, STEPPER_DISABLE);

    // 设置步进电机的旋转方向
    Stepper_SetDir(id, STEPPER_DIR_CW);

    // 设置步进电机的速度
    Stepper_SetSpeed(id, 0);

    return STEPPER_OK;
}

//步进电机控制
StepperErrorCode Stepper_SetSpeed(StepperID id, uint32_t speed){

    if(id >= STEPPER_SUM){
        return STEPPER_INVALID_ID;
    }

    if(speed > STEPPER_MAX_RPM || speed < STEPPER_MIN_RPM){
        stepper_status[id].error = STEPPER_INVALID_PARAM;
        return STEPPER_INVALID_PARAM;        
    }

    TIM_HandleTypeDef *tim_handle = stepper_hw_res[id].htim;
    // 获取APB2总线时钟频率，因为此处用的是TIM8
    uint32_t apb2_freq = HAL_RCC_GetPCLK2Freq();
    // 得到预分频寄存器的值
    uint16_t psc = tim_handle->Init.Prescaler;
    // 计算TIM8的计数器的时钟频率cnt_freq
    uint32_t cnt_freq = apb2_freq / (psc + 1);

    // 根据速度speed的要求，来计算TIM8的重装载值
    uint32_t auto_reload = cnt_freq / speed - 1;

    HAL_TIM_PWM_Stop(tim_handle, stepper_hw_res[id].tim_channel);
    // 设置TIM8的重装载值
    __HAL_TIM_SET_AUTORELOAD(tim_handle, auto_reload);
    // PWM的占空比设置为50%
    __HAL_TIM_SET_COMPARE(tim_handle, stepper_hw_res[id].tim_channel, (auto_reload + 1) / 2);
    HAL_TIM_GenerateEvent(tim_handle, TIM_EVENTSOURCE_UPDATE); //强制产生更新
    HAL_TIM_PWM_Start(tim_handle, stepper_hw_res[id].tim_channel);

    // 记录id对应步进电机的速度
    stepper_status[id].speed = speed;

    return STEPPER_OK;
}

//步进电机控制旋转方向
StepperErrorCode Stepper_SetDir(StepperID id, StepperDir dir){
    if(id >= STEPPER_SUM){
        return STEPPER_INVALID_ID;
    }

    if(dir == STEPPER_DIR_CW){//顺时针转动，让板子输出高电平
        HAL_GPIO_WritePin(stepper_hw_res[id].dir_port, stepper_hw_res[id].dir_pin, GPIO_PIN_RESET); // 因为光耦的翻转作用，所以io置为reset，实际板子输出是高电平
    }else if (dir == STEPPER_DIR_CCW){//逆时针转动，让板子输出高电平
        HAL_GPIO_WritePin(stepper_hw_res[id].dir_port, stepper_hw_res[id].dir_pin, GPIO_PIN_SET); // 因为光耦的翻转作用，所以io置为set，实际板子输出是低电平
    }else{
        stepper_status[id].error = STEPPER_INVALID_PARAM;
        return STEPPER_INVALID_PARAM;
    }

    // 记录id对应步进电机的旋转方向
    stepper_status[id].dir = dir;
    return STEPPER_OK;
}

//步进电机控制使能
StepperErrorCode Stepper_SetEnable(StepperID id, StepperEnableState enable){
    if(id >= STEPPER_SUM){
        return STEPPER_INVALID_ID;
    }

    // 根据id，将对应步进电机的enable端口置为有效
    if(enable == STEPPER_DISABLE){
        HAL_GPIO_WritePin(stepper_hw_res[id].en_port, stepper_hw_res[id].en_pin, GPIO_PIN_SET); // 因为光耦的翻转作用，所以io置为set，实际板子输出是低电平
    }else if(enable == STEPPER_ENABLE){
        HAL_GPIO_WritePin(stepper_hw_res[id].en_port, stepper_hw_res[id].en_pin, GPIO_PIN_RESET); // 因为光耦的翻转作用，所以io置为reset，实际板子输出是高电平
    }else{
        stepper_status[id].error = STEPPER_INVALID_PARAM;
        return STEPPER_INVALID_PARAM;
    }

    // 记录id对应步进电机的使能状态
    stepper_status[id].enable = enable;
    return STEPPER_OK;
}

StepperErrorCode Stepper_GetStatus(StepperID id, StepperStatus *status){
    if(id >= STEPPER_SUM) {
        return STEPPER_INVALID_ID;
    }

    if(status == NULL){
        stepper_status[id].error = STEPPER_INVALID_PARAM;
        return STEPPER_INVALID_PARAM;
    }

    *status = stepper_status[id];
    return STEPPER_OK;
};
