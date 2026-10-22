# -*- coding: utf-8 -*-
"""
PID控制器实现 - 优化用于教学演示
支持P、PI、PID三种模式切换
"""

class PIDController:
    """PID控制器"""
    
    def __init__(self, Kp=0.8, Ki=0.0, Kd=0.0, mode='P'):
        """
        初始化PID控制器
        
        Args:
            Kp: 比例系数
            Ki: 积分系数
            Kd: 微分系数
            mode: 控制模式 ('P', 'PI', 'PID')
        """
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.mode = mode
        
        # 状态变量
        self.integral = 0.0     # 积分项累积
        self.prev_error = 0.0   # 上一次误差
        self.output = 0.0       # 当前输出
        self.prev_level = 0.0   # 用于微分计算的水位值
        
        # 抗积分饱和
        self.integral_limit = 2.0
        
        # 历史记录
        self.error_history = []
        self.output_history = []
        self.p_term_history = []
        self.i_term_history = []
        self.d_term_history = []
        self.time_history = []
        
        # 输出限幅
        self.output_min = 0.0
        self.output_max = 1.0
        
        # 微分项滤波器（防止微分项噪声放大）
        self.derivative_filter = 0.1
        self.filtered_derivative = 0.0
    
    def calculate(self, error, dt, current_level=None):
        """
        计算控制输出
        
        Args:
            error: 当前误差
            dt: 时间步长
            current_level: 当前水位（用于微分项）
            
        Returns:
            float: 控制信号 (0-1之间)
        """
        # 记录误差
        self.error_history.append(error)
        
        # 计算比例项
        p_term = self.Kp * error
        
        # 计算积分项 (根据模式决定是否启用)
        i_term = 0.0
        if self.mode in ['PI', 'PID'] and self.Ki > 0:
            self.integral += error * dt
            
            # 抗积分饱和（为了演示效果，暂时取消限制）
            # self.integral = max(-self.integral_limit, min(self.integral, self.integral_limit))
            
            i_term = self.Ki * self.integral
        
        # 计算微分项 (根据模式决定是否启用)
        d_term = 0.0
        if self.mode == 'PID' and self.Kd > 0 and current_level is not None:
            # 检查这里是否真的执行了
            #print(f"微分计算：prev_level={self.prev_level}, current_level={current_level}")
            if self.prev_level is not None:
                # 计算水位变化率（使用对输出的微分，避免设定值突变的冲击）
                level_derivative = (current_level - self.prev_level) / dt
                
                # 应用一阶低通滤波器，减少噪声影响
                self.filtered_derivative = (1 - self.derivative_filter) * self.filtered_derivative + \
                                          self.derivative_filter * level_derivative
                
                # 微分项为负反馈：水位上升时应减小控制量
                d_term = -self.Kd * self.filtered_derivative
            
            # 更新上一次水位
            self.prev_level = current_level
        
        # 计算总输出
        raw_output = p_term + i_term + d_term
        
        # 输出限幅（0-1之间）
        self.output = raw_output
#        self.output = max(self.output_min, min(raw_output, self.output_max))
        
        # 保存当前误差供下次使用
        self.prev_error = error
        
        # 记录各项历史
        self.output_history.append(self.output)
        self.p_term_history.append(p_term)
        self.i_term_history.append(i_term)
        self.d_term_history.append(d_term)
        
        return self.output
    
    def set_mode(self, mode):
        """设置控制模式"""
        valid_modes = ['P', 'PI', 'PID']
        if mode in valid_modes:
            self.mode = mode
            # 切换模式时重置状态
            self.integral = 0.0
            self.filtered_derivative = 0.0
            self.prev_level = None
            return True
        return False
    
    def set_parameters(self, Kp=None, Ki=None, Kd=None):
        """设置PID参数"""
        if Kp is not None:
            self.Kp = max(0.0, Kp)
        if Ki is not None:
            self.Ki = max(0.0, Ki)
        if Kd is not None:
            self.Kd = max(0.0, Kd)
    
    def reset(self):
        """重置控制器状态"""
        self.integral = 0.0
        self.prev_error = 0.0
        self.output = 0.0
        self.filtered_derivative = 0.0
        self.prev_level = None
        self.error_history = []
        self.output_history = []
        self.p_term_history = []
        self.i_term_history = []
        self.d_term_history = []
        self.time_history = []
    
    def get_current_terms(self):
        """获取当前各项的值"""
        if not self.error_history:
            return {'P': 0, 'I': 0, 'D': 0, 'total': 0}
        
        return {
            'P': self.p_term_history[-1] if self.p_term_history else 0,
            'I': self.i_term_history[-1] if self.i_term_history else 0,
            'D': self.d_term_history[-1] if self.d_term_history else 0,
            'total': self.output_history[-1] if self.output_history else 0
        }
    
    def get_formula(self):
        """获取当前控制律公式"""
        if self.mode == 'P':
            return "u(t) = Kp × e(t)"
        elif self.mode == 'PI':
            return "u(t) = Kp × e(t) + Ki × ∫e(t)dt"
        else:  # PID
            return "u(t) = Kp × e(t) + Ki × ∫e(t)dt + Kd × (-dy(t)/dt)"
    
    def get_history(self):
        """获取历史数据"""
        return {
            'error': self.error_history,
            'output': self.output_history,
            'p_term': self.p_term_history,
            'i_term': self.i_term_history,
            'd_term': self.d_term_history
        }