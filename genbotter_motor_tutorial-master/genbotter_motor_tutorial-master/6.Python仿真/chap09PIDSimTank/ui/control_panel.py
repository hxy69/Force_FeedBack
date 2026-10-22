# -*- coding: utf-8 -*-
"""
控制面板部件 - 优化默认参数
"""

import tkinter as tk
from tkinter import ttk
import config

class ControlPanel(tk.Frame):
    """控制面板部件"""
    
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)
        
        # PID参数 - 使用配置中的默认值
        self.current_mode = 'P'
        preset = config.PID_PRESETS[self.current_mode]
        self.Kp = preset['Kp']
        self.Ki = preset['Ki']
        self.Kd = preset['Kd']
        
        # 回调函数字典
        self.callbacks = {}
        
        # 初始化UI
        self._init_ui()
        self._apply_styles()
    
    def _init_ui(self):
        """初始化UI"""
        self.configure(bg='#f5f5f5')
        
        # 创建滚动区域
        canvas = tk.Canvas(self, bg='#f5f5f5', highlightthickness=0)
        scrollbar = ttk.Scrollbar(self, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg='#f5f5f5')
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas_window = canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # 标题
        title_label = tk.Label(scrollable_frame, text="PID控制面板", 
                              font=("Microsoft YaHei", 14, "bold"),
                              bg='#f5f5f5', fg='#2c3e50')
        title_label.pack(pady=(10, 5))
        
        # 控制模式选择
        self._create_mode_group(scrollable_frame)
        
        # PID参数设置
        self._create_pid_group(scrollable_frame)
        
        # 水池参数设置
        self._create_tank_group(scrollable_frame)
        
        # 仿真控制
        self._create_simulation_group(scrollable_frame)
        
        # 打包滚动区域
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
    
    def _apply_styles(self):
        """应用样式"""
        style = ttk.Style()
        style.configure('TButton', padding=6)
        style.configure('TCheckbutton', background='#f5f5f5')
        style.configure('TRadiobutton', background='#f5f5f5')
    
    def _create_mode_group(self, parent):
        """创建控制模式选择组"""
        mode_frame = tk.LabelFrame(parent, text="控制模式选择", 
                                   font=("Microsoft YaHei", 10, "bold"),
                                   bg='#f5f5f5', fg='#2c3e50',
                                   padx=10, pady=10)
        mode_frame.pack(fill='x', padx=10, pady=5, ipadx=5, ipady=5)
        
        # 模式变量
        self.mode_var = tk.StringVar(value='P')
        
        # P控制按钮
        p_radio = tk.Radiobutton(mode_frame, text="P控制 - 比例控制", 
                                variable=self.mode_var, value='P',
                                command=lambda: self._on_mode_changed('P'),
                                font=("Microsoft YaHei", 9),
                                bg='#f5f5f5', fg='#2c3e50',
                                activebackground='#f5f5f5')
        p_radio.pack(anchor='w', pady=2)
        
        # PI控制按钮
        pi_radio = tk.Radiobutton(mode_frame, text="PI控制 - 比例积分控制", 
                                 variable=self.mode_var, value='PI',
                                 command=lambda: self._on_mode_changed('PI'),
                                 font=("Microsoft YaHei", 9),
                                 bg='#f5f5f5', fg='#2c3e50',
                                 activebackground='#f5f5f5')
        pi_radio.pack(anchor='w', pady=2)
        
        # PID控制按钮
        pid_radio = tk.Radiobutton(mode_frame, text="PID控制 - 比例积分微分控制", 
                                  variable=self.mode_var, value='PID',
                                  command=lambda: self._on_mode_changed('PID'),
                                  font=("Microsoft YaHei", 9),
                                  bg='#f5f5f5', fg='#2c3e50',
                                  activebackground='#f5f5f5')
        pid_radio.pack(anchor='w', pady=2)
        
        # 模式描述标签
        self.mode_desc = tk.Label(mode_frame, 
                                  text=config.PID_PRESETS['P']['description'],
                                  font=("Microsoft YaHei", 9),
                                  bg='#f5f5f5', fg='#666666',
                                  wraplength=280, justify='left')
        self.mode_desc.pack(anchor='w', pady=(5, 0))
    
    def _create_pid_group(self, parent):
        """创建PID参数设置组"""
        pid_frame = tk.LabelFrame(parent, text="PID参数设置", 
                                  font=("Microsoft YaHei", 10, "bold"),
                                  bg='#f5f5f5', fg='#2c3e50',
                                  padx=10, pady=10)
        pid_frame.pack(fill='x', padx=10, pady=5, ipadx=5, ipady=5)
        
        # Kp参数
        kp_frame = tk.Frame(pid_frame, bg='#f5f5f5')
        kp_frame.pack(fill='x', pady=5)
        
        tk.Label(kp_frame, text="比例系数 Kp:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # Kp滑块
        self.kp_var = tk.DoubleVar(value=self.Kp)
        self.kp_slider = tk.Scale(kp_frame, from_=0.0, to=20.0,
                                 resolution=0.01, orient='horizontal',
                                 variable=self.kp_var, length=140,
                                 command=lambda v: self._on_kp_changed(float(v)),
                                 bg='#f5f5f5', fg='#2c3e50',
                                 highlightbackground='#f5f5f5',
                                 showvalue=0)
        self.kp_slider.pack(side='left', padx=(5, 10))
        
        # Kp数值框
        self.kp_spinbox = tk.Spinbox(kp_frame, from_=0.0, to=20.0, 
                                     increment=0.01, width=8,
                                     textvariable=self.kp_var,
                                     command=lambda: self._on_kp_spinbox_changed(),
                                     font=("Consolas", 9))
        self.kp_spinbox.pack(side='left')
        
        # Ki参数
        ki_frame = tk.Frame(pid_frame, bg='#f5f5f5')
        ki_frame.pack(fill='x', pady=5)
        
        tk.Label(ki_frame, text="积分系数 Ki:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # Ki滑块
        self.ki_var = tk.DoubleVar(value=self.Ki)
        self.ki_slider = tk.Scale(ki_frame, from_=0.0, to=2.0,
                                 resolution=0.01, orient='horizontal',
                                 variable=self.ki_var, length=140,
                                 command=lambda v: self._on_ki_changed(float(v)),
                                 bg='#f5f5f5', fg='#2c3e50',
                                 highlightbackground='#f5f5f5',
                                 showvalue=0,
                                 state='disabled')
        self.ki_slider.pack(side='left', padx=(5, 10))
        
        # Ki数值框
        self.ki_spinbox = tk.Spinbox(ki_frame, from_=0.0, to=2.0, 
                                     increment=0.01, width=8,
                                     textvariable=self.ki_var,
                                     command=lambda: self._on_ki_spinbox_changed(),
                                     font=("Consolas", 9),
                                     state='disabled')
        self.ki_spinbox.pack(side='left')
        
        # Kd参数
        kd_frame = tk.Frame(pid_frame, bg='#f5f5f5')
        kd_frame.pack(fill='x', pady=5)
        
        tk.Label(kd_frame, text="微分系数 Kd:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # Kd滑块
        self.kd_var = tk.DoubleVar(value=self.Kd)
        self.kd_slider = tk.Scale(kd_frame, from_=0.0, to=5.0,  # 扩大范围，便于调节
                                 resolution=0.01, orient='horizontal',
                                 variable=self.kd_var, length=140,
                                 command=lambda v: self._on_kd_changed(float(v)),
                                 bg='#f5f5f5', fg='#2c3e50',
                                 highlightbackground='#f5f5f5',
                                 showvalue=0,
                                 state='disabled')
        self.kd_slider.pack(side='left', padx=(5, 10))
        
        # Kd数值框
        self.kd_spinbox = tk.Spinbox(kd_frame, from_=0.0, to=5.0, 
                                     increment=0.01, width=8,
                                     textvariable=self.kd_var,
                                     command=lambda: self._on_kd_spinbox_changed(),
                                     font=("Consolas", 9),
                                     state='disabled')
        self.kd_spinbox.pack(side='left')
        
        # 恢复默认值按钮
        reset_button = ttk.Button(pid_frame, text="恢复默认值",
                                 command=self._reset_to_defaults)
        reset_button.pack(pady=(10, 0))
    
    def _create_tank_group(self, parent):
        """创建水池参数设置组"""
        tank_frame = tk.LabelFrame(parent, text="水池参数", 
                                   font=("Microsoft YaHei", 10, "bold"),
                                   bg='#f5f5f5', fg='#2c3e50',
                                   padx=10, pady=10)
        tank_frame.pack(fill='x', padx=10, pady=5, ipadx=5, ipady=5)
        
        # 出水流速设置
        outflow_frame = tk.Frame(tank_frame, bg='#f5f5f5')
        outflow_frame.pack(fill='x', pady=5)
        
        tk.Label(outflow_frame, text="出水流速:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # 出水流速滑块
        self.outflow_var = tk.DoubleVar(value=0.2)
        self.outflow_slider = tk.Scale(outflow_frame, from_=0.0, to=0.5, 
                                      resolution=0.001, orient='horizontal',
                                      variable=self.outflow_var, length=140,
                                      command=lambda v: self._on_outflow_changed(float(v)),
                                      bg='#f5f5f5', fg='#2c3e50',
                                      highlightbackground='#f5f5f5',
                                      showvalue=0)
        self.outflow_slider.pack(side='left', padx=(5, 10))
        
        # 出水流速数值框
        self.outflow_spinbox = tk.Spinbox(outflow_frame, from_=0.0, to=0.5, 
                                         increment=0.001, width=8,
                                         textvariable=self.outflow_var,
                                         command=lambda: self._on_outflow_spinbox_changed(),
                                         font=("Consolas", 9))
        self.outflow_spinbox.pack(side='left')
        
        # 目标水位设置
        target_frame = tk.Frame(tank_frame, bg='#f5f5f5')
        target_frame.pack(fill='x', pady=5)
        
        tk.Label(target_frame, text="目标水位:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # 目标水位数值框
        self.target_var = tk.DoubleVar(value=1.0)
        self.target_spinbox = tk.Spinbox(target_frame, from_=0.1, to=1.8, 
                                        increment=0.1, width=8,
                                        textvariable=self.target_var,
                                        command=lambda: self._on_target_changed(),
                                        font=("Consolas", 9))
        self.target_spinbox.pack(side='left')
    
    def _create_simulation_group(self, parent):
        """创建仿真控制组"""
        sim_frame = tk.LabelFrame(parent, text="仿真控制", 
                                  font=("Microsoft YaHei", 10, "bold"),
                                  bg='#f5f5f5', fg='#2c3e50',
                                  padx=10, pady=10)
        sim_frame.pack(fill='x', padx=10, pady=5, ipadx=5, ipady=5)
        
        # 仿真速度选择
        speed_frame = tk.Frame(sim_frame, bg='#f5f5f5')
        speed_frame.pack(fill='x', pady=5)
        
        tk.Label(speed_frame, text="仿真速度:", 
                font=("Microsoft YaHei", 9),
                bg='#f5f5f5', fg='#2c3e50',
                width=12, anchor='w').pack(side='left')
        
        # 速度选择下拉框
        self.speed_var = tk.StringVar(value="1.0x (正常)")
        self.speed_combo = ttk.Combobox(speed_frame, 
                                       values=["0.1x (极慢)", "0.5x (慢速)", 
                                              "1.0x (正常)", "2.0x (快速)", 
                                              "5.0x (高速)", "10.0x (极速)"],
                                       textvariable=self.speed_var,
                                       state="readonly", width=15)
        self.speed_combo.bind('<<ComboboxSelected>>', self._on_speed_changed)
        self.speed_combo.pack(side='left')
        
        # 控制按钮
        button_frame = tk.Frame(sim_frame, bg='#f5f5f5')
        button_frame.pack(pady=10)
        
        self.start_button = ttk.Button(button_frame, text="▶ 开始",
                                      command=self._on_start_clicked,
                                      width=8)
        self.start_button.pack(side='left', padx=2)
        
        self.pause_button = ttk.Button(button_frame, text="⏸ 暂停",
                                      command=self._on_pause_clicked,
                                      state='disabled', width=8)
        self.pause_button.pack(side='left', padx=2)
        
        self.reset_button = ttk.Button(button_frame, text="↺ 重置",
                                      command=self._on_reset_clicked,
                                      width=8)
        self.reset_button.pack(side='left', padx=2)
        
        # 状态显示
        self.status_label = tk.Label(sim_frame, text="状态: 准备就绪",
                                     font=("Microsoft YaHei", 10, "bold"),
                                     bg='#f5f5f5', fg='#666666')
        self.status_label.pack(pady=5)
        
        # 性能指标显示
        self.performance_label = tk.Label(sim_frame, text="性能指标: 未开始",
                                         font=("Microsoft YaHei", 9),
                                         bg='#f5f5f5', fg='#666666',
                                         wraplength=280, justify='left')
        self.performance_label.pack(pady=5)
    
    def _on_mode_changed(self, mode):
        """控制模式改变"""
        self.current_mode = mode
        preset = config.PID_PRESETS[mode]
        
        # 更新模式描述
        self.mode_desc.config(text=preset['description'])
        
        # 更新UI控件状态
        if mode == 'P':
            self.ki_slider.config(state='disabled')
            self.ki_spinbox.config(state='disabled')
            self.kd_slider.config(state='disabled')
            self.kd_spinbox.config(state='disabled')
        elif mode == 'PI':
            self.ki_slider.config(state='normal')
            self.ki_spinbox.config(state='normal')
            self.kd_slider.config(state='disabled')
            self.kd_spinbox.config(state='disabled')
        else:  # PID
            self.ki_slider.config(state='normal')
            self.ki_spinbox.config(state='normal')
            self.kd_slider.config(state='normal')
            self.kd_spinbox.config(state='normal')
        
        # 调用回调
        if 'mode_changed' in self.callbacks:
            self.callbacks['mode_changed'](mode)
        
        # 恢复默认值
        self._reset_to_defaults()
    
    def _on_kp_changed(self, value):
        """Kp滑块改变"""
        self.Kp = value
        self.kp_var.set(value)
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_kp_spinbox_changed(self):
        """Kp数值框改变"""
        self.Kp = self.kp_var.get()
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_ki_changed(self, value):
        """Ki滑块改变"""
        self.Ki = value
        self.ki_var.set(value)
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_ki_spinbox_changed(self):
        """Ki数值框改变"""
        self.Ki = self.ki_var.get()
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_kd_changed(self, value):
        """Kd滑块改变"""
        self.Kd = value
        self.kd_var.set(value)
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_kd_spinbox_changed(self):
        """Kd数值框改变"""
        self.Kd = self.kd_var.get()
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def _on_outflow_changed(self, value):
        """出水流速滑块改变"""
        self.outflow_var.set(value)
        if 'outflow_changed' in self.callbacks:
            self.callbacks['outflow_changed'](value)
    
    def _on_outflow_spinbox_changed(self):
        """出水流速数值框改变"""
        value = self.outflow_var.get()
        if 'outflow_changed' in self.callbacks:
            self.callbacks['outflow_changed'](value)
    
    def _on_target_changed(self):
        """目标水位改变"""
        value = self.target_var.get()
        if 'target_changed' in self.callbacks:
            self.callbacks['target_changed'](value)
    
    def _on_speed_changed(self, event=None):
        """仿真速度改变"""
        speed_text = self.speed_var.get()
        speed_map = {
            "0.1x (极慢)": 0.1,
            "0.5x (慢速)": 0.5,
            "1.0x (正常)": 1.0,
            "2.0x (快速)": 2.0,
            "5.0x (高速)": 5.0,
            "10.0x (极速)": 10.0
        }
        
        if speed_text in speed_map:
            if 'simulation_speed_changed' in self.callbacks:
                self.callbacks['simulation_speed_changed'](speed_map[speed_text])
    
    def _on_start_clicked(self):
        """开始按钮点击"""
        self.start_button.config(state='disabled')
        self.pause_button.config(state='normal')
        self.status_label.config(text="状态: 运行中", fg='#4CAF50')
        
        if 'start_simulation' in self.callbacks:
            self.callbacks['start_simulation']()
    
    def _on_pause_clicked(self):
        """暂停按钮点击"""
        self.start_button.config(state='normal')
        self.pause_button.config(state='disabled')
        self.status_label.config(text="状态: 已暂停", fg='#FF9800')
        
        if 'pause_simulation' in self.callbacks:
            self.callbacks['pause_simulation']()
    
    def _on_reset_clicked(self):
        """重置按钮点击"""
        self.start_button.config(state='normal')
        self.pause_button.config(state='disabled')
        self.status_label.config(text="状态: 已重置", fg='#f44336')
        
        if 'reset_simulation' in self.callbacks:
            self.callbacks['reset_simulation']()
    
    def _reset_to_defaults(self):
        """恢复默认值"""
        preset = config.PID_PRESETS[self.current_mode]
        self.Kp = preset['Kp']
        self.Ki = preset['Ki']
        self.Kd = preset['Kd']
        
        # 更新UI
        self.kp_var.set(self.Kp)
        self.ki_var.set(self.Ki)
        self.kd_var.set(self.Kd)
        
        # 调用回调
        if 'pid_changed' in self.callbacks:
            self.callbacks['pid_changed'](self.Kp, self.Ki, self.Kd)
    
    def update_status(self, is_running, is_paused):
        """更新仿真状态"""
        if is_running:
            if is_paused:
                self.start_button.config(state='normal')
                self.pause_button.config(state='disabled')
                self.status_label.config(text="状态: 已暂停", fg='#FF9800')
            else:
                self.start_button.config(state='disabled')
                self.pause_button.config(state='normal')
                self.status_label.config(text="状态: 运行中", fg='#4CAF50')
        else:
            self.start_button.config(state='normal')
            self.pause_button.config(state='disabled')
            self.status_label.config(text="状态: 准备就绪", fg='#666666')
    
    def update_performance(self, performance_stats):
        """更新性能指标显示"""
        if performance_stats:
            text = "性能指标: "
            stats_list = []
            
            if performance_stats.get('rise_time') is not None:
                stats_list.append(f"上升时间: {performance_stats['rise_time']:.2f}s")
            
            if performance_stats.get('overshoot') is not None:
                stats_list.append(f"超调量: {performance_stats['overshoot']:.1f}%")
            
            if performance_stats.get('steady_state_error') is not None:
                stats_list.append(f"稳态误差: {performance_stats['steady_state_error']:.3f}m")
            
            if stats_list:
                text += " | ".join(stats_list)
            else:
                text += "未计算"
            
            self.performance_label.config(text=text)
    
    def set_callback(self, name, func):
        """设置回调函数"""
        self.callbacks[name] = func