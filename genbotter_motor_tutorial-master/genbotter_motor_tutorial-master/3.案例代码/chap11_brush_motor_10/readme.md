# 直流有刷电机的电流检测

通过ADC注入通道获取模拟信号进行模数转换，实现直流有刷电机的开环电流检测。

基于GenBotter Motor-1电机开发板，STM32F407IGT6主控，配套GenBotter直流有刷电机驱动板的直流有刷电机电流检测与PWM调速项目。

本项目使用STM32F4系列微控制器，通过TIM1触发ADC注入转换实现电流实时采样，结合PWM输出对直流有刷电机进行开环调速控制。项目包含完整的硬件驱动和软件实现。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)  
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV1qWdmBqE3X)

---

## 🛒 硬件要求

- GenBotter Motor-1电机开发板
- 直流有刷电机（带AB相霍尔编码器）
- GenBotter直流有刷电机驱动板
- 4.0寸FSMC并口LCD显示屏（用于显示测量结果）
- DapLink下载器
- 按键和LED指示模块（用于操作与状态反馈）

---

## 📂 主要目录结构

```text
├── Core/                    # 核心文件
│   ├── Inc/                 # 头文件
│   │   ├── adc.h
│   │   ├── fsmc.h
│   │   ├── gpio.h
│   │   ├── main.h
│   │   ├── stm32f4xx_hal_conf.h
│   │   ├── stm32f4xx_it.h
│   │   ├── tim.h
│   │   └── usart.h
│   └── Src/                 # 源文件
│       ├── adc.c
│       ├── fsmc.c
│       ├── gpio.c
│       ├── main.c
│       ├── stm32f4xx_hal_msp.c
│       ├── stm32f4xx_it.c
│       ├── system_stm32f4xx.c
│       ├── tim.c
│       └── usart.c
├── Drivers/                  # 驱动层
│   ├── BSP/                  # 板级支持包
│   │   ├── Inc/              # BSP头文件
│   │   │   ├── bsp_brush_motor.h
│   │   │   ├── bsp_current_sensor.h
│   │   │   ├── bsp_delay.h
│   │   │   ├── bsp_key_led.h
│   │   │   ├── bsp_lcd.h
│   │   │   └── bsp_lcdfont.h
│   │   └── Src/              # BSP源文件
│   │       ├── bsp_brush_motor.c
│   │       ├── bsp_current_sensor.c
│   │       ├── bsp_delay.c
│   │       ├── bsp_key_led.c
│   │       ├── bsp_lcd.c
│   │       └── bsp_lcd_ex.c
│   ├── Control/              # 控制算法
│   │   ├── current_loop.c
│   │   ├── current_loop.h
│   │   ├── pid.c
│   │   ├── pid.h
│   │   └── user_config.h
│   ├── CMSIS/                # CMSIS核心支持
│   └── STM32F4xx_HAL_Driver/ # HAL库驱动
└── brush_motor_10.ioc        # STM32CubeMX配置文件
```

- 关键文件说明
  - bsp_current_sensor.c/h - 电机电流检测处理模块
  - bsp_brush_motor.c/h - 电机控制驱动
  - current_loop.c/h - 电流环控制逻辑
  - pid.c/h - PID控制器
  - bsp_lcd.c/h - LCD显示驱动
  - bsp_key_led.c/h - 按键和LED控制
  - main.c - 主程序逻辑

⚠️ **注意：本仓库未包含 Keil MDK 工程文件（MDK-ARM 目录），需自行通过 CubeMX 生成。**

要编译运行案例，请参考以下步骤。

---

## 📌 使用步骤

1. **打开 CubeMX 工程**
   - 在 STM32CubeMX 中打开 `.ioc` 文件  
   - 选择工程路径及工具链（Keil MDK-ARM）

2. **生成 Keil 工程**
   - 点击 `GENERATE CODE`  
   - CubeMX 将生成一个 Keil MDK 工程目录

3. **复制源代码**
   - 将本文件夹中的 `.c` 文件复制或覆盖到生成的 Keil 工程目录  
   - 若提示覆盖，请选择 **覆盖已有文件**

4. **添加头文件路径**
   - 打开 Keil 工程  
   - 进入 `Options for Target` → `C/C++` → `Include Paths`  
   - 添加缺失的路径，避免编译错误

5. **编译与下载**
   - 在 Keil 中点击 **Build** 进行编译  
   - 使用下载器烧录至开发板

---

## 📎 注意事项

- 请确保 CubeMX 与 Keil MDK 版本兼容  
- 若遇到编译报错，优先检查 **Include Path 是否设置完整**  
- 建议在 Keil 中开启 **警告信息显示**，方便调试

---

💡 **提示**  
如遇问题，可在仓库 Issue 区提交反馈。  
