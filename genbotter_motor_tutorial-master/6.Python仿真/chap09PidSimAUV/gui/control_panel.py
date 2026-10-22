import tkinter as tk
from tkinter import ttk
import webbrowser

class ControlPanel:
    def __init__(self, parent, sim, max_height, canvas_view=None):
        self.sim = sim
        self.canvas_view = canvas_view
        self.frame = ttk.Frame(parent, padding=10)
        
        # 定义PID参数的固定默认值
        self.DEFAULT_KP = 2.0  # 比例增益默认值
        self.DEFAULT_KI = 0.5  # 积分增益默认值
        self.DEFAULT_KD = 0.8  # 微分增益默认值
        
        # 存储滑块标签的引用，方便更新
        self.slider_labels = {}

        # ---------------- PID ----------------
        ttk.Label(
            self.frame,
            text="PID Parameters",
            font=("Arial", 10, "bold")
        ).pack(pady=5)

        # 使用默认值初始化变量
        self.kp = tk.DoubleVar(value=self.DEFAULT_KP)
        self.ki = tk.DoubleVar(value=self.DEFAULT_KI)
        self.kd = tk.DoubleVar(value=self.DEFAULT_KD)

        # 确保仿真器的PID参数也使用默认值
        self.sim.pid.kp = self.DEFAULT_KP
        self.sim.pid.ki = self.DEFAULT_KI
        self.sim.pid.kd = self.DEFAULT_KD

        self._slider("Kp", self.kp, 0.0, 5.0)
        self._slider("Ki", self.ki, 0.0, 5.0)
        self._slider("Kd", self.kd, 0.0, 5.0)

        # ---------------- Target ----------------
        ttk.Label(
            self.frame,
            text="Target Altitude (m)",
            font=("Arial", 10, "bold")
        ).pack(pady=10)

        self.target = tk.DoubleVar(value=sim.target_height)
        self._slider("Target", self.target, 0.0, max_height, is_target=True)

        # ---------------- UAV Response ----------------
        ttk.Label(
            self.frame,
            text="UAV Response Speed",
            font=("Arial", 10, "bold")
        ).pack(pady=10)

        self.response = tk.DoubleVar(value=sim.uav.response_gain)
        self._slider("Response", self.response, 0.2, 5.0, is_response=True)

        # ---------------- Buttons ----------------
        ttk.Button(
            self.frame,
            text="Reset PID",
            command=self._reset_pid
        ).pack(fill=tk.X, pady=5)

        ttk.Button(
            self.frame,
            text="Reset UAV (To Ground)",
            command=self._reset_uav
        ).pack(fill=tk.X)

        # ---------------- 淘宝店铺链接 ----------------
        self._add_taobao_link()

    # ------------------------------------------------

    def _add_taobao_link(self):
        """添加淘宝店铺链接按钮"""
        def open_taobao():
            webbrowser.open("https://genbotter.taobao.com")
        
        # 添加分隔线
        ttk.Separator(self.frame, orient='horizontal').pack(fill=tk.X, pady=10)
        
        # 容器frame
        link_frame = ttk.Frame(self.frame)
        link_frame.pack(fill=tk.X, pady=(5, 0))
        
        # 淘宝按钮
        taobao_btn = tk.Button(
            link_frame,
            text="🛒 访问 GenBotter 淘宝店铺",
            command=open_taobao,
            fg="white",
            bg="#FF5000",
            font=("Microsoft YaHei", 9),
            padx=15,
            pady=4,
            relief="flat",
            cursor="hand2",
            bd=0
        )
        taobao_btn.pack()
        
        # 悬停效果
        taobao_btn.bind("<Enter>", lambda e: taobao_btn.config(bg="#FF7A00"))
        taobao_btn.bind("<Leave>", lambda e: taobao_btn.config(bg="#FF5000"))

    def _slider(self, name, var, min_v, max_v, is_target=False, is_response=False):
        """创建滑块并存储标签引用"""
        # 创建标签
        label = ttk.Label(self.frame)
        label.pack(anchor="w")
        
        # 存储标签引用，方便后续更新
        self.slider_labels[name] = label

        def update(*_):
            """更新标签显示"""
            label.config(text=f"{name}: {var.get():.2f}")

            if is_target:
                self.sim.target_height = var.get()
            elif is_response:
                self.sim.uav.response_gain = var.get()
            else:
                self._update_pid()

        # 初始化显示
        update()

        # 创建滑块
        ttk.Scale(
            self.frame,
            from_=min_v,
            to=max_v,
            orient=tk.HORIZONTAL,
            variable=var,
            command=update
        ).pack(fill=tk.X)

    def _update_pid(self):
        """将GUI的值更新到仿真器的PID控制器"""
        self.sim.pid.kp = self.kp.get()
        self.sim.pid.ki = self.ki.get()
        self.sim.pid.kd = self.kd.get()

    def _update_slider_labels(self):
        """手动更新所有滑块标签的显示"""
        # 更新PID滑块标签
        if "Kp" in self.slider_labels:
            self.slider_labels["Kp"].config(text=f"Kp: {self.kp.get():.2f}")
        if "Ki" in self.slider_labels:
            self.slider_labels["Ki"].config(text=f"Ki: {self.ki.get():.2f}")
        if "Kd" in self.slider_labels:
            self.slider_labels["Kd"].config(text=f"Kd: {self.kd.get():.2f}")
        
        # 更新其他滑块标签
        if "Target" in self.slider_labels:
            self.slider_labels["Target"].config(text=f"Target: {self.target.get():.2f}")
        if "Response" in self.slider_labels:
            self.slider_labels["Response"].config(text=f"Response: {self.response.get():.2f}")

    def _reset_pid(self):
        """重置PID参数到固定默认值"""
        # 1. 将GUI变量设置为默认值
        self.kp.set(self.DEFAULT_KP)
        self.ki.set(self.DEFAULT_KI)
        self.kd.set(self.DEFAULT_KD)
        
        # 2. 手动更新滑块标签的显示
        self._update_slider_labels()
        
        # 3. 更新仿真器的PID参数
        self.sim.pid.kp = self.DEFAULT_KP
        self.sim.pid.ki = self.DEFAULT_KI
        self.sim.pid.kd = self.DEFAULT_KD
        
        # 4. 可选：如果需要，重置PID控制器的积分项
        if hasattr(self.sim.pid, 'reset'):
            self.sim.pid.reset()
        
    def _reset_uav(self):
        """重置无人机到地面"""
        self.sim.reset(to_ground=True)
        # 重置高度历史曲线
        if self.canvas_view:
            self.canvas_view.reset_plot()