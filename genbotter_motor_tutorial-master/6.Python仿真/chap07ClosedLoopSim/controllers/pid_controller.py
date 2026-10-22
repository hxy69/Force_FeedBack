"""
PID 控制器
包含完整的抗积分饱和、微分滤波和输出平滑
"""
class PIDController:
    def __init__(self, Kp, Ki, Kd, dt, output_limits=(-12, 12), windup_guard=20.0):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.dt = dt
        self.output_limits = output_limits
        self.windup_guard = windup_guard  # 积分抗饱和保护
        
        # 控制器状态
        self._integral = 0.0
        self._prev_error = 0.0
        self._prev_feedback = 0.0
        self._prev_output = 0.0
        self._derivative_filter = 0.0
        
        # 跟踪性能指标
        self.total_error = 0.0
        self.control_effort = 0.0

    def reset(self):
        """完全重置控制器状态"""
        self._integral = 0.0
        self._prev_error = 0.0
        self._prev_feedback = 0.0
        self._prev_output = 0.0
        self._derivative_filter = 0.0
        self.total_error = 0.0
        self.control_effort = 0.0

    def update(self, setpoint, feedback):
        """
        增强 PID 更新：抗饱和 + 微分滤波 + 输出平滑
        """
        # 计算误差
        error = setpoint - feedback
        
        # 比例项
        proportional = self.Kp * error
        
        # 积分项 (带抗饱和和积分限幅)
        integral_candidate = self._integral + error * self.dt
        
        # 积分抗饱和：仅在控制器未饱和时积分
        raw_output = proportional + self.Ki * integral_candidate + self._get_derivative(setpoint, feedback)
        output_saturated = (raw_output <= self.output_limits[0] or raw_output >= self.output_limits[1])
        
        if not output_saturated:
            self._integral = integral_candidate
        else:
            # 温和的积分抗饱和：限制积分增长
            self._integral += error * self.dt * 0.1  # 降低积分速度
        
        # 积分限幅
        max_integral = self.windup_guard / (self.Ki + 1e-6)  # 避免除零
        self._integral = max(min(self._integral, max_integral), -max_integral)
        
        integral_term = self.Ki * self._integral
        
        # 微分项 (对测量值微分 + 低通滤波)
        derivative_term = self._get_derivative(setpoint, feedback)
        
        # 计算PID输出
        output = proportional + integral_term + derivative_term
        
        # 输出限幅
        output = max(min(output, self.output_limits[1]), self.output_limits[0])
        
        # 输出平滑 (可选)
        # output = 0.8 * output + 0.2 * self._prev_output
        
        # 更新状态
        self._prev_error = error
        self._prev_feedback = feedback
        self._prev_output = output
        
        # 更新性能指标
        self.total_error += abs(error) * self.dt
        self.control_effort += abs(output) * self.dt
        
        return output
    
    def _get_derivative(self, setpoint, feedback):
        """计算滤波后的微分项"""
        # 对测量值微分，减少设定值变化的冲击
        derivative_raw = (feedback - self._prev_feedback) / self.dt
        
        # 一阶低通滤波
        alpha = 0.1  # 滤波系数
        self._derivative_filter = (1 - alpha) * self._derivative_filter - alpha * self.Kd * derivative_raw
        
        return self._derivative_filter
    
    def get_parameters(self):
        """获取当前PID参数"""
        return {
            'Kp': self.Kp,
            'Ki': self.Ki, 
            'Kd': self.Kd,
            'integral': self._integral,
            'total_error': self.total_error,
            'control_effort': self.control_effort
        }