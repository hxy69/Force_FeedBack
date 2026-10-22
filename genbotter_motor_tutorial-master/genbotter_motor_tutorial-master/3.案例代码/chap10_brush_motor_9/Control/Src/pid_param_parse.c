/**
 * @file    pid_param_parse.c
 * @brief   pid参数解析器源文件
 * @author  Dr. GAO
 * @date    2025-3-23
 * @version V1.0
 */

#include "pid_param_parse.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include <math.h>

/* 私有函数声明 */
static PID_ControllerType_t _parse_controller_type(const char *str);
static float _parse_float_value(const char *str, const char *key);

/**
 * @brief 初始化PID参数解析器
 */
void PID_ParamParser_Init(PID_ParamParser_t *parser) {
    // 1. 空指针保护：避免传入NULL指针导致内存操作崩溃
    if (parser == NULL) return;

    // 2. 清空接收缓冲区：重置用于存储串口/总线接收的原始数据的缓冲区
    //    防止旧数据残留导致解析错误（比如上次解析的残数据干扰本次解析）
    memset(parser->recv_buf, 0, sizeof(parser->recv_buf));

    // 3. 重置接收长度计数器：记录接收缓冲区中有效数据长度，初始化为0
    parser->recv_len = 0;

    // 4. 重置解析器状态机：将解析状态置为“空闲”（初始状态）
    //    状态机（state）用于管理解析流程（如：等待帧头、接收参数、校验帧尾等）
    parser->state = PARSE_STATE_IDLE;

    // 5. 清空已解析的PID参数结构体：确保参数初始值为0，避免脏数据
    //    该结构体存储最终解析出的KP/KI/KD/输出限幅等PID核心参数
    memset(&parser->parsed_param, 0, sizeof(PID_Param_t));
}

/**
 * @brief 串口字节接收回调（中断中调用，仅存数据不解析）
 */
void PID_ParamParser_RecvByte(PID_ParamParser_t *parser, uint8_t byte) {
    if (parser == NULL || parser->recv_len >= sizeof(parser->recv_buf)-1) {
        PID_ParamParser_Reset(parser); // 缓冲区满，重置
        return;
    }

    // 接收结束符（\r\n），标记接收完成
    if (byte == '\n') {
        parser->recv_buf[parser->recv_len] = '\0'; // 字符串结束符
        parser->state = PARSE_STATE_COMPLETE;
        parser->recv_len = 0; // 重置接收长度，准备下一次
        return;
    }

    // 数据合法性校验：
    // 1. 非回车符 \r
    // 2. 属于ASCII可打印字符范围：0x20(空格) ~ 0x7E(~)
    if (byte != '\r' && byte >= 0x20 && byte <= 0x7E) {
        parser->recv_buf[parser->recv_len++] = byte;
        parser->state = PARSE_STATE_RECEIVING;
    }
}

/**
 * @brief 解析控制器类型（SPEED/POSITION）
 */
static PID_ControllerType_t _parse_controller_type(const char *str) {
    if (strstr(str, "SPEED") != NULL) {
        return PID_CONTROLLER_SPEED;
    } else if (strstr(str, "POSITION") != NULL) {
        return PID_CONTROLLER_POSITION;
    } else {
        return PID_CONTROLLER_UNKNOWN;
    }
}

/**
 * @brief 解析键值对中的浮点值（如从"KP=20.0"中提取20.0）
 */
static float _parse_float_value(const char *str, const char *key) {
    char *pos = strstr(str, key);
    if (pos == NULL) return 0.0f;

    pos += strlen(key) + 1; // 跳过"KP="，指向数值部分
    return atof(pos); // 字符串转浮点
}

/**
 * @brief 解析完整的PID指令, "PID:SPEED,KP=20.0,KI=1.0,KD=0.5,MAX_OUT=100.0,MAX_INT=50.0\r\n"
 * @return 解析状态
 */
PID_ParseState_t PID_ParamParser_Parse(PID_ParamParser_t *parser) {
    if (parser == NULL || parser->state != PARSE_STATE_COMPLETE) {
        return parser->state;
    }

    char *recv_str = (char*)parser->recv_buf;
    // 校验指令头
    if (strstr(recv_str, "PID:") == NULL) {
        parser->state = PARSE_STATE_ERROR;
        return PARSE_STATE_ERROR;
    }

    // 1. 解析控制器类型
    parser->parsed_param.ctrl_type = _parse_controller_type(recv_str);
    if (parser->parsed_param.ctrl_type == PID_CONTROLLER_UNKNOWN) {
        parser->state = PARSE_STATE_ERROR;
        return PARSE_STATE_ERROR;
    }

    // 2. 解析PID参数（键值对）
    parser->parsed_param.kp = _parse_float_value(recv_str, "KP");
    parser->parsed_param.ki = _parse_float_value(recv_str, "KI");
    parser->parsed_param.kd = _parse_float_value(recv_str, "KD");
    parser->parsed_param.max_out = _parse_float_value(recv_str, "MAX_OUT");
    parser->parsed_param.max_int = _parse_float_value(recv_str, "MAX_INT");

    // 3. 校验参数有效性（防止非法值）
    if (isnan(parser->parsed_param.kp) || isinf(parser->parsed_param.kp) ||
        isnan(parser->parsed_param.ki) || isinf(parser->parsed_param.ki) ||
        isnan(parser->parsed_param.kd) || isinf(parser->parsed_param.kd)) {
        parser->state = PARSE_STATE_ERROR;
        return PARSE_STATE_ERROR;
    }

    parser->state = PARSE_STATE_IDLE; // 解析完成，重置状态
    return PARSE_STATE_COMPLETE;
}

/**
 * @brief 获取解析后的PID参数
 */
PID_Param_t PID_ParamParser_GetParam(PID_ParamParser_t *parser) {
    if (parser == NULL) {
        // 原代码：PID_Param_t empty = {0}; （触发枚举类型警告）
        // 修复后：显式初始化所有字段，枚举字段用枚举值
        PID_Param_t empty = {
            .ctrl_type = PID_CONTROLLER_UNKNOWN,  // 枚举字段显式初始化
            .kp = 0.0f,
            .ki = 0.0f,
            .kd = 0.0f,
            .max_out = 0.0f,
            .max_int = 0.0f
        };
        return empty;
    }
    return parser->parsed_param;
}

/**
 * @brief 重置解析器状态
 */
void PID_ParamParser_Reset(PID_ParamParser_t *parser) {
    if (parser == NULL) return;
    memset(parser->recv_buf, 0, sizeof(parser->recv_buf));
    parser->recv_len = 0;
    parser->state = PARSE_STATE_IDLE;
    memset(&parser->parsed_param, 0, sizeof(PID_Param_t));
}
