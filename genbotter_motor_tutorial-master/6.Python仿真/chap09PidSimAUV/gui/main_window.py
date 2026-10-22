# gui/main_window.py - 修复版

import tkinter as tk
from tkinter import ttk

# 使用您原有的类，不是增强版
from controllers.pid import PID
from models.uav import UAV
from simulations.hover_sim import HoverSimulation

from gui.canvas_view import CanvasView
from gui.control_panel import ControlPanel


class MainWindow:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("PID Altitude Control Simulation")

        # 设置图标（如果存在）
        try:
            icon = tk.PhotoImage(file="gui/assets/genbotter.png")
            self.root.iconphoto(True, icon)
        except:
            pass  # 如果图标不存在，继续运行

        # 配置网格权重
        self.root.rowconfigure(0, weight=1)
        self.root.columnconfigure(0, weight=3)
        self.root.columnconfigure(1, weight=1)

        # 使用您原有的参数创建仿真实例
        pid = PID(2.0, 0.5, 0.8)
        uav = UAV()

        self.sim = HoverSimulation(pid, uav)

        # 创建CanvasView（现在它会返回一个frame）
        self.canvas_view = CanvasView(self.root, self.sim.max_height)
        
        # 使用grid布局CanvasView的frame
        self.canvas_view.get_frame().grid(row=0, column=0, sticky="nsew", padx=5, pady=5)

        # 创建控制面板
        self.control_panel = ControlPanel(self.root, self.sim, self.sim.max_height, self.canvas_view)
        self.control_panel.frame.grid(row=0, column=1, sticky="nsew", padx=(0, 5), pady=5)

        # 启动仿真循环
        self._loop()

    def _loop(self):
        dt = 0.02
        self.sim.step(dt)

        self.canvas_view.update_view(
            self.sim.uav.y,
            self.sim.target_height
        )

        self.root.after(int(dt * 1000), self._loop)

    def run(self):
        self.root.mainloop()
        