# gui/canvas_view.py

import tkinter as tk
from utils.mapping import height_to_canvas_y


class CanvasView:
    def __init__(self, parent, max_height, width=800, height=600):
        self.max_height = max_height
        self.parent = parent  # 保存父容器引用

        # 创建一个Frame作为容器，内部使用pack，外部使用grid
        self.frame = tk.Frame(parent)
        
        self.canvas = tk.Canvas(
            self.frame,  # 改为使用frame
            width=width,
            height=height,
            bg="white"
        )
        self.canvas.pack(fill=tk.BOTH, expand=True)

        # UAV image
        try:
            self.uav_img = tk.PhotoImage(file="gui/assets/uav.png")
        except:
            # 如果图片不存在，创建一个简单的替代
            self.uav_img = None

        # ===============================
        # Height tracking plot settings
        # ===============================
        self.plot_ratio = 0.35          # bottom 35%
        self.height_history = []        # full response history

    # =================================================
    # External API
    # =================================================
    def reset_plot(self):
        """Clear height tracking history"""
        self.height_history.clear()

    # =================================================
    # 新增：获取frame的方法，用于grid布局
    # =================================================
    def get_frame(self):
        """返回用于grid布局的frame"""
        return self.frame

    # =================================================
    # Main update entry
    # =================================================
    def update_view(self, uav_height, target_height):
        self.canvas.delete("all")

        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        
        # 如果画布尺寸为1x1（未初始化），使用默认尺寸
        if canvas_w <= 1 or canvas_h <= 1:
            canvas_w = 800
            canvas_h = 600

        # Layout split
        plot_h = int(canvas_h * self.plot_ratio)
        scene_h = canvas_h - plot_h

        # -------------------------
        # Upper: UAV physical scene
        # -------------------------
        self._draw_ground(canvas_w, scene_h)
        self._draw_target(canvas_w, scene_h, target_height)
        self._draw_uav(canvas_w, scene_h, uav_height)

        # -------------------------
        # Lower: height tracking plot
        # -------------------------
        self._update_height_history(uav_height)
        self._draw_height_plot(
            canvas_w,
            scene_h,
            plot_h,
            target_height
        )

        # Error text
        self._draw_error_text(
            uav_height,
            target_height,
            scene_h,
            plot_h
        )

    # =================================================
    # UAV scene
    # =================================================
    def _draw_ground(self, w, h):
        ground_y = h - 10

        self.canvas.create_line(
            0, ground_y, w, ground_y,
            fill="green",
            width=6
        )

        self.canvas.create_text(
            10, ground_y - 8,
            anchor="w",
            fill="green",
            font=("Arial", 10, "bold"),
            text="Ground (0 m)"
        )

    def _draw_target(self, w, h, target_height):
        y = height_to_canvas_y(
            target_height,
            self.max_height,
            h
        )

        self.canvas.create_line(
            0, y, w, y,
            fill="red",
            dash=(6, 4),
            width=2
        )

        self.canvas.create_text(
            10,
            y - 12,
            anchor="w",
            fill="red",
            font=("Arial", 10, "bold"),
            text=f"Target altitude = {target_height:.2f} m"
        )

    def _draw_uav(self, w, h, uav_height):
        y = height_to_canvas_y(
            uav_height,
            self.max_height,
            h
        )

        x = w // 2

        if self.uav_img:
            self.canvas.create_image(x, y, image=self.uav_img)
        else:
            # 绘制简单的无人机图标
            self.canvas.create_oval(x-15, y-10, x+15, y+10, fill="blue", outline="black", width=2)
            self.canvas.create_line(x, y-10, x, y-25, fill="black", width=3)  # 顶部天线

        self.canvas.create_text(
            x + 70,
            y - 20,
            anchor="w",
            fill="black",
            font=("Arial", 10, "bold"),
            text=f"Altitude = {uav_height:.2f} m"
        )

    # =================================================
    # Height tracking plot (FULL history)
    # =================================================
    def _update_height_history(self, uav_height):
        self.height_history.append(uav_height)

    def _draw_height_plot(self, w, y_offset, h, target_height):
        # Background
        self.canvas.create_rectangle(
            0, y_offset, w, y_offset + h,
            fill="#f4f4f4",
            outline=""
        )

        self.canvas.create_text(
            10,
            y_offset + 8,
            anchor="nw",
            fill="black",
            font=("Arial", 10, "bold"),
            text="Altitude tracking y(t)"
        )

        # Target line
        target_y = y_offset + h * (1 - target_height / self.max_height)

        self.canvas.create_line(
            0, target_y, w, target_y,
            fill="red",
            dash=(6, 4),
            width=2
        )

        # Actual response
        if len(self.height_history) < 2:
            return

        total_points = len(self.height_history)
        dx = w / max(total_points - 1, 1)

        points = []
        for i, hgt in enumerate(self.height_history):
            x = i * dx
            y = y_offset + h * (1 - hgt / self.max_height)
            points.extend([x, y])

        self.canvas.create_line(
            points,
            fill="blue",
            width=3
        )

    # =================================================
    # Error display
    # =================================================
    def _draw_error_text(self, uav_height, target_height, y_offset, h):
        error = target_height - uav_height

        self.canvas.create_text(
            10,
            y_offset + h - 8,
            anchor="sw",
            fill="black",
            font=("Arial", 10, "bold"),
            text=f"Error e(t) = {error:.3f} m"
        )