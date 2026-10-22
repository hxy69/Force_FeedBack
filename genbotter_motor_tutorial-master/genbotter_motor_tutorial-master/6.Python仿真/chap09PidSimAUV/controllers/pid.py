# controllers/pid.py - 增强版

class PID:
    def __init__(self, kp=1.2, ki=0.0, kd=0.0, output_limit=10.0, integral_limit=5.0):
        """
        增强版PID控制器
        
        参数:
            kp: 比例增益
            ki: 积分增益  
            kd: 微分增益
            output_limit: 输出限制 (±N)
            integral_limit: 积分抗饱和限制
        """
        self.kp = kp
        self.ki = ki
        self.kd = kd
        
        # 新增：限制参数
        self.output_limit = output_limit
        self.integral_limit = integral_limit
        
        # 状态变量
        self.integral = 0.0
        self.last_error = 0.0
        self.last_output = 0.0
        
        # 新增：低通滤波器用于微分项（减少噪声敏感）
        self.alpha = 0.3  # 滤波器系数
        
    def reset(self):
        """重置控制器状态"""
        self.integral = 0.0
        self.last_error = 0.0
        self.last_output = 0.0
    
    def update(self, error, dt):
        """
        计算PID输出
        
        参数:
            error: 当前误差
            dt: 时间步长 (秒)
        
        返回:
            控制输出 (经过限制)
        """
        if dt <= 0:
            return self.last_output
        
        # 1. 比例项
        p_term = self.kp * error
        
        # 2. 积分项 (带抗饱和)
        self.integral += error * dt
        # 积分抗饱和：限制积分积累
        self.integral = max(min(self.integral, self.integral_limit), -self.integral_limit)
        i_term = self.ki * self.integral
        
        # 3. 微分项 (带低通滤波)
        if self.last_error is not None:
            raw_derivative = (error - self.last_error) / dt
            # 低通滤波减少噪声影响
            if hasattr(self, 'filtered_derivative'):
                self.filtered_derivative = (self.alpha * raw_derivative + 
                                          (1 - self.alpha) * self.filtered_derivative)
            else:
                self.filtered_derivative = raw_derivative
            d_term = self.kd * self.filtered_derivative
        else:
            d_term = 0.0
        
        # 4. 计算总输出
        output = p_term + i_term + d_term
        
        # 5. 输出限制 (真实无人机推力有限)
        output = max(min(output, self.output_limit), -self.output_limit)
        
        # 6. 保存状态
        self.last_error = error
        self.last_output = output
        
        return output
    
    def get_terms(self):
        """获取各项贡献 (用于调试)"""
        return {
            'kp': self.kp,
            'ki': self.ki, 
            'kd': self.kd,
            'integral': self.integral,
            'last_error': self.last_error
        }
