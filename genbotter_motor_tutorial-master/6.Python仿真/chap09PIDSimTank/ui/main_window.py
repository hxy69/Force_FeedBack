# -*- coding: utf-8 -*-
"""
主窗口 - 优化用于教学演示
"""

import tkinter as tk
from tkinter import ttk, messagebox
import sys
import os
import threading
import time

# 导入自定义部件
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from ui.control_panel import ControlPanel
from ui.tank_widget import TankWidget
from ui.plot_widget import PlotWidget

# 导入仿真模块
from simulation.tank import WaterTank
from simulation.pid import PIDController
from simulation.simulator import SimulationEngine

# 导入配置
import config

class MainWindow(tk.Tk):
    """主窗口"""
    
    def __init__(self):
        super().__init__()
        
        # 设置窗口属性
        self.title("PID水箱教学仿真系统 - 高博士课件演示")
        self.geometry("1400x800")
        self.minsize(1200, 700)
        
        # 设置窗口图标
        self._set_window_icon()
        
        # 初始化仿真组件
        self._init_simulation()
        
        # 初始化UI
        self._init_ui()
        
        # 连接信号与槽
        self._connect_signals()
        
        # 更新定时器
        self._setup_update_timer()
        
        # 当前仿真数据
        self.current_simulation_data = {
            'level': 0.0,
            'target': 1.0,
            'error': 1.0,
            'control': 0.0,
            'inflow': 0.0,
            'outflow': 0.2,
            'P': 0.0,
            'I': 0.0,
            'D': 0.0,
            'time': 0.0,
            'max_time': 120.0,
            'max_time_reached': False
        }
    
    def _set_window_icon(self):
        """设置窗口图标"""
        try:
            icon_path = os.path.join(os.path.dirname(__file__), "assets", "genbotter.png")
            if os.path.exists(icon_path):
                icon = tk.PhotoImage(file=icon_path)
                self.iconphoto(True, icon)
        except Exception as e:
            print(f"无法加载窗口图标: {e}")
    
    def _init_simulation(self):
        """初始化仿真组件"""
        # 使用配置中的参数
        self.tank = WaterTank(config.TANK_CONFIG)
        
        # 创建PID控制器 - 使用P控制的预设参数
        pid_preset = config.PID_PRESETS['P']
        self.pid = PIDController(
            Kp=pid_preset['Kp'],
            Ki=pid_preset['Ki'],
            Kd=pid_preset['Kd'],
            mode='P'
        )
        
        # 创建仿真引擎
        self.simulator = SimulationEngine(self.tank, self.pid, 
                                         time_step=0.1, max_time=120.0)
        
        # 设置数据回调
        self.simulator.set_data_callback(self._on_simulation_data)
        self.simulator.set_status_callback(self._on_simulation_status)
        
        # 仿真状态
        self.is_simulation_running = False
        self.is_simulation_paused = False
    
    def _init_ui(self):
        """初始化UI"""
        self.configure(bg='#f5f5f5')
        
        # 创建主框架
        main_frame = tk.Frame(self, bg='#f5f5f5')
        main_frame.pack(fill='both', expand=True, padx=10, pady=10)
        
        # 创建三栏布局
        self._create_three_column_layout(main_frame)
        
        # 创建状态栏
        self._create_status_bar()
    
    def _create_three_column_layout(self, parent):
        """创建三栏布局"""
        self.paned_window = tk.PanedWindow(parent, orient='horizontal', 
                                          bg='#f5f5f5', sashwidth=10,
                                          sashrelief='raised',
                                          opaqueresize=False)
        self.paned_window.pack(fill='both', expand=True)
        
        # 左侧 - 控制面板
        left_frame = tk.Frame(self.paned_window, bg='#f5f5f5')
        self.control_panel = ControlPanel(left_frame)
        self.control_panel.pack(fill='both', expand=True, padx=5, pady=5)
        self.paned_window.add(left_frame, width=350, minsize=300)
        
        # 中间 - 水箱显示
        center_frame = tk.Frame(self.paned_window, bg='#f5f5f5')
        self.tank_widget = TankWidget(center_frame)
        self.tank_widget.pack(fill='both', expand=True, padx=5, pady=5)
        self.paned_window.add(center_frame, width=500, minsize=400)
        
        # 绑定尺寸变化事件
        self.tank_widget.canvas.bind('<Configure>', lambda e: self.tank_widget.resize())
        
        # 右侧 - 跟踪曲线
        right_frame = tk.Frame(self.paned_window, bg='#f5f5f5')
        self.plot_widget = PlotWidget(right_frame)
        self.plot_widget.pack(fill='both', expand=True, padx=5, pady=5)
        self.paned_window.add(right_frame, width=600, minsize=500)
    
    def _create_status_bar(self):
        """创建状态栏"""
        self.status_bar = tk.Frame(self, height=25, bg='#e0e0e0', 
                                  relief='sunken', borderwidth=1)
        self.status_bar.pack(side='bottom', fill='x')
        
        # 状态标签
        self.status_label = tk.Label(self.status_bar, 
                                    text="PID水箱教学仿真系统已就绪 - 高博士课件演示",
                                    bg='#e0e0e0', fg='#2c3e50',
                                    font=("Microsoft YaHei", 9))
        self.status_label.pack(side='left', padx=10)
        
        # 仿真时间显示
        self.time_label = tk.Label(self.status_bar, text="时间: 0.0s / 120.0s",
                                  bg='#e0e0e0', fg='#666666',
                                  font=("Consolas", 9))
        self.time_label.pack(side='right', padx=10)
        
        # 水位显示
        self.level_label = tk.Label(self.status_bar, text="水位: 0.000m",
                                   bg='#e0e0e0', fg='#666666',
                                   font=("Consolas", 9))
        self.level_label.pack(side='right', padx=10)
        
        # 误差显示
        self.error_label = tk.Label(self.status_bar, text="误差: +1.000m",
                                   bg='#e0e0e0', fg='#666666',
                                   font=("Consolas", 9))
        self.error_label.pack(side='right', padx=10)
    
    def _connect_signals(self):
        """连接信号与槽"""
        # 控制面板回调
        self.control_panel.set_callback('pid_changed', self._on_pid_changed)
        self.control_panel.set_callback('mode_changed', self._on_mode_changed)
        self.control_panel.set_callback('outflow_changed', self._on_outflow_changed)
        self.control_panel.set_callback('simulation_speed_changed', self._on_speed_changed)
        self.control_panel.set_callback('start_simulation', self._on_start_simulation)
        self.control_panel.set_callback('pause_simulation', self._on_pause_simulation)
        self.control_panel.set_callback('reset_simulation', self._on_reset_simulation)
        self.control_panel.set_callback('target_changed', self._on_target_changed)
    
    def _setup_update_timer(self):
        """设置更新定时器"""
        def update_display():
            if self.is_simulation_paused:
                self.tank_widget.update_data(self.current_simulation_data)
            self.after(100, update_display)
        
        update_display()
    
    def _on_simulation_data(self, data):
        """处理仿真数据"""
        self.current_simulation_data = data.copy()
        self.after(0, self._update_ui_with_data, data)
    
    def _update_ui_with_data(self, data):
        """使用仿真数据更新UI"""
        self.tank_widget.update_data(data)
        self.plot_widget.update_data(data)
        
        time_str = f"时间: {data['time']:.1f}s / {data.get('max_time', 120.0):.1f}s"
        level_str = f"水位: {data['level']:.3f}m"
        error_str = f"误差: {data['error']:+.3f}m"
        
        self.time_label.config(text=time_str)
        self.level_label.config(text=level_str)
        self.error_label.config(text=error_str)
        
        if 'performance' in data:
            self.control_panel.update_performance(data['performance'])
        
        if data.get('max_time_reached', False):
            self.control_panel.update_status(False, False)
            self.status_label.config(text="仿真已完成 - 达到最大仿真时间")
    
    def _on_simulation_status(self, status):
        """处理仿真状态变化"""
        if status == 'started':
            self.is_simulation_running = True
            self.is_simulation_paused = False
            self.control_panel.update_status(True, False)
            self.status_label.config(text="仿真运行中")
        elif status == 'paused':
            self.is_simulation_paused = True
            self.control_panel.update_status(True, True)
            self.status_label.config(text="仿真已暂停")
        elif status == 'resumed':
            self.is_simulation_paused = False
            self.control_panel.update_status(True, False)
            self.status_label.config(text="仿真运行中")
        elif status == 'stopped':
            self.is_simulation_running = False
            self.is_simulation_paused = False
            self.control_panel.update_status(False, False)
            self.status_label.config(text="仿真已停止")
        elif status == 'max_time_reached':
            self.is_simulation_running = False
            self.is_simulation_paused = False
            self.control_panel.update_status(False, False)
            self.status_label.config(text="仿真已完成 - 达到最大仿真时间")
            messagebox.showinfo("仿真完成", "仿真已达到最大时间（120秒），仿真已自动停止。")
        elif status == 'reset':
            self.is_simulation_running = False
            self.is_simulation_paused = False
            self.control_panel.update_status(False, False)
            self.status_label.config(text="仿真已重置")
            self.plot_widget.clear_data()
            self.tank_widget.reset()
        elif status.startswith('error:'):
            messagebox.showerror("仿真错误", f"仿真过程中发生错误:\n{status[6:]}")
    
    def _on_pid_changed(self, Kp, Ki, Kd):
        """PID参数改变"""
        self.pid.set_parameters(Kp=Kp, Ki=Ki, Kd=Kd)
    
    def _on_mode_changed(self, mode):
        """控制模式改变"""
        success = self.pid.set_mode(mode)
        if success and self.is_simulation_running:
            self.simulator.reset()
            self.is_simulation_running = False
            self.is_simulation_paused = False
            self.control_panel.update_status(False, False)
    
    def _on_outflow_changed(self, rate):
        """出水流速改变"""
        self.tank.set_outflow_rate(rate)
        self.current_simulation_data['outflow'] = rate
        if not self.is_simulation_running:
            self.tank_widget.update_data(self.current_simulation_data)
    
    def _on_target_changed(self, target):
        """目标水位改变"""
        self.tank.set_target_level(target)
        self.current_simulation_data['target'] = target
        self.current_simulation_data['error'] = target - self.current_simulation_data['level']
        if not self.is_simulation_running:
            self.tank_widget.update_data(self.current_simulation_data)
    
    def _on_speed_changed(self, factor):
        """仿真速度改变"""
        self.simulator.set_speed(factor)
    
    def _on_start_simulation(self):
        """开始仿真"""
        if not self.is_simulation_running:
            def run_simulation():
                self.simulator.start()
            
            sim_thread = threading.Thread(target=run_simulation, daemon=True)
            sim_thread.start()
    
    def _on_pause_simulation(self):
        """暂停仿真"""
        if self.is_simulation_running and not self.is_simulation_paused:
            self.simulator.pause()
    
    def _on_reset_simulation(self):
        """重置仿真"""
        if self.is_simulation_running:
            self.simulator.stop()
        
        self.simulator.reset()
        self.tank.reset(initial_level=0.0)
        self.pid.reset()
        
        # 重置当前数据
        self.current_simulation_data = {
            'level': 0.0,
            'target': self.tank.target_level,
            'error': self.tank.target_level,
            'control': 0.0,
            'inflow': 0.0,
            'outflow': self.tank.outflow_rate,
            'P': 0.0,
            'I': 0.0,
            'D': 0.0,
            'time': 0.0,
            'max_time': 120.0,
            'max_time_reached': False
        }
        
        self.tank_widget.update_data(self.current_simulation_data)
        self.time_label.config(text="时间: 0.0s / 120.0s")
        self.level_label.config(text=f"水位: 0.000m")
        self.error_label.config(text=f"误差: +{self.tank.target_level:.3f}m")
        self.plot_widget.clear_data()
        self.control_panel.update_status(False, False)
        self.status_label.config(text="PID水箱教学仿真系统已就绪 - 高博士课件演示")
    
    def on_closing(self):
        """窗口关闭事件"""
        if self.is_simulation_running:
            self.simulator.stop()
        
        if messagebox.askokcancel("退出", "确定要退出PID水箱教学仿真系统吗？"):
            self.destroy()
            sys.exit(0)