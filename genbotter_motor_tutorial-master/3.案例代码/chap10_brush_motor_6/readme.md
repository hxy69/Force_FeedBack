# 直流有刷电机 PID 速度环控制（含位置式、增量式PID代码）

在“brush_motor_5”的基础上，实现速度环的pid控制。

通过ADC1的IN8获取电流模拟信号，通过ADC1的IN9获取电压模拟信号，通过ADC1的IN0获取温度传感器电压变化的模拟信号，进行模数转换实现电流与电压的同步检测。

基于GenBotter Motor-1 电机开发板（STM32F407IGT6） 实现直流有刷电机位置式 PID 速度闭环控制，同步通过 ADC 完成电机电流、电压、温度三参数采集检测，配套 LCD 显示屏实时显示转速、电流、电压、温度等关键数据，是嵌入式电机控制入门的完整实战项目。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)  
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

🎯 项目核心功能

- 位置式 PID 速度闭环：基于 AB 相霍尔编码器的转速反馈，实现电机精准速度控制，支持目标转速设置、实际转速采集；
- 多参数同步检测：通过 ADC 完成模拟信号采集与模数转换，实时检测电机工作电流、供电电压、温度；
- 数据可视化：4.0 寸 FSMC 并口 LCD 显示屏，实时刷新显示目标转速 / 实际转速、电流、电压、温度；
- 硬件保护机制：PID 输出限幅、积分抗饱和、电机 PWM 死区补偿，避免执行器过载 / 发热；
- 模块化驱动：板级支持包（BSP）分层设计，驱动与控制逻辑解耦，代码易读、易拓展、易移植。

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV18S6FBnEtS)（直流有刷电机PID速度环控制实现）

---

## 🛒 硬件要求

| 硬件模块         | 规格/功能说明                     | 备注                     |
| ---------------- | --------------------------------- | ------------------------ |
| 主控板           | GenBotter Motor-1（STM32F407IGT6）| 项目核心主控             |
| 执行机构         | 直流有刷电机（带AB相霍尔编码器）| 支持转速反馈，适配PID控制 |
| 驱动板           | GenBotter 直流有刷电机专用驱动板  | 提供电机PWM/方向控制      |
| 显示模块         | 4.0寸FSMC并口LCD显示屏            | 实时显示转速/电流/电压/温度 |
| 程序下载器       | DapLink / ST-Link V2.1            | 支持烧录+在线调试        |

---

## 📂 主要目录结构

```text
chap10_brush_motor_6/
├── Core/                 # 系统核心层（CubeMX自动生成+主逻辑）
│   ├── Inc/             # 核心头文件
│   │   ├── main.h          # 主程序头文件
│   │   ├── adc.h           # ADC底层配置头文件
│   │   ├── gpio.h          # GPIO底层配置头文件
│   │   ├── tim.h           # 定时器底层配置头文件（控制周期/PWM）
│   │   ├── usart.h         # 串口底层配置头文件（调试可选）
│   │   ├── stm32f4xx_hal_conf.h # HAL库配置文件
│   │   └── stm32f4xx_it.h  # 中断服务函数头文件
│   └── Src/             # 核心源文件
│       ├── main.c          # 主程序（系统初始化+任务调度）
│       ├── adc.c           # ADC底层驱动实现
│       ├── gpio.c          # GPIO底层驱动实现
│       ├── tim.c           # 定时器底层驱动实现（TIM6/PWM定时器）
│       ├── stm32f4xx_hal_msp.c # HAL库MSP初始化
│       ├── stm32f4xx_it.c  # 中断服务函数实现（定时器/ADC）
│       └── system_stm32f4xx.c # 系统时钟配置
├── Drivers/              # 驱动层（板级支持包+BSP+库文件）
│   ├── BSP/             # 板级支持包（项目核心驱动，教学重点）
│   │   ├── Inc/         # BSP模块头文件
│   │   │   ├── bsp_brush_motor.h  # 直流有刷电机驱动（PWM/方向）
│   │   │   ├── bsp_encoder.h      # AB相霍尔编码器驱动（转速计算）
│   │   │   ├── bsp_current_sensor.h # 电流检测驱动（ADC采集+量程转换）
│   │   │   ├── bsp_voltage_sensor.h # 电压检测驱动（ADC采集+量程转换）
│   │   │   ├── bsp_temper_sensor.h # 温度检测驱动（ADC采集+温转）
│   │   │   ├── bsp_lcd.h           # LCD显示屏驱动（FSMC并口）
│   │   │   ├── bsp_key_led.h       # 按键/LED驱动（交互/状态指示）
│   │   │   └── bsp_delay.h         # 延时驱动（毫秒/微秒）
│   │   └── Src/         # BSP模块源文件
│   │       ├── bsp_brush_motor.c  # 电机驱动实现
│   │       ├── bsp_encoder.c      # 编码器转速计算实现
│   │       ├── bsp_current_sensor.c # 电流检测实现
│   │       ├── bsp_voltage_sensor.c # 电压检测实现
│   │       ├── bsp_temper_sensor.c # 温度检测实现
│   │       ├── bsp_lcd.c           # LCD显示实现（含数据刷新）
│   │       ├── bsp_key_led.c       # 按键/LED实现
│   │       └── bsp_delay.c         # 延时实现
│   ├── Control/         # 控制层（PID算法与速度环，教学重点）
│   │   ├── Inc/         # 控制层头文件
│   │   │   ├── pid_controller.h    # PID控制器通用头文件（位置式/增量式）
│   │   │   ├── speed_loop.h        # 速度环控制头文件（PID应用层）
│   │   │   └── control_config.h    # 控制参数配置文件（PID/硬件参数）
│   │   └── Src/         # 控制层源文件
│   │       ├── pid_controller.c    # PID控制器核心实现（教学版）
│   │       └── speed_loop.c        # 速度环控制实现（PID闭环）
│   ├── CMSIS/           # CMSIS核心库（STM32内核支持）
│   └── STM32F4xx_HAL_Driver/ # STM32F4xx HAL库官方驱动
├── MDK-ARM/             # Keil MDK-ARM工程文件（编译/链接/调试）
│   ├── brush_motor_6.uvprojx # Keil工程主文件
│   └── Objects/         # 编译中间文件/可执行文件
└── brush_motor_6.ioc  # STM32CubeMX配置文件（硬件引脚/外设配置）
```

## 🔑 核心文件说明（教学/开发重点）

| 文件路径 | 核心作用 | 教学重点 |
| --------------------------------- | ------------------------------------------ | ---------------------------- |
| Drivers/Control/Inc/pid_controller.h/c | 通用PID控制器，支持位置式/增量式           | PID核心算法、参数初始化      |
| Drivers/Control/Inc/speed_loop.h/c     | 速度环控制层，封装PID速度闭环逻辑           | 闭环控制流程、转速反馈处理   |
| Drivers/Control/Inc/control_config.h   | 全局控制参数配置，集中定义PID/硬件参数      | 参数调试、工程化配置思想     |
| Drivers/BSP/Inc/bsp_encoder.h/c    | AB相霍尔编码器驱动，完成转速计算           | 脉冲计数、转速物理量转换     |
| Drivers/BSP/Inc/bsp_*_sensor.h/c   | 电流/电压/温度检测驱动                     | ADC采集、模拟量转实际量      |

**⚠️ 注意：未包含完整的 Keil MDK 工程文件。**

要编译运行案例，请参考以下步骤。

## 📌 快速上手（编译 + 烧录 + 运行）

### 环境准备

- 软件：STM32CubeMX 6.0+、Keil MDK-ARM 5.36+（需安装 STM32F4xx 器件库）；
- 硬件：按接线说明完成开发板、电机、驱动板、LCD、下载器的硬件连接；
- 驱动：安装 DapLink/ST-Link 驱动，确保电脑能识别下载器。

### 操作步骤

1. 打开并配置 CubeMX 工程
用 STM32CubeMX 打开工程根目录的brush_motor_6.ioc文件；
确认外设配置：ADC1（IN8/IN9/IN0）、定时器（TIM6：10ms 控制周期）、PWM 定时器、FSMC（LCD）、GPIO（电机方向 / 按键 / LED）；
确认工程配置：工具链选择Keil MDK-ARM，工程路径与本项目一致，避免中文路径。
2. 生成 Keil 工程文件
点击 CubeMX 顶部GENERATE CODE，生成最新的 Keil 工程文件；
若提示覆盖原有文件，选择覆盖（保留项目自定义驱动代码）。
3. 配置 Keil 工程并编译
打开MDK-ARM/brush_motor_6.uvprojx工程文件；
配置头文件路径：点击Options for Target → C/C++ → Include Paths，添加所有Inc/文件夹路径（核心层 + BSP 层），避免编译报错；
配置下载器：点击Debug → Use，选择DAP-Link Debugger/ST-Link Debugger，点击Settings → Flash Download，添加 STM32F407IGT6 的 Flash 算法；
点击Build（编译）或Rebuild（重新编译），确保工程0 Error 0 Warning。
4. 程序烧录与运行
将下载器连接开发板与电脑，开发板上电；
点击 Keil 顶部Download，将程序烧录至 STM32 主控；
烧录完成后，点击Reset重启开发板，程序自动运行：LCD 显示屏刷新显示参数，电机按默认目标转速启动。

## 🚨 注意事项（避坑指南）

### 硬件层面

- 确保电机、驱动板、开发板的供电电压匹配，避免过压 / 欠压损坏硬件；
- 编码器接线需注意AB 相序，接反会导致转速计算错误、PID 控制异常；
- ADC 采集通道（IN8/IN9/IN0）需与硬件电路对应，避免引脚接错导致检测数据异常；
- LCD 显示屏的 FSMC 并口接线需牢固，接触不良会导致显示花屏 / 无显示。

### 软件层面

- CubeMX 与 Keil 版本需兼容，建议使用 CubeMX6.0 + 搭配 Keil5.36+；
- 编译前务必添加所有头文件路径，否则会出现 “头文件未定义” 编译错误；
- 控制周期（TIM6）需与control_config.h中CTRL_PERIOD_MS保持一致，否则 PID 参数失效；
- 烧录程序前需配置正确的 Flash 算法，否则会出现烧录失败 / 程序无法运行；
- 修改 PID 参数后，需重新编译烧录，参数修改不会自动生效。

### 调试层面

- 首次运行若电机无响应，优先检查电机方向引脚、PWM 引脚、编码器引脚的接线与配置；
- 若转速显示为 0，检查编码器是否正常工作、bsp_encoder.c中脉冲计数逻辑是否正确；
- 若 PWM 直接拉满 100%，检查转速反馈是否为 0、PID 传参是否正确（避免将初始值传入 PID）；
- 若电机转速抖动，可在速度环中添加一阶低通滤波，平滑编码器转速反馈。

## 💡 提示

- 配套视频教程：直流有刷电机 PID 速度环控制实现（[B站高博士_嵌入式](https://space.bilibili.com/486637340)）；
- 硬件问题：请前往[GenBotter淘宝店铺](https://genbotter.taobao.com)咨询客服；
- 代码 / 编译问题：本工程的 issues 留言反馈，会及时回复解答。
