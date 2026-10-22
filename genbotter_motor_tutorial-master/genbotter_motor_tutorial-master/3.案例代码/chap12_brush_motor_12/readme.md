# chap12_brush_motor_12 - 直流有刷电机位置环PID控制（纯PID定位）

> 使用 **STM32CubeMX + Keil MDK + VSCode** 开发  
> 适配 **GenBotter Motor-1 开发板、GenBotter 直流有刷电机驱动板** 与 **MC520 直流减速电机（带霍尔编码器）**

---

## 📌 项目简介

实现**直流有刷电机位置环PID闭环定位控制**——给定目标角度，位置PID直接输出PWM占空比驱动电机到位。

**核心架构：单环 — 位置PID直驱PWM**

```
 目标位置 ──→ 位置PID(PD) ──→ [PWM滤波] ──→ PWM占空比 ──→ 电机
                ↑
          编码器位置反馈
```

> 💡 **设计理念**：让学生亲眼看到纯位置PID在大角度定位时必然出现的超调与振荡。这是故意不加速度内环、不加轨迹规划的"反面教材"——为下一节（chap12_brush_motor_13 梯形加减速）的对比做铺垫。

---

## ⚙️ 硬件清单

- GenBotter Motor-1 开发板（STM32F407IGT6）
- GenBotter 直流有刷电机驱动板
- Genbotter 4寸电容屏
- MC520 直流减速电机（带霍尔编码器，物理11 PPR × 30减速比）
- ST-Link 或 DAP-Link 调试下载器
- 12V 直流电源

---

## 🔌 关键接口连接（PM1 接口 / 直流有刷驱动板）

> PM1 是 Motor-1 控制板的 24Pin 电机接口，驱动板通过 24P FFC 排线直插连接。
> 直流有刷电机仅使用以下引脚：

| PM1 Pin | 信号名   | MCU GPIO        | 功能说明                |
|---------|----------|-----------------|------------------------|
| 9       | PWM_UH   | PA8 (TIM1_CH1)  | 半桥A PWM（正转时输出）   |
| 10      | PWM_UL   | PB13 (TIM1_CH1N)| 半桥B PWM（反转时输出）   |
| 11      | ENCA     | PC6 (TIM3_CH1)  | 编码器A相               |
| 12      | ENCB     | PC7 (TIM3_CH2)  | 编码器B相               |
| 22      | CTRL_SD  | PF10            | IR2104 关断（低电平关闭） |

---

## 📐 编码器与位置参数

> 参数来源：`Drivers/BSP/Inc/bsp_encoder.h`（本项目实际配置）

| 参数 | 值 | 说明 |
|------|-----|------|
| ENCODER_PPR | **44** | 4倍频后计数值（TIM3 编码器模式 TI12 自动4倍频） |
| 物理线数 | 11 PPR | 霍尔编码器每通道脉冲数（= 44 ÷ 4） |
| GEAR_RATIO | **30.0** | MC520 减速电机减速比 |
| 输出轴一圈 | **1320 counts** | = 44 × 30，每输出转的编码器计数值 |
| 每度 | ≈ 3.667 counts | = 1320 ÷ 360，位置分辨率 ≈ 0.27° |
| KEY0/KEY1 步进 | **±1320 counts** | 360°（一整圈）|

---

## ⏱️ 软件定时调度（TIM6 1ms 基频）

TIM6 配置为 **1ms 硬件定时中断**，作为整个控制系统的"心跳"。所有周期性任务在 ISR 中通过软件计数器分频执行，避免阻塞式延时，保证实时性。

```
TIM6 1ms ISR:
  └─ tick++
       ├─ 1ms:   无操作（预留）
       ├─ 50ms:  位置PID计算 → LPF滤波 → PWM输出（20Hz闭环）
       ├─ 100ms: 编码器速度计算（差分法）
       └─ 200ms: LCD刷新 + VOFA+串口上报
```

| 调度项 | 周期 | 用途 |
|--------|:---:|------|
| 位置 PID | **50ms** | 核心控制闭环，20Hz，与低分辨率编码器匹配 |
| 速度计算 | 100ms | 用于串口监视和调试 |
| 位置 PID + 滤波 | **50ms** | PID 计算后立即 LPF 平滑，再写入 PWM 寄存器 |
| LCD + 串口 | 200ms | 显示和上报，慢速 I/O 不拖累控制周期 |

> **为什么选 50ms？**  
> - 11 PPR × 30 减速比，满速时每 50ms ≈ 110 counts 差分，速度估计噪声可控
> - 1ms/10ms：差分 counts 太少（≈2/~22），速度噪声严重，编码器跳动占比大
> - 100ms：相位滞后过大，大角度定位时响应迟钝、超调更严重
> - **50ms**：低分辨率编码器 + 高减速比场景的工程折中点

> **⚠️ PID 周期 ≠ 速度计算周期**  
> 位置环 PID 只用编码器**绝对计数值**（target vs actual），不需要速度。  
> 差分速度（100ms）仅用于 LCD/串口显示，不进控制回路 → 两者周期独立。

## 🔧 工程补丁

单环位置PID在阶跃输入下不可避免超调，以下三项措施是在不改变控制架构的前提下，最实用的工程优化：

### ① 到位死区（Dead Zone）

编码器静止时因齿轮间隙存在 ±2~3 counts 随机跳动。不加死区时，PID 永远试图纠正这几个 count，导致电机持续微振、发热：

```c
if (fabs(error) < DEAD_ZONE) {
    pwm = 0;
    state = HOLDING;
    return;
}
```

死区值取 **4 counts**（≈ 1°，对应代码 DEAD_ZONE），小于此误差即判定到位。

### ② PWM 输出低通滤波（LPF）

PID 高频噪声会直接写入 PWM 寄存器，导致电机啸叫。在 PWM 更新路径上串一级 IIR：

```
pwm_filtered = pwm_filtered × 0.5 + pid_output × 0.5
```

本质是一阶低通，截止频率由 α=0.5 决定。从 50ms PID 周期角度看，相当于对高频抖动平滑 2 个控制周期。

### ③ PWM 占空比限幅

```c
#define PWM_LIMIT       50     // 正向限幅 +50%（PWM 百分比域，1 单位 = 1% 占空比）
#define PWM_LIMIT_NEG  (-50)   // 反向限幅 -50%

if (pwm >  PWM_LIMIT)     pwm = PWM_LIMIT;
if (pwm <  PWM_LIMIT_NEG) pwm = PWM_LIMIT_NEG;
```

限制最大占空比 ±50%：防止 12V 电机满速→过流烧毁芯片，同时让 180° 阶跃不会瞬间全速冲出（软限速）。

---

## 🤖 运行状态机

```
 IDLE ──[新目标]──→ MOVING ──[|error| < 死区]──→ HOLDING
   ↑                   │                            │
   └───[KEY2 复位]─────┴──[新目标到达]───────────────┘
```

| 状态 | PID 计算 | PWM 输出 | 说明 |
|------|:---:|:---:|------|
| **IDLE** | ❌ | 0 | 上电默认，等待按键指令 |
| **MOVING** | ✅ | 计算值 | 正在定位，PID 活跃 |
| **HOLDING** | ✅ | 计算值 | 死区内继续 PID 保持位置，外力推离时自动恢复 |

- 进入 HOLDING 后 PID 继续运行——增量式输出的累加值自带"保持力"，KI=0 时外力推离死区也能自动拉回
- KEY2 在任何状态按下立即复位到原点（SetPoint=0, Position=0）

---

## 🛠️ 开发流程

1. **外设配置**：STM32CubeMX 配置 TIM1（PWM互补输出）、TIM3（编码器模式）、TIM6（1ms定时中断）、USART1（串口调试 / VOFA+）
2. **代码编辑**：VSCode 编辑 `Core/Src/`、`Drivers/Control/` 中的业务逻辑
3. **编译烧录**：Keil MDK 编译 → ST-Link/DAP-Link 下载调试

---

## 📁 工程结构

```text
chap12_brush_motor_12/
├── .vscode/               # VSCode 工作区配置
├── Core/
│   ├── Inc/               # HAL 外设头文件
│   └── Src/               # main.c + 中断服务
├── Drivers/
│   ├── BSP/               # 板级驱动（编码器、电机、LCD、按键）
│   │   ├── Inc/           # bsp_encoder.h, bsp_brush_motor.h …
│   │   └── Src/           # bsp_encoder.c, bsp_brush_motor.c …
│   └── Control/           # 控制算法层
│       ├── Inc/           # pid.h, position_loop.h, user_config.h
│       └── Src/           # pid.c, position_loop.c
├── MDK-ARM/               # Keil MDK 工程目录
│   ├── brush_motor_12.uvprojx   # Keil 工程文件
│   └── startup_stm32f407xx.s    # 启动文件
├── brush_motor_12.ioc     # CubeMX 配置文件
├── .mxproject             # CubeMX 工程元数据
└── readme.md              # 本说明文档
```

---

## 🎮 按键操作

| 按键 | 功能 | 编码器偏移 |
|------|------|:---:|
| KEY0 | 目标位置 +360°（顺时针一整圈） | +1320 counts |
| KEY1 | 目标位置 -360°（逆时针一整圈） | -1320 counts |
| KEY2 | 复位到原点（SetPoint = 0, 计数值清零） | 0 |

---

## 📊 运行效果

- 上电 LCD 显示「Chap12: Position PID」
- 串口（115200, 8-N-1）输出：目标位置、实际位置、PID输出、PWM占空比
- LED0 心跳闪烁（200ms），指示系统运行
- 按 KEY0/KEY1 瞬间改变目标位置 → **可观察到位置超调、转速振荡**
- 可以通过vofa+观察pid位置跟踪效果曲线

---

## ⚠️ 注意事项

- **KI 取极小值 0.01**（近似纯 PD 控制，对应代码 POS_KI=0.01）：位置环积分会累积超调阶段的误差，使过冲更严重，本章刻意压到极小
- 控制周期：TIM6 基频 1ms，PID 计算周期 50ms（通过软件计数器分频，对应 PID_PERIOD_TICKS=50）
- PWM 频率：≈500Hz（TIM1 中心对齐，PSC=167、ARR=999、计数时钟 1MHz），互补输出
- 占空比限幅：±50%（防止过流）
- 编码器溢出保护：TIM3 ARR=65535，`BSP_Encoder_HandleOverflow()` 处理 16 位回绕
- main 循环为非阻塞设计：所有任务由 TIM6 ISR 调度，while(1) 仅处理按键扫描

---

## 🔜 下一章预告

[chap12_brush_motor_13](../chap12_brush_motor_13/) 将在本工程基础上加入**梯形速度轨迹规划**，用相同PID参数对比定位效果——无超调、无振荡、平滑到位。

---

## 📄 许可证

本项目仅供学习与非商业用途。

---

**GenBotter Robotics Lab**  
Dr. GAO — 2026年7月
