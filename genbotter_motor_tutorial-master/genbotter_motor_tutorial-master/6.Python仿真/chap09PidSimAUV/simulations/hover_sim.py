# simulations/hover_sim.py - 增强版仿真器

import numpy as np

class HoverSimulation:
    """
    增强版悬停仿真，包含更多现实因素
    """
    
    def __init__(self, pid, uav, max_height=10.0, wind_enabled=False):
        """
        初始化仿真
        
        参数:
            pid: PID控制器实例
            uav: 无人机模型实例
            max_height: 最大飞行高度
            wind_enabled: 是否启用风干扰
        """
        self.pid = pid
        self.uav = uav
        self.max_height = max_height
        
        # 目标高度（初始设为中间值）
        self.target_height = max_height / 2
        
        # 新增：仿真统计
        self.time = 0.0
        self.error_history = []
        self.thrust_history = []
        
        # 新增：环境干扰
        self.wind_enabled = wind_enabled
        self.wind_strength = 0.0      # 当前风力
        self.wind_direction = 1       # 风向 (1:向上, -1:向下)
        self.wind_update_time = 0.0
        
        # 新增：随机干扰（模拟气流）
        self.turbulence_enabled = True
        self.turbulence_strength = 0.1
    
    def reset(self, to_ground=False):
        """
        重置仿真
        
        参数:
            to_ground: 是否重置到地面
        """
        self.pid.reset()
        if to_ground:
            self.uav.reset(0.0)
        
        # 重置仿真状态
        self.time = 0.0
        self.error_history.clear()
        self.thrust_history.clear()
        self.wind_strength = 0.0
    
    def step(self, dt):
        """
        执行单步仿真
        
        参数:
            dt: 时间步长 (秒)
        """
        if dt <= 0:
            return
        
        # 1. 更新时间
        self.time += dt
        
        # 2. 更新环境干扰
        if self.wind_enabled:
            self._update_wind(dt)
        
        # 3. 计算当前误差
        error = self.target_height - self.uav.y
        
        # 4. 添加随机干扰（模拟气流）
        if self.turbulence_enabled:
            turbulence = np.random.normal(0, self.turbulence_strength)
            error += turbulence
        
        # 5. 获取PID输出
        thrust = self.pid.update(error, dt)
        
        # 6. 添加风干扰力
        if self.wind_enabled and self.wind_strength > 0:
            wind_force = self.wind_strength * self.wind_direction
            thrust += wind_force
        
        # 7. 更新无人机状态
        self.uav.update(thrust, dt)
        
        # 8. 高度限制保护
        if self.uav.y < 0:
            self.uav.y = 0
            self.uav.v = max(self.uav.v, 0)  # 地面碰撞后速度归零
        elif self.uav.y > self.max_height:
            self.uav.y = self.max_height
            self.uav.v = min(self.uav.v, 0)  # 上限碰撞后速度归零
        
        # 9. 记录历史数据
        self.error_history.append(error)
        self.thrust_history.append(thrust)
        
        # 保持历史数据长度（防止内存无限增长）
        max_history = 1000
        if len(self.error_history) > max_history:
            self.error_history = self.error_history[-max_history:]
            self.thrust_history = self.thrust_history[-max_history:]
    
    def _update_wind(self, dt):
        """
        更新风模型
        模拟随时间变化的风
        """
        # 每5秒更新一次风向和强度
        self.wind_update_time += dt
        if self.wind_update_time > 5.0:
            # 随机生成新的风参数
            self.wind_strength = np.random.uniform(0, 1.5)  # 0-1.5 m/s的风速
            self.wind_direction = 1 if np.random.random() > 0.5 else -1
            self.wind_update_time = 0.0
    
    def set_target_height(self, height):
        """
        设置目标高度
        
        参数:
            height: 目标高度 (米)
        """
        self.target_height = max(0, min(height, self.max_height))
    
    def add_disturbance(self, strength=2.0, duration=0.5):
        """
        添加瞬时干扰（模拟阵风或冲击）
        
        参数:
            strength: 干扰强度
            duration: 持续时间 (秒)
        """
        # 在实际应用中，这里可以添加瞬时干扰
        pass
    
    def get_performance_metrics(self):
        """
        获取性能指标
        
        返回:
            包含各种性能指标的字典
        """
        if not self.error_history:
            return {}
        
        errors = np.array(self.error_history)
        
        return {
            'rmse': np.sqrt(np.mean(errors**2)),           # 均方根误差
            'max_error': np.max(np.abs(errors)),           # 最大误差
            'settling_time': self._calculate_settling_time(),  # 稳定时间
            'overshoot': self._calculate_overshoot(),          # 超调量
            'avg_thrust': np.mean(np.abs(self.thrust_history)) if self.thrust_history else 0
        }
    
    def _calculate_settling_time(self, threshold=0.05):
        """计算稳定时间（误差进入±5%范围内的时间）"""
        if len(self.error_history) < 10:
            return float('inf')
        
        target_range = self.target_height * threshold
        for i, error in enumerate(self.error_history[-500:]):  # 检查最近500个点
            if abs(error) <= target_range:
                return i * 0.02  # 假设dt=0.02
        return float('inf')
    
    def _calculate_overshoot(self):
        """计算超调量百分比"""
        if len(self.error_history) < 10:
            return 0.0
        
        # 找到最大超过目标的值
        overshoot = max(0, max(self.uav.y - self.target_height, 0))
        return (overshoot / self.target_height * 100) if self.target_height > 0 else 0.0
    
    def get_debug_info(self):
        """获取调试信息"""
        return {
            'time': self.time,
            'target_height': self.target_height,
            'current_height': self.uav.y,
            'current_error': self.target_height - self.uav.y,
            'wind_strength': self.wind_strength if self.wind_enabled else 0,
            'uav_state': self.uav.get_state()
        }
