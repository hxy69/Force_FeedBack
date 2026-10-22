"""
电机控制仿真主程序 - 简化版
启动 GUI 并运行 PID 调参仿真
"""
import matplotlib
matplotlib.use('TkAgg')  # 明确指定后端
matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'FangSong']
matplotlib.rcParams['axes.unicode_minus'] = False
matplotlib.rcParams['figure.dpi'] = 100

import tkinter as tk
from tkinter import ttk
import sys
import os

# 添加模块路径
sys.path.append(os.path.join(os.path.dirname(__file__), 'models'))
sys.path.append(os.path.join(os.path.dirname(__file__), 'controllers'))
sys.path.append(os.path.join(os.path.dirname(__file__), 'simulation'))
sys.path.append(os.path.join(os.path.dirname(__file__), 'gui'))

from models.dc_motor import DCMotor
from controllers.pid_controller import PIDController
from simulation.simulator import SimulationEngine
from gui.pid_tuner_gui import PIDTunerGUI


def setup_styles():
    """设置界面样式"""
    style = ttk.Style()
    try:
        style.theme_use('clam')
    except:
        try:
            style.theme_use('alt')
        except:
            pass
    
    style.configure('Accent.TButton', 
                   foreground='white', 
                   background='#3498db',
                   focuscolor='none')


def create_system_components():
    """创建系统组件"""
    # 电机模型
    motor = DCMotor(
        Kt=0.15,     # 转矩常数
        J=0.008,     # 转动惯量  
        B=0.002,     # 阻尼系数
        tau=0.03,    # 时间常数
        R=1.2,       # 电枢电阻
        L=0.015,     # 电枢电感
        dt=0.001     # 仿真步长
    )

    # PID 控制器
    controller = PIDController(
        Kp=1.0,      # 比例增益
        Ki=0.5,      # 积分增益
        Kd=0.05,     # 微分增益
        dt=0.001,    # 控制周期
        output_limits=(-12, 12),
        windup_guard=25.0
    )

    # 仿真引擎
    simulator = SimulationEngine(
        motor=motor,
        controller=controller,
        dt=0.001,
        max_time=10.0,
        buffer_size=20000
    )
    
    return motor, controller, simulator


def main():
    """主函数"""
    print("启动 PID 控制仿真调参平台...")
    print("=" * 50)
    print("系统配置:")
    print("- 电机模型: 一阶直流有刷电机")
    print("- 控制器: 增强版 PID 控制器") 
    print("- 仿真引擎: 实时数据采集与性能分析")
    print("- 界面: 多参数实时调参与可视化")
    print("=" * 50)
    
    # 创建主窗口
    root = tk.Tk()
    root.title("PID 控制仿真调参平台")
    root.geometry("1400x800")
    
    # 设置样式
    setup_styles()
    
    # 创建系统组件
    motor, controller, simulator = create_system_components()
    
    # 创建 GUI 应用
    app = PIDTunerGUI(root, simulator)
    
    # 启动主循环
    try:
        root.mainloop()
    except Exception as e:
        print(f"应用程序错误: {e}")


if __name__ == "__main__":
    main()