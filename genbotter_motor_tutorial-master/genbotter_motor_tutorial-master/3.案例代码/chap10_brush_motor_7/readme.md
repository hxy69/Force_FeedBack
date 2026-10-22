# 直流有刷电机 PID 速度环控制（代码工程化优化）

在“brush_motor_6”的实现了速度环的pid控制的基础上，对pid控制的代码进行了工程优化。

通过ADC1的IN8获取电流模拟信号，通过ADC1的IN9获取电压模拟信号，通过ADC1的IN0获取温度传感器电压变化的模拟信号，进行模数转换实现电流与电压的同步检测。

基于GenBotter Motor-1电机开发板（STM32F407IGT6主控），配套GenBotter直流有刷电机驱动板和带AB相霍尔编码器的直流有刷电机，实现直流有刷电机PID速度环控制的项目。

- 🎬 B站主页：[高博士_嵌入式](https://space.bilibili.com/486637340)  
- 🛒 淘宝店铺：[GenBotter](https://genbotter.taobao.com)

---

## 🎬 视频教程

👉 [电机开发教程 - B站视频](https://www.bilibili.com/video/BV18S6FBnEtS)（直流有刷电机PID速度环控制实现）

---

## 🛒 硬件要求

- GenBotter Motor-1电机开发板
- 直流有刷电机（带AB相霍尔编码器）
- GenBotter直流有刷电机驱动板
- 4.0寸FSMC并口LCD显示屏（用于显示电流、电压、转速等测量结果）
- DapLink下载器（或ST-Link V2.1下载器）

---

## 📌 使用步骤

1. 打开 CubeMX 工程
   1. 在 STM32CubeMX 中打开 brush_motor_7.ioc 文件
   2. 确认 ADC 配置（电流检测通道 ADC1_IN8、电压检测通道 ADC1_IN9 和温度检测通道 ADC1_IN0 的引脚、采样率等参数）
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
