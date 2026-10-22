#include "vofa_plus.h"
#include "usart.h"
#include "speed_loop.h" // 引入速度环头文件，获取调试数据

#if VOFA_PLUS_ENABLE

// 静态发送缓冲区，避免每次调用都分配内存
static uint8_t vofa_send_buf[VOFA_SEND_BUF_MAX_LEN] = {0};

/******************** VOFA+核心函数实现 ********************/
/**
 * @brief  VOFA+模块初始化
 * @note   串口初始化建议在main.c中统一做，此处仅做空实现（避免重复初始化）
 */
void VOFA_Plus_Init(void) {
    // VOFA+串口初始化建议在main.c中统一做，此处仅做空实现（避免重复初始化）
    // 例如：MX_USART1_UART_Init();

    // 清空发送缓冲区，避免脏数据干扰调试
    memset(vofa_send_buf, 0, sizeof(vofa_send_buf));
}

/**
 * @brief 封装VOFA+待发送数据（FireWater协议）
 * @param vofa_data: 待发送的数据结构体
 * @param send_buf: 输出缓冲区
 * @param buf_len: 缓冲区长度
 * @retval 封装后的数据长度
 */
uint16_t VOFA_Plus_PackData(VOFA_Data_t *vofa_data, uint8_t *send_buf, uint16_t buf_len) {
    // 简单封装示例：将结构体数据按顺序转换为字节流
    // 注意：实际使用时需根据VOFA+协议要求进行封装，如添加帧头、校验等
    if (buf_len < sizeof(VOFA_Data_t)) {
        return 0; // 缓冲区不足，无法封装
    }

#if VOFA_PROTOCOL == 1 // FireWater协议示例（字符串格式）
    // 以字符串格式封装数据，便于调试和查看
    if (buf_len < 32) { // 字符串格式需要更大的缓冲区
        return 0; // 缓冲区不足，无法封装
    }

    // FireWater协议示例：以逗号分隔的字符串格式，回车换行结尾 "RPM:100.0,ACT:95.5,PID:20.0,OUT:25.0\r\n"
    uint16_t len = snprintf((char *)send_buf, buf_len, "%.1f,%.1f,%.1f,%.1f\r\n", 
                            vofa_data->target_rpm, 
                            vofa_data->actual_rpm, 
                            vofa_data->pid_output, 
                            vofa_data->final_output);
    return len; // 返回封装后的数据长度

#elif VOFA_PROTOCOL == 2 // FireWater协议示例（RawData格式）, 暂未测试的代码

    // 以RawData格式封装数据，直接内存复制结构体到发送缓冲区
    if (buf_len < 16) { // 4个float数据需要16字节缓冲区 （4个float32）
        return 0; // 缓冲区不足，无法封装
    }
    // RawData协议示例：直接内存复制结构体到发送缓冲区，注意字节序和对齐问题
    memcpy(send_buf, &vofa_data->target_rpm, sizeof(float)); // 目标转速
    memcpy(send_buf + sizeof(float), &vofa_data->actual_rpm, sizeof(float)); // 实际转速
    memcpy(send_buf + sizeof(float) * 2, &vofa_data->pid_output, sizeof(float)); // PID原始输出 （未补偿的pid_output）
    memcpy(send_buf + sizeof(float) * 3, &vofa_data->final_output, sizeof(float)); // 死区补偿+限幅后的最终输出 （最终输出）
    return sizeof(VOFA_Data_t); // 返回封装后的数据长度
#else // 默认RawData格式，直接内存复制结构体到发送缓冲区
    return 0; // 协议选择错误，无法封装
#endif
}

/**
 * @brief  VOFA+模块发送数据到串口（封装上层接口，简化业务代码调用）
 * @note   无需手动传参，该函数可在speed_loop.c的主循环或定时器回调中调用，确保实时性
 */
void VOFA_Plus_SendData(VOFA_Data_t *data) {
    if(data == NULL) {
        return; // 数据指针无效，无法发送
    }

    // 根据选择的协议，封装数据打包
    uint16_t data_len = VOFA_Plus_PackData(data, vofa_send_buf, VOFA_SEND_BUF_MAX_LEN);
    if(data_len == 0) {
        return; // 数据封装失败，无法发送
    }

    // 发送封装后的数据
    HAL_UART_Transmit(&VOFA_UART_HANDLE, vofa_send_buf, data_len, 10); // 发送数据到串口，超时时间10ms
}

// 简化接口：从speed_loop.c获取当前数据并发送到VOFA+
void VOFA_Plus_SendSpeedLoopData(void) {
    // 从speed_loop.c获取当前调试数据
    VOFA_Data_t vofa_data = SpeedLoop_GetDebugData(); // 获取当前速度环调试数据，供VOFA+或显示屏使用
    
    // 将获取的数据发送到VOFA+
    VOFA_Plus_SendData(&vofa_data);
}
#endif
