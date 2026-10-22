#ifndef PID_PARAM_PARSE_H
#define PID_PARAM_PARSE_H

#include "stdint.h"

/* 三种指令类型 */
typedef enum
{
    CMD_TYPE_NONE = 0,
    CMD_TYPE_RST,         /* RST;           停止3路 */
    CMD_TYPE_TARGET,      /* X:num,Y:num,Z:num;  分别设目标电流 */
    CMD_TYPE_PID,          /* KP:num,KI:num,KD:num; 设PID参数 */
    CMD_TYPE_PID_AXIS /* 新增：PID_X:kp,ki,kd; 单独设某一路PID */
} CmdType_t;

typedef enum
{
    PARSE_STATE_IDLE,
    PARSE_STATE_RECEIVING,
    PARSE_STATE_COMPLETE,
    PARSE_STATE_ERROR
} PID_ParseState_t;

/* 解析结果 */
typedef struct
{
    CmdType_t type;
    float target_x;
    float target_y;
    float target_z;
    float kp;
    float ki;
    float kd;
    uint8_t pid_axis;   /* 新增：0=X, 1=Y, 2=Z，仅CMD_TYPE_PID_AXIS有效 */
} ParsedCmd_t;

typedef struct
{
    uint8_t recv_buf[128];
    uint16_t recv_len;
    PID_ParseState_t state;
    ParsedCmd_t parsed_cmd;
} PID_ParamParser_t;

void PID_ParamParser_Init(PID_ParamParser_t *parser);
void PID_ParamParser_RecvByte(PID_ParamParser_t *parser, uint8_t byte);
PID_ParseState_t PID_ParamParser_Parse(PID_ParamParser_t *parser);
ParsedCmd_t PID_ParamParser_GetCmd(PID_ParamParser_t *parser);
void PID_ParamParser_Reset(PID_ParamParser_t *parser);

#endif
