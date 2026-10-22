# chap12_brush_motor_13 - 直流有刷电机梯形速度规划的位置环控制

> 使用 **STM32CubeMX + Keil MDK + VSCode** 开发  
> 适配 **GenBotter Motor-1 开发板、GenBotter 直流有刷电机驱动板** 与 **MC520 直流减速电机（带编码器）**

---

## 📌 项目简介

在 [chap12_brush_motor_12](../chap12_brush_motor_12/) 单环位置PID基础上，加入**梯形速度轨迹规划器**——每次定位不再把目标直接扔给PID，而是生成一条平滑的梯形速度曲线（加速→匀速→减速），将中间目标逐周期喂给PID。阶跃变台阶，从根源消除超调。

**核心架构：轨迹规划 → 位置PID**

```
 Key ──→ 轨迹规划器(TrajPlanner) ──→ 中间目标(每周期) ──→ 位置PID ──→ LPF ──→ PWM ──→ 电机
                                           ↑
                                     编码器位置反馈
```

> 💡 **教学定位**：与 _12 对比，演示"同一条PID，目标给法不同，效果天差地别"。_12 阶跃 → 8% 超调 + 振荡；_13 梯形轨迹 → 平滑到位，零明显超调。

---

## ⚙️ 硬件清单

与 chap12_brush_motor_12 完全相同，无任何硬件改动。

---

## 🔌 关键接口连接

| PM1 Pin | 信号名   | MCU GPIO        | 功能说明              |
|---------|----------|-----------------|----------------------|
| 9       | PWM_UH   | PA8 (TIM1_CH1)  | 半桥A PWM（正转时输出） |
| 10      | PWM_UL   | PB13 (TIM1_CH1N)| 半桥B PWM（反转时输出） |
| 11      | ENCA     | PC6 (TIM3_CH1)  | 编码器A相             |
| 12      | ENCB     | PC7 (TIM3_CH2)  | 编码器B相             |
| 22      | CTRL_SD  | PF10            | IR2104 关断           |

---

## 📐 编码器与位置参数

与 _12 完全相同：

| 参数 | 值 | 说明 |
|------|-----|------|
| ENCODER_PPR | **44** | 4倍频后计数值 |
| 物理线数 | 11 PPR | 霍尔编码器每通道脉冲数 |
| GEAR_RATIO | **30.0** | 减速比 |
| 输出轴一圈 | **1320 counts** | = 44 × 30 |
| 每度 | ≈ 3.667 counts | 位置分辨率 ≈ 0.27° |
| KEY0/KEY1 步进 | **±1320 counts** | 一整圈 |

---

## ⏱️ 软件定时调度（TIM6 1ms 基频）

```
TIM6 1ms ISR:
  └─ tick++
       ├─ 1ms:   无操作
       ├─ 50ms:  轨迹规划(Traj_Update) → 位置PID → LPF → PWM  ★新增
       ├─ 100ms: 编码器速度计算（差分法）
       └─ 200ms: LCD刷新 + VOFA+串口上报
```

| 调度项 | 周期 | 用途 |
|--------|:---:|------|
| 轨迹 + PID | **50ms** | 先推轨迹目标，再跑PID，两步串行 |
| 速度计算 | 100ms | 串口监视和调试，不进控制回路 |
| LCD + 串口 | 200ms | 显示和上报 |

---

## 🆕 新增模块：TrajPlanner（轨迹规划器）

### 梯形速度曲线原理

```
速度 v(t)
  ↑
  │     ╱‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾ ╲
  │    ╱                    ╲
  │   ╱                      ╲
  │  ╱                        ╲
  └──────────────────────────────→ t
     加速段   匀速段   减速段
```

- **Phase 1 加速**：v += accel × dt，直到 v ≥ v_max 或需提前减速
- **Phase 2 匀速**：v = v_max，持续监控剩余距离
- **Phase 3 减速**：v -= accel × dt，直到 v ≤ 0
- **关键公式**：减速距离 = v²/(2a)，保证正好到终点速度归零

### 关键参数

定义在 `Drivers/Control/Inc/user_config.h`：

| 参数 | 值 | 说明 |
|------|-----|------|
| TRAJ_V_MAX | 1000 counts/s | 最大速度（约 45 RPM） |
| TRAJ_ACCEL | 1000 counts/s² | 加减速度 |

### 公开接口

```c
void     TrajPlanner_Init(TrajPlanner *tp, float v_max, float accel);
void     TrajPlanner_Start(TrajPlanner *tp, int32_t from, int32_t to);
int32_t  TrajPlanner_Update(TrajPlanner *tp, float dt);
bool     TrajPlanner_IsDone(TrajPlanner *tp);
float    TrajPlanner_GetProgress(TrajPlanner *tp);
int32_t  TrajPlanner_GetSetpoint(TrajPlanner *tp);
float    TrajPlanner_GetSpeed(TrajPlanner *tp);
const char* TrajPlanner_PhaseName(TrajPhase_t phase);
```

### 工程要点

1. **轨迹启动时序**：TrajPlanner_Start 不在按键处理中调用，而是推迟到 TIM6 ISR 的下一个 PID 周期，使用那一刻的实时编码器值作为起点——消除按键时刻与 ISR 时刻之间的编码器漂移导致的反转。
2. **PID 反馈预填**：PID_Reset 后 feedback_prev 清零，但编码器非零 → 首帧 D 项产生负尖峰。手动预填为当前编码器值消除。
3. **停机策略**：轨迹 DONE 后，① 进入死区立即 IDLE + 禁能；② 超时 750ms 强制 IDLE + 禁能。防止死区微震啸叫。

---

## 📁 工程结构

```text
chap12_brush_motor_13/
├── .vscode/               # VSCode 工作区配置
├── Core/
│   ├── Inc/               # HAL 外设头文件
│   └── Src/               # main.c + 中断服务
├── Drivers/
│   ├── BSP/               # 板级驱动（编码器、电机、LCD、按键）
│   └── Control/           # 控制算法层
│       ├── Inc/           # pid.h, position_loop.h, user_config.h
│       │                   # ★ traj_planner.h（新增）
│       └── Src/           # pid.c, position_loop.c
│                           # ★ traj_planner.c（新增）
├── MDK-ARM/               # Keil MDK 工程目录
├── brush_motor_13.ioc     # CubeMX 配置文件
└── readme.md
```

> **_12 → _13 代码变动**：新增 traj_planner.c/h，修改 main.c（ISR + LCD + 按键）、position_loop.c（SetTarget 去 PID_Reset）、user_config.h（加轨迹参数，PID 参数沿用 _12）。pid.c/h、bsp_*、HAL 层**一行不动**。

---

## 🎮 按键操作

| 按键 | 功能 |
|------|------|
| KEY0 | 目标位置 +1 圈（基于上一条轨迹终点累加） |
| KEY1 | 目标位置 -1 圈 |
| KEY2 | 复位到原点 |

---

## 📊 运行效果

- 上电 LCD 显示「Chap12-03: Traj PID」
- 按 KEY0/KEY1 启动梯形轨迹 → 电机平滑加速→匀速→减速→到位
- 串口（115200, 8-N-1）输出 VOFA+ FireWater 格式数据（5 通道：目标/实际/误差/PWM/轨迹中间目标）
- 到位后自动停机，无啸叫
- 与 _12 对比：阶跃超调 vs 轨迹平滑

---

## ⚠️ 注意事项

- PID 参数（Kp=0.25, Ki=0.01, Kd=0.25）与 _12 完全相同——本章刻意沿用同一套 PID，仅改目标给法（梯形轨迹 vs 阶跃）
- 轨迹更新 (Traj_Update) 和 PID 计算在同一 50ms 周期内串行执行
- 短行程（< v_max² / accel）时梯形退化为三角形
- MIN_START = 0（轨迹模式下 PID 自由输出小微 PWM，无需 bang-bang 启动）
- DEAD_ZONE = 6（到位死区 ±1.6°）
- main 循环为非阻塞设计：所有任务由 TIM6 ISR 调度，while(1) 仅处理按键扫描
- PID 反馈预填是**单次初始化行为**，不在每个控制周期执行

---

## 🔙 对比上一章

| | chap12_brush_motor_12 | chap12_brush_motor_13 |
|------|------|------|
| 目标给法 | 阶跃（0→1320 瞬间跳变） | 梯形曲线（加速→匀速→减速） |
| 超调 | ~8%（111 counts） | 无明显超调 |
| 到位时间 | ~0.5s（不可控） | ~2.6s（可控，由 v_max/accel 决定） |
| PID 参数 | Kp=0.25, Ki=0.01, Kd=0.25 | Kp=0.25, Ki=0.01, Kd=0.25（同 _12） |
| 新增模块 | — | traj_planner.c/h |
| 啸叫 | 无（IDLE 静默） | 轨迹完成后自动停机静默 |

---

## 📄 许可证

本项目仅供学习与非商业用途。

---

<https://genbotter.taobao.com>
Dr. GAO — 2026年7月
