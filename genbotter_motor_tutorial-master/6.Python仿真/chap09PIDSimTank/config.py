# -*- coding: utf-8 -*-
"""
配置文件 - 优化用于教学演示
"""

import platform
import matplotlib.pyplot as plt

# 设置 matplotlib 支持中文显示
def setup_matplotlib_fonts():
    """配置 matplotlib 中文字体"""
    if platform.system() == 'Windows':
        plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei']
    elif platform.system() == 'Darwin':
        plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC']
    else:
        plt.rcParams['font.sans-serif'] = ['WenQuanYi Micro Hei']
    
    plt.rcParams['axes.unicode_minus'] = False

setup_matplotlib_fonts()

# 水池参数 - 优化用于教学演示
TANK_CONFIG = {
    'height': 2.0,           # 水池高度 (米)
    'area': 1.0,             # 减小底面积，响应更快
    'max_water_level': 1.8,  # 最大安全水位
    'initial_level': 0.0,    # 初始水位为0
    'fixed_outflow_rate': 0.2,  # 固定出水流速 (m³/s)
    'max_outflow_rate': 0.5,
    'max_inflow_rate': 2.0,
    'target_level': 1.0,     # 目标水位
    'time_step': 0.1,        # 时间步长
    'inertia': 0.4           # 惯性系数
}

# PID预设参数 - 优化用于清晰演示
PID_PRESETS = {
    'P': {
        'name': 'P控制',
        'description': '比例控制 - 存在稳态误差',
        'Kp': 1.5,    # 增大，产生明显稳态误差
        'Ki': 0.0,
        'Kd': 0.0
    },
    'PI': {
        'name': 'PI控制',
        'description': '比例积分控制 - 消除稳态误差',
        'Kp': 1.5,    # 适当减小，避免过冲
        'Ki': 0.15,   # 适中，清晰消除稳态误差
        'Kd': 0.0
    },
    'PID': {
        'name': 'PID控制',
        'description': '比例积分微分控制 - 兼顾响应与稳定',
        'Kp': 1.5,    # 适中
        'Ki': 0.15,   # 减小，避免积分饱和
        'Kd': 0.5     # 增大，明显抑制超调
    }
}

# 仿真参数
SIMULATION_CONFIG = {
    'time_step': 0.1,
    'max_time': 60.0,
    'default_speed': 1.0,
}