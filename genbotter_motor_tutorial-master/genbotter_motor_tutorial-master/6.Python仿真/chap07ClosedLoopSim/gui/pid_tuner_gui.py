import matplotlib
matplotlib.use('TkAgg')
matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'FangSong']
matplotlib.rcParams['axes.unicode_minus'] = False
matplotlib.rcParams['figure.dpi'] = 100

import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
import webbrowser


class PIDTunerGUI:
    """优化版 PID 调参界面 - 左右布局，保留所有功能"""

    def __init__(self, root, simulator):
        self.root = root
        self.sim = simulator
        self.running = False
        self.setpoint = tk.DoubleVar(value=50.0)
        self.dt_plot = 100  # 绘图更新间隔 (ms)
        self.data_window = 5000  # 显示数据点数
        self.after_id = None  # 存储 after 调用的 ID

        # PID 参数
        self.kp = tk.DoubleVar(value=1.0)
        self.ki = tk.DoubleVar(value=0.5)
        self.kd = tk.DoubleVar(value=0.05)

        # 性能指标
        self.performance_data = {
            'rise_time': tk.StringVar(value="--"),
            'overshoot': tk.StringVar(value="--"),
            'settling_time': tk.StringVar(value="--"),
            'steady_error': tk.StringVar(value="--"),
            'iae': tk.StringVar(value="--"),
            'real_time_factor': tk.StringVar(value="--")
        }

        self._build_ui()
        self._enhance_ui_features()
        
        # 注册关闭事件处理
        self.root.protocol("WM_DELETE_WINDOW", self._on_closing)
        
        # 窗口默认最大化
        self.root.state('zoomed')  # Windows系统
        # 对于Linux系统可以使用：self.root.attributes('-zoomed', True)

    def _build_ui(self):
        """构建用户界面 - 左右布局"""
        self.root.title("PID 控制仿真调参平台 - GenBotter")
        
        # 设置合适的初始窗口大小
        self.root.geometry("1200x700")
        self.root.minsize(1000, 600)
        
        # 使用PanedWindow实现可调整的左右分割
        main_pane = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_pane.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 左侧控制面板 - 固定宽度
        left_frame = ttk.Frame(main_pane, width=320)
        main_pane.add(left_frame, weight=0)
        left_frame.pack_propagate(False)  # 防止框架收缩
        
        # 右侧绘图区域
        right_frame = ttk.Frame(main_pane)
        main_pane.add(right_frame, weight=1)
        
        # 构建左侧控制面板内容
        self._build_left_panel(left_frame)
        
        # 构建右侧绘图区域 - 2x2布局
        self._build_right_panel(right_frame)
        
        # 底部状态栏和链接
        self._build_bottom_bar()

    def _build_left_panel(self, parent):
        """构建左侧控制面板"""
        # 创建可滚动的框架
        canvas = tk.Canvas(parent)
        scrollbar = ttk.Scrollbar(parent, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        # PID 参数调节区域
        pid_frame = ttk.LabelFrame(scrollable_frame, text="PID 参数调节", padding=10)
        pid_frame.pack(fill=tk.X, pady=(0, 10))

        # PID 参数配置
        params_config = [
            ("Kp (比例)", self.kp, (0.1, 10.0), "控制响应速度"),
            ("Ki (积分)", self.ki, (0.01, 20.0), "消除稳态误差"),
            ("Kd (微分)", self.kd, (0.001, 5.0), "抑制超调振荡")
        ]

        for label, var, range_, tooltip in params_config:
            frame = ttk.Frame(pid_frame)
            frame.pack(fill=tk.X, pady=5)
            
            # 标签
            ttk.Label(frame, text=label, width=8, anchor=tk.W).pack(side=tk.LEFT)
            
            # 滑块
            scale = ttk.Scale(frame, from_=range_[0], to=range_[1], 
                            orient=tk.HORIZONTAL, variable=var, length=150)
            scale.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 10))
            
            # 输入框
            entry = ttk.Entry(frame, textvariable=var, width=6, 
                             font=("Consolas", 9), justify=tk.CENTER)
            entry.pack(side=tk.RIGHT)
            
            # 输入验证和工具提示
            self._setup_entry_validation(entry, var, range_)
            self._create_tooltip(scale, tooltip)
            self._create_tooltip(entry, f"输入范围: {range_[0]} - {range_[1]}")

        # 设定值输入
        setpoint_frame = ttk.Frame(pid_frame)
        setpoint_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Label(setpoint_frame, text="目标转速:", width=8).pack(side=tk.LEFT)
        setpoint_entry = ttk.Entry(setpoint_frame, textvariable=self.setpoint, width=8, 
                                 font=("Consolas", 10), justify=tk.CENTER)
        setpoint_entry.pack(side=tk.LEFT, padx=5)
        ttk.Label(setpoint_frame, text="rad/s").pack(side=tk.LEFT)
        self._setup_entry_validation(setpoint_entry, self.setpoint, (0, 200))

        # 控制按钮
        btn_frame = ttk.Frame(pid_frame)
        btn_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(btn_frame, text="开始仿真", command=self.start,
                  width=10).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="暂停", command=self.stop,
                  width=8).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="重置", command=self.reset,
                  width=8).pack(side=tk.LEFT, padx=2)

        # 系统状态显示
        status_frame = ttk.LabelFrame(scrollable_frame, text="系统实时状态", padding=10)
        status_frame.pack(fill=tk.X, pady=(0, 10))

        # 状态变量
        self.status_vars = {
            'speed': tk.StringVar(value="0.00 rad/s"),
            'voltage': tk.StringVar(value="0.00 V"),
            'current': tk.StringVar(value="0.00 A"),
            'error': tk.StringVar(value="0.00 rad/s"),
            'torque': tk.StringVar(value="0.00 N·m")
        }

        status_labels = [
            ("当前转速:", "speed"),
            ("控制电压:", "voltage"), 
            ("电枢电流:", "current"),
            ("跟踪误差:", "error"),
            ("输出转矩:", "torque")
        ]

        for text, key in status_labels:
            frame = ttk.Frame(status_frame)
            frame.pack(fill=tk.X, pady=3)
            ttk.Label(frame, text=text, width=10, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(frame, textvariable=self.status_vars[key], 
                     font=("Consolas", 9), foreground="#2c3e50").pack(side=tk.RIGHT)

        # 性能指标显示
        perf_frame = ttk.LabelFrame(scrollable_frame, text="控制系统性能指标", padding=10)
        perf_frame.pack(fill=tk.X, pady=(0, 10))

        perf_labels = [
            ("上升时间:", "rise_time", "s"),
            ("超调量:", "overshoot", "%"),
            ("调节时间:", "settling_time", "s"),
            ("稳态误差:", "steady_error", "rad/s"),
            ("IAE指标:", "iae", ""),
            ("实时因子:", "real_time_factor", "x")
        ]

        for text, key, unit in perf_labels:
            frame = ttk.Frame(perf_frame)
            frame.pack(fill=tk.X, pady=2)
            ttk.Label(frame, text=text, width=10, anchor=tk.W).pack(side=tk.LEFT)
            value_label = ttk.Label(frame, textvariable=self.performance_data[key],
                                  font=("Consolas", 9), foreground="#e74c3c", width=8)
            value_label.pack(side=tk.LEFT)
            ttk.Label(frame, text=unit, font=("宋体", 8), foreground="#7f8c8d").pack(side=tk.LEFT)
            
        # 店铺链接
        shop_frame = ttk.Frame(scrollable_frame)
        shop_frame.pack(fill=tk.X, pady=10)
        
        taobao_label = ttk.Label(shop_frame, text="🛒 访问GenBotter淘宝店铺", 
                                foreground="#e74c3c", cursor="hand2", 
                                font=("宋体", 10, "underline"))
        taobao_label.pack(side=tk.LEFT)
        taobao_label.bind("<Button-1>", lambda e: self._open_taobao())
        
        copyright_label = ttk.Label(shop_frame, 
                                   text="© GenBotter 智能控制实验平台", 
                                   foreground="#3498db", font=("宋体", 9))
        copyright_label.pack(side=tk.RIGHT)

    def _build_right_panel(self, parent):
        """构建右侧绘图区域 - 2x2布局"""
        # 创建2x2子图
        self.fig, self.axs = plt.subplots(2, 2, figsize=(10, 7))
        self.fig.set_facecolor('#f8f9fa')
        
        # 配置画布
        self.canvas = FigureCanvasTkAgg(self.fig, master=parent)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 添加工具栏
        from matplotlib.backends.backend_tkagg import NavigationToolbar2Tk
        toolbar = NavigationToolbar2Tk(self.canvas, parent)
        toolbar.update()
        
        # 初始化图形设置
        self._init_plots()

    def _init_plots(self):
        """初始化图形设置"""
        titles = ['电机转速响应', '控制电压输出', '电枢电流变化', '系统跟踪误差']
        ylabels = ['转速 (rad/s)', '电压 (V)', '电流 (A)', '误差 (rad/s)']
        colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12']
        
        # 将2x2的axs数组展平以便迭代
        axs_flat = self.axs.flatten()
        
        for i, (ax, title, ylabel, color) in enumerate(zip(axs_flat, titles, ylabels, colors)):
            ax.set_title(title, fontsize=11, pad=8)
            ax.set_ylabel(ylabel, fontsize=9)
            ax.grid(True, alpha=0.3, linestyle='--')
            ax.set_facecolor('#fdfdfd')
            
            # 设置边框颜色
            for spine in ax.spines.values():
                spine.set_color('#bdc3c7')
            
            # 只在底部子图设置x轴标签
            if i >= 2:
                ax.set_xlabel('时间 (s)', fontsize=9)
        
        self.fig.tight_layout(pad=3.0)
        self.canvas.draw()

    def _build_bottom_bar(self):
        """构建底部状态栏"""
        # 底部状态栏
        bottom_frame = ttk.Frame(self.root)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=5)
        
        # 状态提示
        self.status_label = ttk.Label(bottom_frame, text="就绪", 
                                     foreground="#27ae60", font=("宋体", 9))
        self.status_label.pack(side=tk.LEFT)

    def _setup_entry_validation(self, entry, var, valid_range):
        """设置输入框验证，确保输入值在有效范围内"""
        def validate_input(new_value):
            if new_value == "":
                return True
            try:
                value = float(new_value)
                min_val, max_val = valid_range
                return min_val <= value <= max_val
            except ValueError:
                return False
        
        def on_focusout(event):
            try:
                current_value = float(var.get())
                min_val, max_val = valid_range
                if current_value < min_val:
                    var.set(min_val)
                elif current_value > max_val:
                    var.set(max_val)
            except (ValueError, tk.TclError):
                pass
        
        def on_return(event):
            on_focusout(event)
            entry.select_range(0, tk.END)
        
        vcmd = (entry.register(validate_input), '%P')
        entry.configure(validate='key', validatecommand=vcmd)
        entry.bind('<FocusOut>', on_focusout)
        entry.bind('<Return>', on_return)

    def _create_tooltip(self, widget, text):
        """创建工具提示"""
        def on_enter(event):
            tooltip = tk.Toplevel()
            tooltip.wm_overrideredirect(True)
            tooltip.wm_geometry(f"+{event.x_root+10}+{event.y_root+10}")
            label = ttk.Label(tooltip, text=text, background="#ffffe0", 
                            relief='solid', borderwidth=1, padding=5)
            label.pack()
            widget.tooltip = tooltip

        def on_leave(event):
            if hasattr(widget, 'tooltip'):
                widget.tooltip.destroy()

        widget.bind("<Enter>", on_enter)
        widget.bind("<Leave>", on_leave)

    def _open_taobao(self):
        """打开淘宝店铺"""
        webbrowser.open("https://genbotter.taobao.com")
        self._show_temp_message("正在打开淘宝店铺...", "#e67e22")

    def _show_temp_message(self, message, color="#27ae60"):
        """显示临时消息"""
        self.status_label.config(text=message, foreground=color)
        self.root.after(3000, lambda: self.status_label.config(text="就绪", foreground="#27ae60"))

    def _enhance_ui_features(self):
        """增强UI功能"""
        # 添加快捷键
        self.root.bind('<Control-r>', lambda e: self.reset())
        self.root.bind('<Control-s>', lambda e: self.start())
        self.root.bind('<Control-p>', lambda e: self.stop())

    def _on_closing(self):
        """程序关闭时的清理工作"""
        print("正在关闭 PID 调参平台...")
        self.running = False
        if self.after_id:
            self.root.after_cancel(self.after_id)
            self.after_id = None
        self.root.quit()
        self.root.destroy()

    def start(self):
        """开始仿真"""
        if not self.running:
            self.running = True
            self.status_label.config(text="仿真运行中...", foreground="#3498db")
            self._run_simulation()

    def stop(self):
        """暂停仿真"""
        self.running = False
        self.status_label.config(text="仿真已暂停", foreground="#e67e22")
        if self.after_id:
            self.root.after_cancel(self.after_id)
            self.after_id = None

    def reset(self):
        """重置仿真"""
        self.stop()
        self.sim.reset()
        self.status_label.config(text="系统已重置", foreground="#27ae60")
        self.root.after(3000, lambda: self.status_label.config(text="就绪", foreground="#27ae60"))
        
        # 重置图形
        for ax in self.axs.flatten():
            ax.cla()
        self._init_plots()
        
        # 重置状态显示
        for var in self.status_vars.values():
            var.set("0.00")
        
        # 重置性能指标
        for var in self.performance_data.values():
            var.set("--")

    def _run_simulation(self):
        """运行仿真循环"""
        if not self.running:
            return

        try:
            # 更新控制器参数
            self.sim.controller.Kp = self.kp.get()
            self.sim.controller.Ki = self.ki.get()
            self.sim.controller.Kd = self.kd.get()

            setpoint = self.setpoint.get()

            # 执行仿真步骤
            for _ in range(5):
                omega, u = self.sim.step(setpoint)

            # 获取当前数据
            data = self.sim.get_current_data(self.data_window)

            # 更新状态显示
            self._update_status_display(data, setpoint, u)

            # 更新图形
            self._update_plots(data, setpoint)

            # 更新性能指标
            self._update_performance_metrics(setpoint)

            # 继续下一次更新
            if self.running:
                self.after_id = self.root.after(self.dt_plot, self._run_simulation)

        except Exception as e:
            print(f"仿真错误: {e}")
            self.running = False
            self.status_label.config(text=f"仿真错误: {e}", foreground="#e74c3c")
            if self.after_id:
                self.root.after_cancel(self.after_id)
                self.after_id = None

    def _update_status_display(self, data, setpoint, u):
        """更新状态显示"""
        if len(data['omega']) > 0:
            current_speed = data['omega'][-1]
            current_error = setpoint - current_speed
            
            self.status_vars['speed'].set(f"{current_speed:.2f} rad/s")
            self.status_vars['voltage'].set(f"{u:.2f} V")
            self.status_vars['error'].set(f"{current_error:.2f} rad/s")
            
            # 获取电机和控制器状态
            system_info = self.sim.get_system_info()
            motor_state = system_info.get('motor_state', {})
            
            self.status_vars['current'].set(f"{motor_state.get('current', 0):.2f} A")
            self.status_vars['torque'].set(f"{motor_state.get('torque', 0):.3f} N·m")

    def _update_plots(self, data, setpoint):
        """更新图形显示"""
        if len(data['t']) < 2:
            return

        t = data['t']
        
        # 清除并重新绘制
        for ax in self.axs.flatten():
            ax.cla()
        
        # 重新初始化图形设置
        titles = ['电机转速响应', '控制电压输出', '电枢电流变化', '系统跟踪误差']
        ylabels = ['转速 (rad/s)', '电压 (V)', '电流 (A)', '误差 (rad/s)']
        colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12']
        
        axs_flat = self.axs.flatten()
        for i, (ax, title, ylabel, color) in enumerate(zip(axs_flat, titles, ylabels, colors)):
            ax.set_title(title, fontsize=11, pad=8)
            ax.set_ylabel(ylabel, fontsize=9)
            ax.grid(True, alpha=0.3, linestyle='--')
            ax.set_facecolor('#fdfdfd')
            
            for spine in ax.spines.values():
                spine.set_color('#bdc3c7')
            
            if i >= 2:
                ax.set_xlabel('时间 (s)', fontsize=9)

        # 绘制数据
        self.axs[0, 0].plot(t, data['omega'], label='实际转速', color=colors[0], linewidth=1.5)
        self.axs[0, 0].axhline(setpoint, color='red', linestyle='--', alpha=0.7, linewidth=1, label='目标转速')
        self.axs[0, 0].legend(fontsize=8, loc='upper right')
        
        self.axs[0, 1].plot(t, data['u'], color=colors[1], linewidth=1.5)
        self.axs[0, 1].axhline(12, color='red', linestyle=':', alpha=0.5, linewidth=0.8, label='电压限幅')
        self.axs[0, 1].axhline(-12, color='red', linestyle=':', alpha=0.5, linewidth=0.8)
        self.axs[0, 1].legend(fontsize=8, loc='upper right')
        
        if 'current' in data:
            self.axs[1, 0].plot(t, data['current'], color=colors[2], linewidth=1.5)
        
        self.axs[1, 1].plot(t, data['error'], color=colors[3], linewidth=1.5)
        self.axs[1, 1].axhline(0, color='red', linestyle='--', alpha=0.5, linewidth=0.8)

        # 自动调整坐标轴范围
        for ax in self.axs.flatten():
            ax.relim()
            ax.autoscale_view()
        
        self.fig.tight_layout(pad=3.0)
        self.canvas.draw_idle()

    def _update_performance_metrics(self, setpoint):
        """更新性能指标显示"""
        metrics = self.sim.get_performance_metrics(setpoint)
        if not metrics:
            return

        # 更新性能指标显示
        self.performance_data['rise_time'].set(f"{metrics.get('rise_time', float('nan')):.3f}")
        self.performance_data['overshoot'].set(f"{metrics.get('overshoot_percent', 0):.1f}")
        self.performance_data['settling_time'].set(f"{metrics.get('settling_time', float('nan')):.3f}")
        self.performance_data['steady_error'].set(f"{metrics.get('steady_state_error', 0):.3f}")
        self.performance_data['iae'].set(f"{metrics.get('iae', 0):.3f}")
        self.performance_data['real_time_factor'].set(f"{metrics.get('real_time_factor', 1):.2f}")