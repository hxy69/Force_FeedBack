"""
一阶直流电机模型
"""
class DCMotor:
    def __init__(self, Kt=0.1, J=0.01, B=0.001, tau=0.05, dt=0.001, R=1.0, L=0.01):
        """
        参数说明:
        Kt: 转矩常数 (N·m/A)
        J: 转动惯量 (kg·m²)
        B: 粘性阻尼系数 (N·m·s/rad)
        tau: 电气时间常数 (s)
        R: 电枢电阻 (Ω)
        L: 电枢电感 (H)
        """
        self.Kt = Kt
        self.J = J
        self.B = B
        self.tau = tau
        self.R = R
        self.L = L
        self.dt = dt
        self.omega = 0.0  # 角速度 (rad/s)
        self.current = 0.0  # 电流 (A)
        
    def reset(self):
        """重置电机状态"""
        self.omega = 0.0
        self.current = 0.0

    def step(self, voltage):
        """
        改进的一阶电机模型
        考虑电气和机械动力学
        """
        # 限制输入电压在合理范围内
        voltage = max(min(voltage, 12), -12)
        
        # 电气动力学: 电流响应 (一阶惯性)
        target_current = (voltage - self.Kt * self.omega) / self.R
        self.current += (target_current - self.current) * self.dt / self.tau
        
        # 机械动力学: 速度响应
        torque = self.Kt * self.current  # 电磁转矩
        friction_torque = self.B * self.omega  # 摩擦转矩
        acceleration = (torque - friction_torque) / self.J
        self.omega += acceleration * self.dt
        
        # 添加微小噪声模拟现实不确定性
        import random
        self.omega += random.gauss(0, 0.01) * self.dt
        
        return self.omega
    
    def get_state(self):
        """获取电机当前状态"""
        return {
            'speed': self.omega,
            'current': self.current,
            'torque': self.Kt * self.current
        }