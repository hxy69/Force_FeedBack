# chap13_brush_motor_14 - 直流有刷电机电流环+速度环双环串级控制

> 使用 **STM32CubeMX + Keil MDK** 开发  
> 适配 **GenBotter Motor-1 开发板**、**GenBotter 直流有刷电机驱动板** 与 **MC520 直流减速电机（带霍尔编码器）**

---

## 📌 项目简介

实现**直流有刷电机电流环+速度环双环串级 PID 控制**——速度外环输出作为电流内环的给定值，电流内环直接控制 PWM 占空比。ADC 注入采样 16kHz 过采样采集电流，TIM3 编码器模式读取速度反馈。

**核心架构：串级双环 — 速度PID → 电流PI → PWM**

```
 目标转速(rpm) ──→ 速度PID(外环) ──→ 目标电流(mA) ──→ 电流PI(内环) ──→ PWM占空比 ──→ 电机
      ↑                  ↑                          ↑
 编码器速度反馈     设定值传递                  ADC电流反馈 (16kHz)
```

> 💡 **设计理念**：对比 chap12 的单环位置 PID，演示串级控制的优势——电流内环限制转矩过冲，速度外环负责跟踪目标，响应更快、更稳定。

---

## ⚙️ 硬件清单

- GenBotter Motor-1 开发板（STM32F407IGT6）
- GenBotter 直流有刷电机驱动板
- GenBotter 4 寸电容屏（FSMC 接口）
- MC520 直流减速电机（带霍尔编码器，11 PPR × 30 减速比）
- ST-Link / DAP-Link 调试下载器
- 12V 直流电源

---

## 🔌 关键接口连接（PM1 接口）

| PM1 Pin | 信号名 | MCU GPIO | 功能 |
|---------|--------|----------|------|
| 9 | PWM_UH | PA8 (TIM1_CH1) | 半桥 PWM 输出 |
| 10 | PWM_UL | PB13 (TIM1_CH1N) | 互补 PWM |
| 11 | ENCA | PC6 (TIM3_CH1) | 编码器 A 相 |
| 12 | ENCB | PC7 (TIM3_CH2) | 编码器 B 相 |
| 22 | CTRL_SD | PF10 | IR2104 关断 |

> ADC1 注入通道采样电流传感器输出（16kHz TRGO 触发，中心对齐 PWM 中点采样）。

---

## 📐 编码器参数

| 参数 | 值 | 说明 |
|------|-----|------|
| ENCODER_PPR | **44** | 4倍频后计数值 |
| 物理线数 | 11 PPR | 霍尔编码器每通道脉冲数 |
| GEAR_RATIO | **30.0** | 减速比 |
| 输出轴一圈 | **1320 counts** | = 44 × 30 |
| 每 RPM | **22 counts/s** | = 1320 ÷ 60 |

---

## ⏱️ 定时调度

```
TIM1 (16kHz center-aligned):
  └─ TRGO=Update → ADC 注入采样 (16kHz)
     └─ 16次过采样平均+LPF → current_measured

TIM6 (1kHz 周期中断):
  ├─ 每次触发: 电流PI计算 (1kHz)   ← 内环
  └─ 每10次:   速度PID计算 (100Hz) ← 外环 + 编码器速度更新
```

| 调度项 | 周期 | 用途 |
|--------|:---:|------|
| ADC 采样 | 16kHz | 电流过采样 + LPF |
| 电流 PI | **1kHz** | 内环快速响应，电流指令跟踪 |
| 速度 PID | **100Hz** | 外环，目标转速跟踪 |
| LCD 刷新 | 500ms | 显示目标/实测速度、电流 |
| 串口输出 | 100ms | VOFA+ FireWater 协议实时曲线 |

---

## 🛠️ 双环控制架构

### 速度外环（速度 PID → 电流给定值）

```
目标 rpm  ──→ [速度 PID] ──→ 电流给定值(mA) ──→ 限幅(±CUR_MAX)
  ↑                    │
编码器速度(m/s)  ──→ 反馈
```

- PID 参数：`user_config.h` 中可配置 Kp/Ki/Kd
- 输出限幅：±CUR_MAX（保护电机和驱动）
- 速度推算：编码器差分法 @100Hz

### 电流内环（电流 PI → PWM 占空比）

```
电流给定(mA) ──→ [电流 PI] ──→ PWM 占空比 ──→ 限幅 → TIM1 CCR
  ↑                    │
ADC 电流(mA)  ──→ 反馈 (16kHz 过采样+LPF)
```

- PI 参数：`user_config.h` 中可配置
- 占空比限幅：防止过流（±MAX_PWM_PERCENT）

---

## 🎮 按键操作

| 按键 | 功能 | 效果 |
|------|------|------|
| **KEY0** | 加速 | 目标转速 +20 RPM，首次按下自动启动 |
| **KEY1** | 减速 | 目标转速 -20 RPM，首次按下自动启动 |
| **KEY2** | 停止 | 停止双环控制，PWM 清零，SD 拉低 |

| LED | 指示 | 含义 |
|-----|------|------|
| LED1 | 运行状态 | 亮=双环运行，灭=停止 |

---

## 📊 运行效果

- 上电 LCD 显示「Chap13: Cur-Speed PID」
- 校准零偏后显示「STOPPED」
- KEY0/KEY1 首次按下自动启动双环，±20 RPM 调整目标速度
- LCD 实时显示：
  - 目标转速 / 实测转速
  - 电流给定值 / 实测电流值
  - 电流误差 / 速度误差
- 串口 VOFA+ 输出 5 通道数据：目标/实测转速、电流给定/实测、PWM 占空比
- 目标速度范围：±200 RPM

---

## 📁 工程结构

```text
chap13_brush_motor_14/
├── Core/
│   ├── Inc/               # HAL 外设头文件 (adc.h, tim.h, usart.h, fsmc.h, gpio.h)
│   └── Src/               # main.c + HAL 外设实现 (adc.c, tim.c, usart.c, fsmc.c)
├── Drivers/
│   ├── BSP/               # 板级驱动
│   │   ├── Inc/           # bsp_brush_motor.h, bsp_encoder.h, bsp_current_sensor.h, bsp_lcd.h, bsp_key_led.h, bsp_delay.h
│   │   └── Src/           # 对应的 .c 实现
│   ├── Control/           # 控制算法层
│   │   ├── Inc/           # cur_speed_loop.h, pid.h, user_config.h
│   │   └── Src/           # cur_speed_loop.c, pid.c
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── MDK-ARM/
│   └── brush_motor_14.uvprojx
├── brush_motor_14.ioc
└── readme.md
```

---

## ⚠️ 注意事项

- PID 参数在 `Drivers/Control/Inc/user_config.h` 中统一管理，调试时优先修改此文件
- 电流传感器需**上电零偏校准**，代码在启动时自动执行
- 开关管 SD 控制：停止时拉低 SD 关断 IR2104，防止误触发
- 串级控制内环频率 1kHz 远高于外环 100Hz，由 TIM6 中断软件分频实现
- 占空比限幅在 `user_config.h` 的 `MAX_PWM_PERCENT` 中配置
- VOFA+ 串口波特率 115200，每秒 10 帧（100ms/帧）

---

## 🔜 下一章预告

[chap14_brush_motor_15](../chap14_brush_motor_15/) 将在本工程基础上引入位置环，实现**电流-速度-位置三环串级**的完整伺服控制系统。

---

## 📄 许可证

本项目仅供学习与非商业用途。

---

**GenBotter Robotics Lab**  
Dr. GAO — 2026年7月
