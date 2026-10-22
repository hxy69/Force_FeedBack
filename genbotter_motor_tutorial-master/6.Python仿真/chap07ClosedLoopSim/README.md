# PID控制仿真（Python）

本目录提供一个**模块化、可扩展的电机控制算法仿真框架**，基于 Python 实现，支持可视化调参与动态响应观察，适用于教学演示、算法验证与控制策略快速原型开发。

## 🎯 支持功能

- ✅ **直流有刷电机（DC Motor）一阶/二阶动态模型**
- ✅ **PID 速度/位置控制仿真**
- ✅ **实时参数调节（Kp, Ki, Kd, 目标值）**
- ✅ **图形化界面（Tkinter + Matplotlib）**

## 📂 目录结构

```bash
6.Motor_Ctrl_Sim_Py
├── main.py # 程序入口
├── models/ # 电机模型（DC, PMSM 等）
├── controllers/ # 控制器实现（PID, FOC...）
├── simulation/ # 仿真引擎与调度逻辑
├── gui/ # 可视化界面
├── utils/ # 工具函数（限幅、滤波等）
├── requirements.txt # Python 依赖
└── README.md
```

## 🚀 快速开始

### 安装依赖

```bash
pip install -r requirements.txt
```

### 运行仿真

```bash
python main.py
```

### 在 GUI 中调节 PID 参数，观察电机转速响应曲线

## 📦 依赖说明

- Python ≥ 3.12
- numpy：数值计算
- matplotlib：实时绘图
- tkinter：GUI（Python 标准库，通常已内置）

## 📝 使用场景

- 电机控制课程教学演示
- PID 参数整定练习
- 新控制算法（如自适应 PID、滑模）的快速验证
- 与硬件实验形成“仿真 → 实现”闭环

## 🤝 贡献与反馈

- 欢迎提交 Issue 或 PR！
- 配套硬件与视频教程请访问：
  - [GenBotter 淘宝店铺](https://genbotter.taobao.com)
  - [B站教程](https://www.bilibili.com/video/BV122JEzaE2n/)
