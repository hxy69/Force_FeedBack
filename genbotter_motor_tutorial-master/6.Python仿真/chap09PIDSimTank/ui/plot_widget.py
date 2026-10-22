# -*- coding: utf-8 -*-
"""
跟踪曲线绘制部件 - tkinter 版本
使用 matplotlib 嵌入 tkinter
"""

import tkinter as tk
from tkinter import ttk
import numpy as np

import matplotlib
matplotlib.use('TkAgg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk

# 导入配置以设置中文字体
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config  # 这会自动设置 matplotlib 字体

class PlotWidget(tk.Frame):
    """曲线绘制部件"""
    
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)
        
        # 数据存储
        self.time_data = []
        self.level_data = []
        self.target_data = []
        self.error_data = []
        self.control_data = []
        self.inflow_data = []
        self.outflow_data = []
        
        # 当前显示的曲线类型
        self.current_plot_type = "水位跟踪"
        
        # 最大显示点数 (避免内存过大)
        self.max_points = 2000
        
        # 颜色配置
        self.colors = {
            'level': '#4A90E2',      # 实际水位
            'target': '#FF4444',     # 目标水位
            'error': '#FF9500',      # 误差
            'control': '#34C759',    # 控制量
            'inflow': '#007AFF',     # 进水流速
            'outflow': '#FF3B30'     # 出水流速
        }
        
        # 初始化UI
        self._init_ui()
    
    def _init_ui(self):
        """初始化UI"""
        self.configure(bg='#ffffff')
        
        # 标题
        self.title_label = tk.Label(self, text="目标跟踪曲线", 
                                   font=("Microsoft YaHei", 16, "bold"),
                                   bg='#ffffff', fg='#2c3e50')
        self.title_label.pack(pady=10)
        
        # 控制工具栏
        self._create_toolbar()
        
        # 创建Matplotlib图形
        self.figure = Figure(figsize=(8, 5), dpi=100, facecolor='#f8f9fa')
        self.canvas = FigureCanvasTkAgg(self.figure, self)
        self.canvas.get_tk_widget().pack(fill='both', expand=True, padx=10, pady=10)
        
        # 创建子图
        self.ax = self.figure.add_subplot(111)
        
        # 初始化绘图
        self._init_plot()
    
    def _create_toolbar(self):
        """创建控制工具栏"""
        toolbar_frame = tk.Frame(self, bg='#ffffff')
        toolbar_frame.pack(fill='x', padx=10, pady=5)
        
        # 曲线选择
        tk.Label(toolbar_frame, text="显示曲线:", 
                font=("Microsoft YaHei", 9),
                bg='#ffffff', fg='#2c3e50').pack(side='left', padx=(0, 10))
        
        self.curve_var = tk.StringVar(value="水位跟踪")
        self.curve_combo = ttk.Combobox(toolbar_frame, 
                                       values=["水位跟踪", "误差曲线", "控制量曲线", "流量曲线", "全部曲线"],
                                       textvariable=self.curve_var,
                                       state="readonly", width=15)
        self.curve_combo.bind('<<ComboboxSelected>>', self._on_curve_changed)
        self.curve_combo.pack(side='left', padx=(0, 20))
        
        # 清空数据按钮
        self.clear_button = ttk.Button(toolbar_frame, text="清空数据",
                                      command=self.clear_data)
        self.clear_button.pack(side='left', padx=5)
        
        # 导出数据按钮
        self.export_button = ttk.Button(toolbar_frame, text="导出数据",
                                       command=self.export_data)
        self.export_button.pack(side='left', padx=5)
        
        # 添加伸缩项
        tk.Frame(toolbar_frame, bg='#ffffff').pack(side='left', expand=True, fill='x')
        
        # 数据点显示
        self.data_count_label = tk.Label(toolbar_frame, text="数据点: 0",
                                        font=("Microsoft YaHei", 9),
                                        bg='#ffffff', fg='#666666')
        self.data_count_label.pack(side='right', padx=10)
    
    def _init_plot(self):
        """初始化绘图"""
        self.ax.clear()
        
        # 设置坐标轴标签
        self.ax.set_xlabel('时间 (秒)', fontsize=10)
        self.ax.set_ylabel('水位 (米)', color=self.colors['level'], fontsize=10)
        
        # 设置网格
        self.ax.grid(True, alpha=0.3, linestyle='--')
        
        # 设置标题
        self.ax.set_title('水位跟踪曲线', fontsize=12, fontweight='bold', pad=15)
        
        # 设置背景色
        self.ax.set_facecolor('#ffffff')
        self.figure.patch.set_facecolor('#f8f9fa')
        
        # 设置坐标轴颜色
        self.ax.spines['bottom'].set_color('#cccccc')
        self.ax.spines['top'].set_color('#cccccc')
        self.ax.spines['left'].set_color('#cccccc')
        self.ax.spines['right'].set_color('#cccccc')
        
        # 移除空图例调用，避免警告
        # self.ax.legend(loc='upper right', fontsize=9, framealpha=0.9)
        
        self.canvas.draw()
    
    def _on_curve_changed(self, event=None):
        """曲线类型改变"""
        self.current_plot_type = self.curve_var.get()
        self._update_plot()
    
    def update_data(self, data):
        """更新数据"""
        time = data.get('time', 0)
        level = data.get('level', 0)
        target = data.get('target', 1.0)
        error = data.get('error', 0)
        control = data.get('control', 0)
        inflow = data.get('inflow', 0.0)
        outflow = data.get('outflow', 0.2)
        
        # 添加新数据
        self.time_data.append(time)
        self.level_data.append(level)
        self.target_data.append(target)
        self.error_data.append(error)
        self.control_data.append(control)
        self.inflow_data.append(inflow)
        self.outflow_data.append(outflow)
        
        # 限制数据长度
        if len(self.time_data) > self.max_points:
            self.time_data = self.time_data[-self.max_points:]
            self.level_data = self.level_data[-self.max_points:]
            self.target_data = self.target_data[-self.max_points:]
            self.error_data = self.error_data[-self.max_points:]
            self.control_data = self.control_data[-self.max_points:]
            self.inflow_data = self.inflow_data[-self.max_points:]
            self.outflow_data = self.outflow_data[-self.max_points:]
        
        # 更新数据点显示
        self.data_count_label.config(text=f"数据点: {len(self.time_data)}")
        
        # 更新绘图
        self._update_plot()
    
    def _update_plot(self):
        """更新绘图"""
        if not self.time_data:
            return
        
        self.ax.clear()
        
        plot_type = self.current_plot_type
        
        if plot_type == "水位跟踪" or plot_type == "全部曲线":
            # 绘制实际水位曲线
            self.ax.plot(self.time_data, self.level_data, 
                        color=self.colors['level'], linewidth=2, 
                        label='实际水位')
            
            # 绘制目标水位线
            self.ax.plot(self.time_data, self.target_data,
                        color=self.colors['target'], linestyle='--', linewidth=1.5,
                        label='目标水位')
            
            # 设置y轴标签
            self.ax.set_ylabel('水位 (米)', color=self.colors['level'], fontsize=10)
        
        if plot_type == "误差曲线" or plot_type == "全部曲线":
            # 绘制误差曲线
            if plot_type == "误差曲线":
                self.ax.plot(self.time_data, self.error_data,
                           color=self.colors['error'], linewidth=2,
                           label='误差')
                self.ax.set_ylabel('误差 (米)', color=self.colors['error'], fontsize=10)
            else:
                # 在全部曲线中，误差使用次坐标轴
                ax2 = self.ax.twinx()
                ax2.plot(self.time_data, self.error_data,
                        color=self.colors['error'], linewidth=1.5, alpha=0.7,
                        label='误差')
                ax2.set_ylabel('误差 (米)', color=self.colors['error'], fontsize=10)
                ax2.tick_params(axis='y', labelcolor=self.colors['error'])
        
        if plot_type == "控制量曲线" or plot_type == "全部曲线":
            # 绘制控制量曲线
            if plot_type == "控制量曲线":
                self.ax.plot(self.time_data, self.control_data,
                           color=self.colors['control'], linewidth=2,
                           label='控制量')
                self.ax.set_ylabel('控制量', color=self.colors['control'], fontsize=10)
            elif plot_type == "全部曲线":
                # 在全部曲线中，控制量使用次坐标轴
                if 'ax2' not in locals():
                    ax2 = self.ax.twinx()
                ax2.plot(self.time_data, self.control_data,
                        color=self.colors['control'], linewidth=1.5, alpha=0.7,
                        label='控制量')
                ax2.set_ylabel('控制量 / 误差', color=self.colors['control'], fontsize=10)
        
        if plot_type == "流量曲线":
            # 绘制流量曲线
            self.ax.plot(self.time_data, self.inflow_data,
                       color=self.colors['inflow'], linewidth=2,
                       label='进水流速')
            self.ax.plot(self.time_data, self.outflow_data,
                       color=self.colors['outflow'], linewidth=2,
                       label='出水流速')
            self.ax.set_ylabel('流速 (m³/s)', fontsize=10)
        
        # 设置标题
        if plot_type == "水位跟踪":
            self.ax.set_title('水位跟踪曲线', fontsize=12, fontweight='bold', pad=15)
        elif plot_type == "误差曲线":
            self.ax.set_title('误差曲线', fontsize=12, fontweight='bold', pad=15)
        elif plot_type == "控制量曲线":
            self.ax.set_title('控制量曲线', fontsize=12, fontweight='bold', pad=15)
        elif plot_type == "流量曲线":
            self.ax.set_title('流量曲线', fontsize=12, fontweight='bold', pad=15)
        else:  # 全部曲线
            self.ax.set_title('全部曲线', fontsize=12, fontweight='bold', pad=15)
        
        # 设置x轴标签
        self.ax.set_xlabel('时间 (秒)', fontsize=10)
        
        # 设置网格
        self.ax.grid(True, alpha=0.3, linestyle='--')
        
        # 设置背景色
        self.ax.set_facecolor('#ffffff')
        
        # 设置坐标轴颜色
        self.ax.spines['bottom'].set_color('#cccccc')
        self.ax.spines['top'].set_color('#cccccc')
        self.ax.spines['left'].set_color('#cccccc')
        self.ax.spines['right'].set_color('#cccccc')
        
        # 自动调整坐标轴范围
        if self.time_data and max(self.time_data) > 0:
            self.ax.set_xlim(0, max(self.time_data) * 1.05)
        else:
            self.ax.set_xlim(0, 1.0)  # 初始范围
        
        # 添加图例
        if plot_type == "全部曲线":
            # 合并多个图例
            lines1, labels1 = self.ax.get_legend_handles_labels()
            if 'ax2' in locals():
                lines2, labels2 = ax2.get_legend_handles_labels()
                self.ax.legend(lines1 + lines2, labels1 + labels2, 
                             loc='upper right', fontsize=9, framealpha=0.9)
            else:
                self.ax.legend(loc='upper right', fontsize=9, framealpha=0.9)
        elif plot_type in ["水位跟踪", "误差曲线", "控制量曲线", "流量曲线"]:
            self.ax.legend(loc='upper right', fontsize=9, framealpha=0.9)
        
        # 刷新画布
        self.figure.tight_layout()
        self.canvas.draw()
    
    def clear_data(self):
        """清除所有数据"""
        self.time_data = []
        self.level_data = []
        self.target_data = []
        self.error_data = []
        self.control_data = []
        self.inflow_data = []
        self.outflow_data = []
        
        self.data_count_label.config(text="数据点: 0")
        self._init_plot()
    
    def export_data(self):
        """导出数据到文件"""
        try:
            import csv
            from datetime import datetime
            
            # 生成文件名
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"pid_simulation_data_{timestamp}.csv"
            
            # 写入CSV文件
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.writer(csvfile)
                
                # 写入表头
                writer.writerow(['时间(s)', '实际水位(m)', '目标水位(m)', 
                               '误差(m)', '控制量', '进水流速(m³/s)', '出水流速(m³/s)'])
                
                # 写入数据
                for i in range(len(self.time_data)):
                    writer.writerow([
                        f"{self.time_data[i]:.3f}",
                        f"{self.level_data[i]:.3f}",
                        f"{self.target_data[i]:.3f}",
                        f"{self.error_data[i]:.3f}",
                        f"{self.control_data[i]:.3f}",
                        f"{self.inflow_data[i]:.3f}",
                        f"{self.outflow_data[i]:.3f}"
                    ])
            
            print(f"数据已导出到: {filename}")
            
            # 显示成功消息
            import tkinter.messagebox as messagebox
            messagebox.showinfo("导出成功", f"数据已成功导出到文件:\n{filename}")
            
        except Exception as e:
            import tkinter.messagebox as messagebox
            messagebox.showerror("导出失败", f"导出数据时出错:\n{str(e)}")