#include "pid_param_parse.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include <math.h>

/* 解析 "key:num" 格式中的数值，key不含冒号 */
static float _parse_value(const char *str, const char *key)
{
    char *pos = strstr(str, key);
    if (pos == NULL)
        return 0.0f;
    pos += strlen(key); /* 指向冒号 */
    if (*pos == ':')
        pos++; /* 跳过冒号 */
    return atof(pos);
}

/**
 * @brief 初始化PID参数解析器
 */
void PID_ParamParser_Init(PID_ParamParser_t *parser)
{
    // 1. 空指针保护：避免传入NULL指针导致内存操作崩溃
    if (parser == NULL)
        return;

    memset(parser->recv_buf, 0, sizeof(parser->recv_buf));

    parser->recv_len = 0;

    parser->state = PARSE_STATE_IDLE;

    memset(&parser->parsed_cmd, 0, sizeof(ParsedCmd_t));
}

/**
 * @brief 串口字节接收回调（中断中调用，仅存数据不解析）
 */
void PID_ParamParser_RecvByte(PID_ParamParser_t *parser, uint8_t byte)
{
    if (parser == NULL || parser->recv_len >= sizeof(parser->recv_buf) - 1)
    {
        PID_ParamParser_Reset(parser);
        return;
    }

    if (byte == '\n')
    {
        parser->recv_buf[parser->recv_len] = '\0';
        parser->state = PARSE_STATE_COMPLETE;
        parser->recv_len = 0;
        return;
    }

    if (byte != '\r' && byte >= 0x20 && byte <= 0x7E)
    {
        parser->recv_buf[parser->recv_len++] = byte;
        parser->state = PARSE_STATE_RECEIVING;
    }
}

PID_ParseState_t PID_ParamParser_Parse(PID_ParamParser_t *parser)
{
    if (parser == NULL || parser->state != PARSE_STATE_COMPLETE)
    {
        return parser->state;
    }

    char *str = (char *)parser->recv_buf;
    memset(&parser->parsed_cmd, 0, sizeof(ParsedCmd_t));

    /* 根据开头判断指令类型 */
    if (strstr(str, "RST") != NULL)
    {
        parser->parsed_cmd.type = CMD_TYPE_RST;
    }
    else if (strstr(str, "PID_X:") != NULL || strstr(str, "PID_Y:") != NULL || strstr(str, "PID_Z:") != NULL)
    {
        parser->parsed_cmd.type = CMD_TYPE_PID_AXIS;

        /* 确定哪一路 */
        if (strstr(str, "PID_X:") != NULL)
            parser->parsed_cmd.pid_axis = 0;
        else if (strstr(str, "PID_Y:") != NULL)
            parser->parsed_cmd.pid_axis = 1;
        else
            parser->parsed_cmd.pid_axis = 2;

        /* 解析冒号后的三个逗号分隔数字 */
        char *p = strchr(str, ':');
        if (p != NULL)
        {
            p++;
            parser->parsed_cmd.kp = atof(p);
            p = strchr(p, ',');
            if (p != NULL)
            {
                p++;
                parser->parsed_cmd.ki = atof(p);
                p = strchr(p, ',');
                if (p != NULL)
                {
                    p++;
                    parser->parsed_cmd.kd = atof(p);
                }
            }
        }
    }
    else if (strstr(str, "X:") != NULL || strstr(str, "Y:") != NULL || strstr(str, "Z:") != NULL)
    {
        parser->parsed_cmd.type = CMD_TYPE_TARGET;
        parser->parsed_cmd.target_x = _parse_value(str, "X");
        parser->parsed_cmd.target_y = _parse_value(str, "Y");
        parser->parsed_cmd.target_z = _parse_value(str, "Z");
    }
    
    else if (strstr(str, "KP:") != NULL || strstr(str, "KI:") != NULL || strstr(str, "KD:") != NULL)
    {
        parser->parsed_cmd.type = CMD_TYPE_PID;
        parser->parsed_cmd.kp = _parse_value(str, "KP");
        parser->parsed_cmd.ki = _parse_value(str, "KI");
        parser->parsed_cmd.kd = _parse_value(str, "KD");
    }
    else
    {
        parser->state = PARSE_STATE_ERROR;
        return PARSE_STATE_ERROR;
    }

    parser->state = PARSE_STATE_IDLE;
    return PARSE_STATE_COMPLETE;
}

ParsedCmd_t PID_ParamParser_GetCmd(PID_ParamParser_t *parser)
{
    if (parser == NULL)
    {
        ParsedCmd_t empty = {0};
        return empty;
    }
    return parser->parsed_cmd;
}

void PID_ParamParser_Reset(PID_ParamParser_t *parser)
{
    if (parser == NULL)
        return;
    memset(parser->recv_buf, 0, sizeof(parser->recv_buf));
    parser->recv_len = 0;
    parser->state = PARSE_STATE_IDLE;
    memset(&parser->parsed_cmd, 0, sizeof(ParsedCmd_t));
}
