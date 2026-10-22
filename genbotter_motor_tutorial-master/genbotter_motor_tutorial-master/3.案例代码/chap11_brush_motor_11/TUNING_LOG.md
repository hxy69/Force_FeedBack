### 电流环最终参数 (2026-06-30)

**架构**：位置式 PI + 1000Hz ISR 闭环
- PID 在 ADC 注入中断内运行，零延迟：过采样满 → 滤波 → PID → PWM
- TIM1 16kHz 中心对齐，TRGO Update 触发 ADC
- 16× 过采样 → 1000Hz 电流更新 = PID 频率
- α=0.05 IIR 一阶低通，兼顾噪声抑制与响应

**最终参数(user_config.h)**：
- Kp=0.4, Ki=0.08, Kd=0
- OUTPUT_MAX=90, OUTPUT_MIN=-90
- INTEGRAL_LIMIT=1500
- CURRENT_OVERSAMPLE_COUNT=16
- CURRENT_LOWPASS_ALPHA=0.05
- ADC_TO_MA_FACTOR=6.714 (3.3V/4096/0.12V/A×1000)

**效果**：稳态误差小、响应速度快、正反转可控。
**调参备忘**：Kp从0.25→0.4加速响应；Ki经历0.04→0.12→0.05→0.08；关键bug是OUTPUT_MIN=0截断负输出。
