#ifndef __STEPPER_MOTOR_H_
#define __STEPPER_MOTOR_H_

#include "main.h"

/**一些宏定义**/

extern TIM_HandleTypeDef htim8;

/*硬件资源的宏定义*/

//stepper1的宏,
//ST1_EN->PF15,ST1_DIR->PF14,ST1_STEP->PI5
//ST1_TIM->TIM8,ST1_TIM_CHANNEL->TIM_CHANNEL_1
#define ST1_EN_PORT      GPIOF
#define ST1_EN_PIN       GPIO_PIN_15
#define ST1_DIR_PORT     GPIOF
#define ST1_DIR_PIN      GPIO_PIN_14
#define ST1_STEP_PORT    GPIOI
#define ST1_STEP_PIN     GPIO_PIN_5
#define ST1_TIM          &htim8
#define ST1_TIM_CHANNEL  TIM_CHANNEL_1

//stepper2的宏,
//ST2_EN->PF13,ST2_DIR->PF12,ST2_STEP->PI6
//ST2_TIM->TIM8,ST2_TIM_CHANNEL->TIM_CHANNEL_2
#define ST2_EN_PORT      GPIOF
#define ST2_EN_PIN       GPIO_PIN_13
#define ST2_DIR_PORT     GPIOF
#define ST2_DIR_PIN      GPIO_PIN_12
#define ST2_STEP_PORT    GPIOI
#define ST2_STEP_PIN     GPIO_PIN_6
#define ST2_TIM          &htim8
#define ST2_TIM_CHANNEL  TIM_CHANNEL_2

//stepper3的宏,
//ST3_EN->PF11,ST3_DIR->PB2,ST3_STEP->PI7
//ST3_TIM->TIM8,ST3_TIM_CHANNEL->TIM_CHANNEL_3
#define ST3_EN_PORT      GPIOF
#define ST3_EN_PIN       GPIO_PIN_11
#define ST3_DIR_PORT     GPIOB
#define ST3_DIR_PIN      GPIO_PIN_2
#define ST3_STEP_PORT    GPIOI
#define ST3_STEP_PIN     GPIO_PIN_7
#define ST3_TIM          &htim8
#define ST3_TIM_CHANNEL  TIM_CHANNEL_3

//stepper4的宏,
//ST4_EN->PH3,ST4_DIR->PH2,ST4_STEP->PC9
//ST4_TIM->TIM8,ST4_TIM_CHANNEL->TIM_CHANNEL_4
#define ST4_EN_PORT      GPIOH
#define ST4_EN_PIN       GPIO_PIN_3
#define ST4_DIR_PORT     GPIOH
#define ST4_DIR_PIN      GPIO_PIN_2
#define ST4_STEP_PORT    GPIOC
#define ST4_STEP_PIN     GPIO_PIN_9
#define ST4_TIM          &htim8
#define ST4_TIM_CHANNEL  TIM_CHANNEL_4

//最小和最大的步进脉冲频率
#define STEPPER_MIN_RPM  1
#define STEPPER_MAX_RPM  30000

/**一些enum**/
//步进电机接口组的ID
typedef enum{
    STEPPER_1 = 0,
    STEPPER_2,
    STEPPER_3,
    STEPPER_4,
    STEPPER_SUM //表示步进电机接口组的数量
}StepperID;

//步进电机的方向
typedef enum{
    STEPPER_DIR_CW = 0,
    STEPPER_DIR_CCW
}StepperDir;

//步进电机的使能状态
typedef enum{
    STEPPER_DISABLE = 0,
    STEPPER_ENABLE
}StepperEnableState;

//错误码
typedef enum{
    STEPPER_OK = 0,
    STEPPER_ERROR,
    STEPPER_INVALID_ID,
    STEPPER_INVALID_PARAM
}StepperErrorCode;

/*一些结构体*/
typedef struct{
    GPIO_TypeDef *en_port;
    uint16_t en_pin;
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;
    GPIO_TypeDef *step_port;
    uint16_t step_pin;
    TIM_HandleTypeDef *htim;
    uint32_t tim_channel;
}StepperMotorHwRes;

// 步进电机工作状态的结构体
typedef struct{
    StepperEnableState enable;
    StepperDir dir;
    uint32_t speed;
    StepperErrorCode error; //最近的错误码
}StepperStatus;

/* 一些接口函数的声明 */
StepperErrorCode Stepper_Init(StepperID id);
StepperErrorCode Stepper_SetSpeed(StepperID id, uint32_t speed); //步进电机控制脉冲的速度
StepperErrorCode Stepper_SetDir(StepperID id, StepperDir dir);
StepperErrorCode Stepper_SetEnable(StepperID id, StepperEnableState enable);
StepperErrorCode Stepper_GetStatus(StepperID id, StepperStatus *status);

#endif /* __STEPPER_MOTOR_H_ */
