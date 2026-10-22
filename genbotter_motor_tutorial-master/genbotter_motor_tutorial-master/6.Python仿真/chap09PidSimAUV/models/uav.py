# models/uav.py - 优化版（响应灵敏）

class UAV:
    """
    优化版无人机模型 - 对PID控制更敏感
    简化物理模型，强调控制响应
    """
    
    def __init__(self, response_gain=2.5, mass=0.3, drag_coefficient=0.01, max_speed=10.0):
        """
        初始化无人机参数（优化值）
        
        参数:
            response_gain: 响应增益 (增大到2.5-3.0)
            mass: 质量 (减小到0.5kg，更轻)
            drag_coefficient: 空气阻力系数 (大幅减小到0.01)
            max_speed: 最大垂直速度 (增大到10m/s)
        """
        # 物理状态
        self.y = 0.0      # 高度 (m)
        self.v = 0.0      # 速度 (m/s)
        self.a = 0.0      # 加速度 (m/s²)
        
        # 优化参数 - 关键修改！
        self.mass = mass                    # 质量减小：0.5kg (原1.0kg)
        self.response_gain = response_gain  # 响应增益增加：2.5 (原1.0)
        self.drag_coefficient = drag_coefficient  # 阻力大幅减小：0.01 (原0.1)
        self.max_speed = max_speed          # 最大速度增加：10m/s (原5m/s)
        self.gravity = 9.81                 # 重力加速度 (m/s²)
        
        # 移除推力延迟 - 简化模型，让控制立即响应
        # self.thrust_delay = 0.05
        # self.thrust_buffer = []
        
        # 优化性能参数 - 关键！
        self.max_thrust = 80.0              # 最大推力增加：30N (原15N)
        self.min_thrust = 0.0               # 最小推力设为0（不能向下推）
        
    def update(self, thrust_command, dt):
        """
        更新无人机状态 - 简化版，更响应
        
        参数:
            thrust_command: 期望推力 (N)
            dt: 时间步长 (秒)
        """
        # 1. 移除推力延迟 - 直接使用指令
        thrust = thrust_command
        
        # 2. 推力限制（优化值）
        thrust = max(min(thrust, self.max_thrust), self.min_thrust)
        
        # 3. 计算净力 - 关键优化！
        # 总力 = 推力 - 重力 - 空气阻力
        # 大幅减小阻力影响
        drag_force = self.drag_coefficient * self.v  # 改为线性阻力，更小影响
        net_force = thrust - self.mass * self.gravity - drag_force
        
        # 4. 应用响应增益 - 增强控制响应
        net_force *= self.response_gain
        
        # 5. 计算加速度 (F = ma)
        self.a = net_force / self.mass
        
        # 6. 更新速度 (带速度限制，但限制放宽)
        self.v += self.a * dt
        
        # 简单阻尼：防止无限加速，但不要过度抑制
        self.v *= 0.99  # 很小的阻尼
        
        # 速度限制（放宽）
        self.v = max(min(self.v, self.max_speed), -self.max_speed)
        
        # 7. 更新高度
        self.y += self.v * dt
        
        # 8. 高度保护（不能低于地面）
        if self.y < 0:
            self.y = 0
            self.v = max(self.v, 0)  # 撞地后速度归零
    
    def reset(self, y=0.0):
        """
        重置无人机状态
        
        参数:
            y: 初始高度
        """
        self.y = y
        self.v = 0.0
        self.a = 0.0
        
    def get_state(self):
        """获取完整状态信息"""
        return {
            'height': self.y,
            'velocity': self.v,
            'acceleration': self.a,
            'mass': self.mass,
            'response_gain': self.response_gain,
            'max_thrust': self.max_thrust
        }