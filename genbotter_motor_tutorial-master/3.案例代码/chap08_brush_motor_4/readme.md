# 直流有刷电机的电压检测

在“brush_motor_3”的基础上，加入电压检测的实现。

通过ADC1的IN8获取电流模拟信号，通过ADC1的IN9获取电压模拟信号，进行模数转换实现电流与电压的同步检测。

基于GenBotter Motor-1电机开发板（STM32F407IGT6主控），配套GenBotter直流有刷电机驱动板和带AB相霍尔编码器的直流有刷电机，实现转速、电流、电压的同步测量项目。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)  
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV1b1CNBuEpR)（完整的电机教学视频网址）

---

## 🛒 硬件要求

- GenBotter Motor-1电机开发板
- 直流有刷电机（带AB相霍尔编码器）
- GenBotter直流有刷电机驱动板
- 4.0寸FSMC并口LCD显示屏（用于显示电流、电压、转速等测量结果）
- DapLink下载器（或ST-Link V2.1下载器）

---

## 📂 主要目录结构

```text
brush_motor_4/
├── Core/                 # 核心文件
│   ├── Inc/             # 头文件
│   │   ├── main.h
│   │   ├── adc.h
│   │   ├── gpio.h
│   │   ├── stm32f4xx_hal_conf.h
│   │   ├── stm32f4xx_it.h
│   │   ├── tim.h
│   │   └── usart.h
│   └── Src/             # 源文件
│       ├── main.c
│       ├── adc.c
│       ├── gpio.c
│       ├── stm32f4xx_hal_msp.c
│       ├── stm32f4xx_it.c
│       ├── system_stm32f4xx.c
│       ├── tim.c
│       └── usart.c
├── Drivers/              # 驱动层
│   ├── BSP/             # 板级支持包
│   │   ├── Inc/         # BSP头文件
│   │   │   ├── bsp_brush_motor.h
│   │   │   ├── bsp_delay.h
│   │   │   ├── bsp_encoder.h
│   │   │   ├── bsp_current_sensor.h
│   │   │   ├── bsp_voltage_sensor.h  # 新增：电压检测头文件
│   │   │   ├── bsp_key_led.h
│   │   │   ├── bsp_lcd.h
│   │   │   └── bsp_lcdfont.h
│   │   └── Src/         # BSP源文件
│   │       ├── bsp_brush_motor.c
│   │       ├── bsp_delay.c
│   │       ├── bsp_encoder.c
│   │       ├── bsp_current_sensor.c
│   │       ├── bsp_voltage_sensor.c  # 新增：电压检测源文件
│   │       ├── bsp_key_led.c
│   │       ├── bsp_lcd.c
│   │       └── bsp_lcd_ex.c
│   ├── CMSIS/           # CMSIS核心支持
│   └── STM32F4xx_HAL_Driver/ # HAL库驱动
├── MDK-ARM/             # Keil MDK工程文件
└── brush_motor_4.ioc    # STM32CubeMX配置文件（更新ADC配置以支持电压检测）
```

- 关键文件说明
  - bsp_encoder.c/h - 编码器驱动，负责 AB 相信号处理
  - bsp_current_sensor.c/h - 电机电流检测处理模块
  - bsp_voltage_sensor.c/h - 新增，电机电压检测处理模块，负责电压信号采集与转换
  - bsp_brush_motor.c/h - 电机控制驱动
  - bsp_lcd.c/h - LCD 显示驱动（新增电压数据显示逻辑）
  - bsp_key_led.c/h - 按键和 LED 控制
  - main.c - 主程序逻辑（同步处理电流、电压检测与数据展示）

**⚠️ 注意：未包含完整的 Keil MDK 工程文件。**

要编译运行案例，请参考以下步骤。

## 📌 使用步骤

1. 打开 CubeMX 工程
   1. 在 STM32CubeMX 中打开 brush_motor_4.ioc 文件
   2. 确认 ADC 配置（电流检测通道 ADC1_IN8 和电压检测通道 ADC1_IN9 的引脚、采样率等参数）
   3. 选择工程路径及工具链（Keil MDK-ARM）
2. 生成 Keil 工程
   1. 点击 GENERATE CODE
   2. CubeMX 将生成一个 Keil MDK 工程目录
3. 复制源代码
   1. 将本文件夹中的 .c 和 .h 文件（包括新增的电压检测相关文件）复制或覆盖到生成的 Keil 工程目录
   2. 若提示覆盖，请选择 覆盖已有文件
4. 添加头文件路径
   1. 打开 Keil 工程
   2. 进入 Options for Target → C/C++ → Include Paths
   3. 添加所有头文件所在路径（确保包含新增的 bsp_voltage_sensor.h 路径），避免编译错误
5. 编译与下载
   1. 在 Keil 中点击 Build 进行编译
   2. 使用下载器烧录至开发板

## 📎 注意事项

- 请确保 CubeMX 与 Keil MDK 版本兼容
- 电流检测仍使用 ADC1_IN8，需确保该通道配置未被修改
- 电压检测需确认 ADC1_IN9 与硬件电路的对应关系，避免引脚冲突
- 若遇到编译报错，优先检查 Include Path 是否设置完整 及新增文件是否正确添加
- 建议在 Keil 中开启 警告信息显示，方便调试
- 首次运行时，可通过 LCD 确认电流、电压数据是否正常显示，若异常请检查 ADC 配置及硬件连接

## 💡 提示

如遇问题，可在仓库 Issue 区提交反馈。
