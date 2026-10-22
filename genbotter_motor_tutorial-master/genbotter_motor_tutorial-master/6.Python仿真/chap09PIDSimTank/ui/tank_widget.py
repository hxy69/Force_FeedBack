# -*- coding: utf-8 -*-
"""
水箱动画显示部件 - tkinter 版本
"""

import tkinter as tk
import math

class TankWidget(tk.Frame):
    """水箱显示部件"""
    
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)
        
        # 水箱数据
        self.current_level = 0.0  # 初始水位为0
        self.target_level = 1.0
        self.inflow_rate = 0.0  # 进水流速 (m³/s)
        self.control_signal = 0.0  # 控制信号 (0-1)
        self.outflow_rate = 0.2
        self.error = 1.0  # 初始误差为1.0
        
        # PID各项数据
        self.p_term = 0.0
        self.i_term = 0.0
        self.d_term = 0.0
        
        # 仿真时间信息
        self.current_time = 0.0
        self.max_time = 120.0
        self.max_time_reached = False
        
        # 初始化UI
        self._init_ui()
    
    def _init_ui(self):
        """初始化UI"""
        self.configure(bg='#ffffff')
        
        # 标题
        self.title_label = tk.Label(self, text="水箱仿真显示", 
                                   font=("Microsoft YaHei", 16, "bold"),
                                   bg='#ffffff', fg='#2c3e50')
        self.title_label.pack(pady=10)
        
        # 仿真时间显示
        self.time_frame = tk.Frame(self, bg='#ffffff')
        self.time_frame.pack(pady=(0, 5))
        
        self.time_label = tk.Label(self.time_frame, 
                                   text="时间: 0.0 / 120.0 秒",
                                   font=("Microsoft YaHei", 10),
                                   bg='#ffffff', fg='#666666')
        self.time_label.pack(side='left', padx=(0, 10))
        
        self.progress_label = tk.Label(self.time_frame,
                                       text="进度: 0.0%",
                                       font=("Microsoft YaHei", 10),
                                       bg='#ffffff', fg='#666666')
        self.progress_label.pack(side='left')
        
        # 创建画布框架
        canvas_frame = tk.Frame(self, bg='#ffffff')
        canvas_frame.pack(pady=5, padx=10, expand=True, fill='both')
        
        # 水箱显示画布 - 
        self.canvas = tk.Canvas(canvas_frame, width=400, height=500, 
                               bg='#f0f5fa', highlightthickness=2,
                               highlightbackground='#cccccc')
        self.canvas.pack(expand=True)
        
        # 当前水位标签（独立显示，避免与进水口标签重叠）
        self.level_label = tk.Label(canvas_frame, 
                                   text=f"当前水位: {self.current_level:.3f} m",
                                   font=("Microsoft YaHei", 12, "bold"),
                                   bg='#f0f5fa', fg='#1E64C8')
        self.level_label.pack(pady=(0, 5))
        
        # 数据显示区域
        self.data_text = tk.Text(self, width=45, height=12,
                                font=("Consolas", 10),
                                bg='#ffffff', fg='#2c3e50',
                                relief='ridge', borderwidth=2)
        self.data_text.pack(pady=(0, 10), padx=10, fill='x')
        self.data_text.config(state='disabled')
        
        # 初始绘制
        self._draw_tank()
    
    def _draw_tank(self):
        """绘制水箱"""
        self.canvas.delete("all")
        
        width = self.canvas.winfo_width() or 400
        height = self.canvas.winfo_height() or 550
        
        # 调整边距，给进水口和出水口留出更多空间
        margin_top = 90   # 进一步增加顶部边距，确保所有文字显示
        margin_bottom = 120  # 进一步增加底部边距
        margin_side = 50   # 两侧边距
        
        # 水箱尺寸
        tank_width = width - 2 * margin_side
        tank_height = height - margin_top - margin_bottom
        
        # 确保水箱尺寸合理
        if tank_width < 100:
            tank_width = 100
        if tank_height < 100:
            tank_height = 100
        
        # 绘制水箱边框
        self.canvas.create_rectangle(margin_side, margin_top, 
                                    margin_side + tank_width, margin_top + tank_height,
                                    width=3, outline='#333333', fill='#ffffff')
        
        # 计算水位高度
        max_display_level = 2.0
        level_ratio = min(self.current_level / max_display_level, 1.0)
        water_height = tank_height * level_ratio
        
        # 绘制水位
        water_bottom = margin_top + tank_height
        water_top = water_bottom - water_height
        
        # 创建渐变效果
        if water_height > 0:
            # 使用矩形填充而不是线，提高性能
            for i in range(0, int(water_height), 5):  # 每5像素画一条线
                y = water_top + i
                # 计算颜色渐变
                ratio = i / water_height if water_height > 0 else 0
                r = int(30 + (100 - 30) * (1 - ratio))
                g = int(100 + (180 - 100) * (1 - ratio))
                b = int(200 + (255 - 200) * (1 - ratio))
                color = f'#{r:02x}{g:02x}{b:02x}'
                
                self.canvas.create_line(margin_side, y, 
                                       margin_side + tank_width, y,
                                       fill=color, width=5)
        
        # 绘制目标水位线
        target_ratio = min(self.target_level / max_display_level, 1.0)
        target_y = margin_top + tank_height - (tank_height * target_ratio)
        
        self.canvas.create_line(margin_side, target_y, 
                               margin_side + tank_width, target_y,
                               fill='#FF4444', width=2, dash=(5, 3))
        
        # 绘制目标水位标签
        self.canvas.create_text(margin_side + tank_width - 10, target_y - 8,
                               text=f"目标: {self.target_level:.2f}m",
                               fill='#FF4444', font=("Microsoft YaHei", 10, "bold"),
                               anchor='e')
        
        # 绘制进水口 (顶部)
        inlet_width = 60
        
        # 使用相同的比例计算进水口和出水口高度（最大流速1.0对应30像素）
        max_rate = 1.0  # 最大流速
        inlet_height = max(5, min(int(self.inflow_rate * 30 / max_rate), 30))
        
        inlet_x = margin_side + tank_width // 2 - inlet_width // 2
        inlet_y = margin_top - 30  # 进一步上移，给标签更多空间
        
        # 进水管道 - 根据进水流速改变高度
        self.canvas.create_rectangle(inlet_x, inlet_y, 
                                    inlet_x + inlet_width, inlet_y + inlet_height,
                                    fill='#007AFF', outline='#007AFF', width=2)
        
        # 进水箭头
        arrow_size = 8
        arrow_x = inlet_x + inlet_width // 2
        arrow_y = inlet_y + inlet_height // 2
        
        self.canvas.create_polygon(
            arrow_x, arrow_y - arrow_size,
            arrow_x - arrow_size, arrow_y,
            arrow_x + arrow_size, arrow_y,
            fill='#007AFF', outline='#007AFF'
        )
        
        # 进水口标签 - 显示控制信号和进水流速
        inflow_text = f"进水流速: {self.inflow_rate:.3f} m³/s"
        self.canvas.create_text(arrow_x, inlet_y - 20,  # 调整为更小的负值，确保在画布内
                               text=inflow_text,
                               fill='#007AFF', font=("Microsoft YaHei", 9),
                               justify='center')
        
        # 控制信号标签（单独显示）
        control_text = f"控制信号: {self.control_signal:.2f}"
        self.canvas.create_text(arrow_x, inlet_y - 40,  # 调整为更小的负值，确保在画布内
                               text=control_text,
                               fill='#007AFF', font=("Microsoft YaHei", 9),
                               justify='center')
        
        # 绘制出水口 (底部)
        outlet_width = 60
        
        # 使用相同的比例计算出水口高度
        outlet_height = max(5, min(int(self.outflow_rate * 30 / max_rate), 30))
        
        outlet_x = margin_side + tank_width // 2 - outlet_width // 2
        outlet_y = margin_top + tank_height + 30  # 进一步下移，给标签更多空间
        
        # 出水管道 - 根据出水流速改变高度
        self.canvas.create_rectangle(outlet_x, outlet_y,
                                    outlet_x + outlet_width, outlet_y + outlet_height,
                                    fill='#FF3B30', outline='#FF3B30', width=2)
        
        # 出水箭头
        arrow_x = outlet_x + outlet_width // 2
        arrow_y = outlet_y + outlet_height // 2
        
        self.canvas.create_polygon(
            arrow_x, arrow_y + arrow_size,
            arrow_x - arrow_size, arrow_y,
            arrow_x + arrow_size, arrow_y,
            fill='#FF3B30', outline='#FF3B30'
        )
        
        # 出水口标签 - 确保完全显示
        outflow_text = f"出水流速: {self.outflow_rate:.3f} m³/s"
        
        # 检查画布边界，确保标签在画布内
        label_y = outlet_y + outlet_height + 25  # 进一步下移，确保显示
        
        # 如果标签位置超出了画布底部边界，调整位置
        if label_y + 15 > height:  # 15是字体的大致高度
            label_y = height - 20  # 放置在画布底部边缘上方
        
        self.canvas.create_text(arrow_x, label_y,
                               text=outflow_text,
                               fill='#FF3B30', font=("Microsoft YaHei", 9),
                               anchor='n')  # 使用anchor='n'确保文字向下扩展
        
        # 绘制刻度尺
        self._draw_scale(margin_side, margin_top, tank_width, tank_height, max_display_level)
    
    def _draw_scale(self, x, y, width, height, max_level):
        """绘制刻度尺"""
        scale_steps = 10
        step_height = height / scale_steps
        
        for i in range(scale_steps + 1):
            scale_y = y + height - (i * step_height)
            
            # 刻度线
            self.canvas.create_line(x - 10, scale_y, x, scale_y,
                                   fill='#666666', width=1)
            self.canvas.create_line(x + width, scale_y, x + width + 10, scale_y,
                                   fill='#666666', width=1)
            
            # 刻度值
            level_value = (i * step_height / height) * max_level
            level_text = f"{level_value:.1f}m"
            
            # 左侧刻度
            self.canvas.create_text(x - 15, scale_y + 4,
                                   text=level_text, anchor='e',
                                   fill='#666666', font=("Microsoft YaHei", 8))
            
            # 右侧刻度
            self.canvas.create_text(x + width + 15, scale_y + 4,
                                   text=level_text, anchor='w',
                                   fill='#666666', font=("Microsoft YaHei", 8))
    
    def update_data(self, data):
        """更新显示数据"""
        self.current_level = data.get('level', 0.0)
        self.target_level = data.get('target', 1.0)
        self.inflow_rate = data.get('inflow', 0.0)
        self.control_signal = data.get('control', 0.0)
        self.outflow_rate = data.get('outflow', 0.2)
        self.error = data.get('error', 0.0)
        self.current_time = data.get('time', 0.0)
        
        # 获取最大时间信息
        self.max_time = data.get('max_time', 120.0)
        self.max_time_reached = data.get('max_time_reached', False)
        
        # PID各项数据
        self.p_term = data.get('P', 0.0)
        self.i_term = data.get('I', 0.0)
        self.d_term = data.get('D', 0.0)
        
        # 更新当前水位标签
        self.level_label.config(text=f"当前水位: {self.current_level:.3f} m")
        
        # 更新仿真时间显示
        progress = min(self.current_time / self.max_time * 100, 100) if self.max_time > 0 else 0
        time_text = f"时间: {self.current_time:.1f} / {self.max_time:.1f} 秒"
        progress_text = f"进度: {progress:.1f}%"
        
        if self.max_time_reached:
            time_text += " (已完成)"
            progress_text = "进度: 100.0% (已完成)"
        
        self.time_label.config(text=time_text)
        self.progress_label.config(text=progress_text)
        
        # 更新数据显示
        self._update_data_display()
        
        # 重绘水箱
        self._draw_tank()
    
    def _update_data_display(self):
        """更新数据标签显示"""
        data_text = ""
        
        # 仿真时间信息
        data_text += f"仿真时间: {self.current_time:.1f} / {self.max_time:.1f} 秒\n"
        if self.max_time_reached:
            data_text += "状态: 仿真已完成\n"
        data_text += "-" * 40 + "\n"
        
        # 水位信息
        data_text += f"当前水位: {self.current_level:.3f} m\n"
        data_text += f"目标水位: {self.target_level:.3f} m\n"
        data_text += f"水位误差: {self.error:+.3f} m\n"
        data_text += "-" * 40 + "\n"
        
        # 控制信息
        data_text += f"控制信号: {self.control_signal:.3f} (0-1)\n"
        data_text += f"进水流速: {self.inflow_rate:.3f} m³/s\n"
        data_text += f"出水流速: {self.outflow_rate:.3f} m³/s\n"
        data_text += f"净流量: {self.inflow_rate - self.outflow_rate:+.3f} m³/s\n"
        data_text += "-" * 40 + "\n"
        
        # PID各项信息
        if abs(self.p_term) > 0 or abs(self.i_term) > 0 or abs(self.d_term) > 0:
            data_text += f"P项: {self.p_term:.3f} | I项: {self.i_term:.3f} | D项: {self.d_term:.3f}\n"
            data_text += f"PID总和: {self.p_term + self.i_term + self.d_term:.3f}"
        
        # 更新文本控件
        self.data_text.config(state='normal')
        self.data_text.delete(1.0, tk.END)
        self.data_text.insert(1.0, data_text)
        self.data_text.config(state='disabled')
    
    def reset(self):
        """重置水箱显示"""
        self.current_level = 0.0
        self.inflow_rate = 0.0
        self.control_signal = 0.0
        self.error = self.target_level  # 误差为目标水位
        self.current_time = 0.0
        self.max_time_reached = False
        
        # PID各项数据
        self.p_term = 0.0
        self.i_term = 0.0
        self.d_term = 0.0
        
        # 更新显示
        self.level_label.config(text=f"当前水位: {self.current_level:.3f} m")
        self.time_label.config(text=f"时间: 0.0 / {self.max_time:.1f} 秒")
        self.progress_label.config(text="进度: 0.0%")
        self._update_data_display()
        self._draw_tank()
    
    def resize(self, event=None):
        """响应尺寸变化"""
        self._draw_tank()