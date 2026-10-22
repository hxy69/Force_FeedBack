# 直流有刷电机 PWM 调速与正反转控制

直流有刷电机入门第一个案例：通过 PWM 实现电机调速，通过 IR2104 半桥切换实现正反转，配合板载按键完成人机交互。

基于 GenBotter Motor-1 电机开发板（STM32F407IGT6 主控），配套 GenBotter 直流有刷电机驱动板，实现直流有刷电机的开环 PWM 调速与方向控制。本案例不使用编码器、LCD 和传感器，专注于最基础的电机驱动原理。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)  
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV15PMFzFEGJ)

---

## 🛒 硬件要求

- GenBotter Motor-1 电机开发板（STM32F407IGT6）
- 直流有刷电机
- GenBotter 直流有刷电机驱动板（IR2104 半桥）
- DapLink 下载器（或 ST-Link V2.1 下载器）
- 板载按键（KEY0/KEY1/KEY2）与 LED（LED0/LED1）用于交互与状态指示

---

## ⚙️ 功能说明

- 电机接口：使用电机开发板的 **PM2** 接口
- PWM 输出：TIM8_CH1，自动重装载值 ARR=1000，占空比 0~100% 对应转速 0~全速
- 方向控制：`BrushMotor_SetDirection()` 通过切换 PWM / 互补 PWMN 通道实现正反转
- 使能控制：通过 IR2104 的 SD 使能引脚（PM2_SD）开关电机
- 转速变量 duty 范围 -100~100：正数正转、负数反转、0 停止

---

## 🎮 按键操作

| 按键 | 功能 | 说明 |
| ---- | ---- | ---- |
| KEY0 | duty += 10（正转加速） | 上限 +100（全速正转），翻转 LED0 |
| KEY1 | duty -= 10（反转加速） | 下限 -100（全速反转），翻转 LED1 |
| KEY2 | duty = 0（停止） | 电机停止，翻转 LED0/LED1 |

---

## 📂 主要目录结构

```text
chap02_brush_motor_1/
├── Core/                 # 核心文件
│   └── Src/
│       └── main.c        # 主程序（按键扫描 + 电机调速/方向控制）
├── Drivers/              # 驱动层
│   └── BSP/              # 板级支持包
│       ├── Inc/          # BSP头文件
│       │   ├── brush_motor.h  # 直流有刷电机驱动（PWM/方向/使能）
│       │   └── key_led.h      # 按键/LED驱动
│       └── Src/          # BSP源文件
│           ├── brush_motor.c  # 电机控制驱动实现
│           └── key_led.c      # 按键/LED实现
└── brush_motor_1.ioc     # STM32CubeMX配置文件
```

- 关键文件说明
  - brush_motor.c/h - 电机控制驱动（PWM 调速、正反转、IR2104 使能）
  - key_led.c/h - 按键扫描与 LED 控制
  - main.c - 主程序逻辑（按键改变 duty，驱动电机调速与换向）

**⚠️ 注意：本仓库未包含 CubeMX 自动生成的官方代码（gpio.c/tim.c 等）与完整的 Keil MDK 工程文件，需自行通过 CubeMX 生成。**

要编译运行案例，请参考以下步骤。

---

## 📌 使用步骤

1. 打开 CubeMX 工程
   1. 在 STM32CubeMX 中打开 brush_motor_1.ioc 文件
   2. 确认定时器配置（TIM8_CH1 PWM 输出、PM2_SD 使能 GPIO）与工具链（Keil MDK-ARM）
2. 生成 Keil 工程
   1. 点击 GENERATE CODE
   2. CubeMX 将生成一个 Keil MDK 工程目录
3. 复制源代码
   1. 将本文件夹中的 main.c 及 Drivers/BSP 下的 .c/.h 文件复制或覆盖到生成的 Keil 工程目录
   2. 若提示覆盖，请选择覆盖已有文件
4. 添加头文件路径
   1. 打开 Keil 工程
   2. 进入 Options for Target → C/C++ → Include Paths
   3. 添加 BSP 头文件所在路径，避免编译错误
5. 编译与下载
   1. 在 Keil 中点击 Build 进行编译
   2. 使用下载器烧录至开发板

---

## 📎 注意事项

- 请确保 CubeMX 与 Keil MDK 版本兼容
- 电机、驱动板、开发板供电电压需匹配，避免过压/欠压损坏硬件
- 若电机不转，优先检查 PM2 接口接线、IR2104 使能引脚（PM2_SD）以及 PWM 引脚配置
- 若正反转方向与预期相反，可交换电机两根引线，或调整 `BrushMotor_SetDirection()` 中 PWM/PWMN 通道
- 若遇到编译报错，优先检查 Include Path 是否设置完整

## 💡 提示

- 配套视频教程见上方 B站链接
- 硬件问题：请前往 [GenBotter 淘宝店铺](https://genbotter.taobao.com) 咨询客服
- 代码/编译问题：可在仓库 Issue 区提交反馈
