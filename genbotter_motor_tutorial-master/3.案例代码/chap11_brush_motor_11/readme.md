# 直流有刷电机电流环闭环 PID 控制

基于 TIM1 TRGO 触发 ADC 注入采样的实时电流反馈，采用**位置式 PI 控制器在 ADC ISR 内 1000Hz 闭环**，实现 ±100mA 双向电流控制。

本项目基于 GenBotter Motor-1 电机开发板，STM32F407IGT6 主控，搭配 GenBotter 直流有刷电机驱动板（IR2104 半桥），在开环电流检测工程基础上扩展而来。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV1qWdmBqE3X)

---

## 🛒 硬件要求

- GenBotter Motor-1 电机开发板（STM32F407IGT6）
- 直流有刷电机（带 AB 相霍尔编码器）
- GenBotter 直流有刷电机驱动板（双 IR2104 半桥架构）
- 4.0寸 FSMC 并口 LCD 显示屏
- DapLink 下载器
- 按键和 LED 指示模块

---

## ⚡ 核心特性

- **位置式 PI**（Kp=0.4, Ki=0.08, Kd=0），ADC ISR 内 1000Hz 零延迟闭环
- **16kHz 中心对齐 PWM**（TIM1, ARR=2624），超出人耳范围
- **16× 过采样 + α=0.05 IIR 低通滤波**，等效噪声抑制 ~2bit
- **±100mA 双向控制**，支持正反转电流闭环
- **双重限幅保护**：PWM 输出 [-90%, +90%] + 积分限幅 ±1500
- **双模式对比**：开环（固定占空比）vs 闭环（PID 自动调节）
- **三键操作**：KEY0 启停闭环 / KEY1 开环步进 / KEY2 紧急失能

---

## 🔗 信号链架构

```
TIM1 16kHz 中心对齐 (ARR=2624) → TRGO Update (16kHz)
  ├→ CH1/CH1N PWM 输出 → IR2104 半桥 → 直流有刷电机
  └→ ADC1 注入触发 → ISR (16kHz):
       accum ×16 → avg → IIR (α=0.05) → current_mA (1000Hz)
         └→ PID_Compute (Kp=0.4, Ki=0.08)
              └→ BrushMotor_SetSpeed → 下个 PWM 周期生效
```

---

## 🎮 按键操作

| 按键 | 功能 | LED 反馈 |
|------|------|:--------:|
| **KEY0** | 启停闭环：+100mA → 0mA → -100mA 循环 | LED1 亮灭翻转 |
| **KEY1** | 开环占空比 ±20% 步进 | LED2 亮灭翻转 |
| **KEY2** | 紧急失能（PM1_CTRL_SD 拉低，全停） | LED1+2 熄灭 |

---

## 📂 主要目录结构

```text
├── Core/
│   ├── Inc/
│   │   ├── adc.h, tim.h, gpio.h, fsmc.h, usart.h, main.h
│   │   └── stm32f4xx_hal_conf.h, stm32f4xx_it.h
│   └── Src/
│       ├── main.c                       # 主程序（初始化、按键、LCD、串口）
│       ├── adc.c                        # ADC 注入中断回调（触发 PID 链）
│       ├── tim.c                        # TIM1 配置（16kHz 中心对齐 + TRGO）
│       └── stm32f4xx_it.c              # 中断服务函数
├── Drivers/
│   ├── BSP/
│   │   ├── Inc/
│   │   │   ├── bsp_brush_motor.h        # 有刷电机：PWM、启停、自举预充电
│   │   │   ├── bsp_current_sensor.h     # 电流传感器：校准、过采样、滤波
│   │   │   ├── bsp_key_led.h            # 按键扫描与 LED
│   │   │   └── bsp_lcd.h               # LCD 显示驱动
│   │   └── Src/
│   │       ├── bsp_brush_motor.c        # IR2104 半桥驱动、正反转、自举充电
│   │       ├── bsp_current_sensor.c     # 零漂校准、16×过采样、IIR滤波、ISR内PID
│   │       ├── bsp_key_led.c            # 按键消抖扫描、LED 控制
│   │       └── bsp_lcd.c               # FSMC LCD 底层驱动
│   └── Control/
│       ├── Inc/
│       │   ├── pid.h                    # 位置式 PID 接口（V2.0）
│       │   ├── current_loop.h           # 电流环控制接口
│       │   └── user_config.h            # 全部可调参数（Kp/Ki/α/过采样/限幅）
│       └── Src/
│           ├── pid.c                    # 位置式 PID 实现（P/I/D + 双重限幅）
│           └── current_loop.c           # 电流环闭环：方向死区、UVLO 恢复、SD 管理
└── brush_motor_11.ioc                   # STM32CubeMX 配置文件
```

### 关键文件说明

| 文件 | 功能 |
|------|------|
| `user_config.h` | 所有参数集中管理：Kp=0.4, Ki=0.08, OUTPUT_MAX=90%, INTEGRAL_LIMIT=1500, α=0.05, 16× 过采样 |
| `pid.c/h` | 位置式 PID：output = Kp·e + Ki·Σe + Kd·Δe，含输出 + 积分双重限幅 |
| `current_loop.c/h` | 电流环：方向检测优化（仅变向时写硬件）、死区 1%、IR2104 UVLO 自动恢复 |
| `bsp_current_sensor.c/h` | ADC 过采样 + IIR 滤波，1000Hz 电流更新后在 ISR 内立即调用 PID |
| `bsp_brush_motor.c/h` | IR2104 半桥 PWM 驱动、自举电容预充电（防冷启动 UVLO 锁死） |
| `main.c` | 初始化（零漂校准→自举预充电→CurrentLoopInit）、按键分发、LCD/串口输出 |
| `adc.c` | ADC1 注入通道 ISR：读取 ADC 值 → 调用过采样+滤波+PID 链 |

---

## 📐 控制参数（user_config.h）

```c
#define PID_KP              0.4f      // 比例系数（位置式，直接与误差成正比）
#define PID_KI              0.08f     // 积分系数（消除稳态误差）
#define PID_KD              0.0f      // 微分系数（电流环噪声大，通常为0）
#define OUTPUT_MAX          90.0f     // 输出上限（留10%给自举充电）
#define OUTPUT_MIN         -90.0f     // 输出下限（双向控制，支持反转）
#define INTEGRAL_LIMIT      1500.0f   // 积分限幅（防windup饱和）
#define CURRENT_OVERSAMPLE_COUNT  16u  // 过采样次数（16kHz/16=1000Hz PID）
#define CURRENT_LOWPASS_ALPHA  0.05f  // IIR滤波系数（配合过采样）
#define ADC_TO_MA_FACTOR     6.714f   // ADC→mA 转换（3.3V/4096/0.12V/A×1000）
#define CURRENT_TARGET_DEFAULT_mA  100.0f  // 默认目标电流
```

---

## ⚠️ 工程要点

- **PID 在 ADC ISR 内运行**（1000Hz），主循环仅负责按键/LCD/串口，不是传统的主循环调用
- **OUTPUT_MAX=90% 而非 100%**：IR2104 自举供电需要下管导通时间给 Cboot 充电，100% PWM 会导致高端 MOSFET 欠压锁死
- **OUTPUT_MIN=-90%**：必须为负值才支持 PID 输出反转方向控制，设 0 会截断所有负电流目标
- **方向检测优化**：`CurrentLoop_Update` 仅在方向变化时调用 `SetDirection`，避免每周期 SD 翻转
- **自举电容预充电**：上电时以 50% 互补 PWM 运行 10ms，消除掉电/UVLO 后冷启动不自举的问题
- **电流方向符号**：板载单采样电阻 (R32)，ADC 仅测 |I| 幅值，方向由 `g_motor_current_direction` 补充

---

## 📌 使用步骤

1. 在 STM32CubeMX 中打开 `brush_motor_11.ioc`，生成 Keil MDK 工程
2. 将本仓库源文件覆盖到生成的工程目录
3. 确保 Keil Include Path 包含 `Drivers/BSP/Inc`、`Drivers/Control/Inc` 等路径
4. 编译下载，使用 DapLink 烧录至开发板
5. 上电后自动完成零漂校准和自举预充电，等待按键操作

---

## 📎 调参指引

1. **Kp=0, Ki=0, Kd=0** → 逐步增大 Kp 直到电流能推动并靠近目标
2. **保留 Kp** → 逐步增大 Ki 消除稳态误差（每次观察 2~3 秒响应）
3. **Kd 保持为 0**（ADC 噪声会被微分放大）
4. **若振荡**：先减 Kp，再调 Ki
5. **若负方向无输出**：检查 OUTPUT_MIN 是否为负值
6. **若电流不到目标**：增大 Ki 和 INTEGRAL_LIMIT

---

💡 **提示**：如遇问题，可在仓库 Issue 区提交反馈。
