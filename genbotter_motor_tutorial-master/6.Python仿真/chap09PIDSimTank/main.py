#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PID水箱教学仿真应用 - 主程序 (tkinter版本)
对应课件：9-1 PID控制结构与原理
作者：高博士教学团队
"""

import sys
import os
import tkinter as tk  # 补充缺失的tkinter导入

# 添加项目根目录到Python路径
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from ui.main_window import MainWindow

def main():
    """应用程序入口"""
    try:
        # 创建并运行主窗口
        app = MainWindow()
        
        # 设置窗口关闭事件
        app.protocol("WM_DELETE_WINDOW", app.on_closing)
        
        # 进入主事件循环
        app.mainloop()
        
    except Exception as e:
        import tkinter.messagebox as messagebox
        import traceback
        
        error_msg = f"应用程序启动失败:\n\n{str(e)}\n\n{traceback.format_exc()}"
        print(error_msg)
        
        # 使用 tkinter 显示错误对话框
        root = tk.Tk()
        root.withdraw()  # 隐藏主窗口
        messagebox.showerror("启动错误", error_msg)
        sys.exit(1)

if __name__ == "__main__":
    main()