/**
 * @file    pid_param_parser.h
 * @brief   pid参数解析器头文件
 * @author  Dr. GAO
 * @date    2025-3-23
 * @version V1.0
 * @note    适配GenBotter Motor-1开发板，pid参数解析器，支持从字符串解析PID参数（配合在线调试pid参数）
 */
#ifndef PID_PARAM_PARSE_H
#define PID_PARAM_PARSE_H

#include "stdint.h"
#include "pid_controller.h"

/* 定义PID参数指令协议格式 */
// 上位机发送格式（字符串）："PID:SPEED,KP=20.0,KI=1.0,KD=0.5,MAX_OUT=100.0,MAX_INT=50.0\r\n"
// - 指令头：PID: 固定标识
// - 控制器类型：SPEED（速度环）/POSITION（位置环，预留）
// - 参数键值对：KP/KI/KD/MAX_OUT/MAX_INT（浮点型）
// - 结束符：\r\n

/* PID控制器类型枚举 */
typedef enum {
    PID_CONTROLLER_SPEED,    // 速度环PID
    PID_CONTROLLER_POSITION, // 位置环PID（预留）
    PID_CONTROLLER_UNKNOWN   // 未知类型
} PID_ControllerType_t;

/* 解析状态枚举 */
typedef enum {
    PARSE_STATE_IDLE,        // 空闲
    PARSE_STATE_RECEIVING,   // 接收中
    PARSE_STATE_COMPLETE,    // 解析完成
    PARSE_STATE_ERROR        // 解析错误
} PID_ParseState_t;

/* PID参数结构体（与上位机指令对应） */
typedef struct {
    PID_ControllerType_t ctrl_type; // 控制器类型
    float kp;                       // 比例系数
    float ki;                       // 积分系数
    float kd;                       // 微分系数
    float max_out;                  // 输出限幅
    float max_int;                  // 积分限幅
} PID_Param_t;

/* 解析器句柄 */
typedef struct {
    uint8_t recv_buf[128];          // 接收缓冲区（足够容纳一条指令）
    uint16_t recv_len;              // 已接收字节数
    PID_ParseState_t state;         // 解析状态
    PID_Param_t parsed_param;       // 解析后的参数
} PID_ParamParser_t;

/* 全局函数声明 */
// 初始化解析器
void PID_ParamParser_Init(PID_ParamParser_t *parser);

// 串口接收字节回调（在USART中断中调用）
void PID_ParamParser_RecvByte(PID_ParamParser_t *parser, uint8_t byte);

// 解析接收到的指令（在主循环/定时器中调用）
PID_ParseState_t PID_ParamParser_Parse(PID_ParamParser_t *parser);

// 获取解析后的PID参数
PID_Param_t PID_ParamParser_GetParam(PID_ParamParser_t *parser);

// 重置解析器状态
void PID_ParamParser_Reset(PID_ParamParser_t *parser);

#endif /* PID_PARAM_PARSE_H */
