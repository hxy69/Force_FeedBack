"""
仿真引擎：实时数据管理和性能分析
"""
import numpy as np
import time

class SimulationEngine:
    def __init__(self, motor, controller, dt=0.001, max_time=10.0, buffer_size=10000):
        self.motor = motor
        self.controller = controller
        self.dt = dt
        self.max_time = max_time
        self.buffer_size = buffer_size
        
        # 实时数据缓冲区
        self.reset()
        
        # 性能统计
        self.simulation_time = 0.0
        self.step_count = 0
        self.real_time_factor = 1.0

    def reset(self):
        """重置仿真状态和数据"""
        self.motor.reset()
        self.controller.reset()
        
        # 初始化数据缓冲区
        self.time_buffer = np.zeros(self.buffer_size)
        self.speed_buffer = np.zeros(self.buffer_size)
        self.voltage_buffer = np.zeros(self.buffer_size)
        self.error_buffer = np.zeros(self.buffer_size)
        self.current_buffer = np.zeros(self.buffer_size)
        
        self.current_index = 0
        self.simulation_time = 0.0
        self.step_count = 0
        self.last_step_time = time.time()

    def step(self, setpoint):
        """
        执行单步仿真，返回当前速度和电压
        """
        if self.current_index >= self.buffer_size:
            # 缓冲区满，从头开始覆盖（循环缓冲区）
            self.current_index = 0
        
        # 控制器计算
        feedback = self.motor.omega
        control_voltage = self.controller.update(setpoint, feedback)
        
        # 电机动力学更新
        current_speed = self.motor.step(control_voltage)
        motor_state = self.motor.get_state()
        
        # 记录数据
        idx = self.current_index
        self.time_buffer[idx] = self.simulation_time
        self.speed_buffer[idx] = current_speed
        self.voltage_buffer[idx] = control_voltage
        self.error_buffer[idx] = setpoint - current_speed
        self.current_buffer[idx] = motor_state['current']
        
        # 更新状态
        self.current_index += 1
        self.simulation_time += self.dt
        self.step_count += 1
        
        # 计算实时因子
        current_time = time.time()
        if self.step_count % 100 == 0:
            elapsed_real_time = current_time - self.last_step_time
            simulated_time = 100 * self.dt
            self.real_time_factor = simulated_time / elapsed_real_time if elapsed_real_time > 0 else 1.0
            self.last_step_time = current_time
        
        return current_speed, control_voltage

    def get_current_data(self, window_size=None):
        """
        获取当前仿真数据
        window_size: 返回最近的数据点数，None表示全部数据
        """
        if self.current_index == 0:
            end = 1
        else:
            end = self.current_index
        
        if window_size and end > window_size:
            start = end - window_size
        else:
            start = 0
        
        return {
            't': self.time_buffer[start:end],
            'omega': self.speed_buffer[start:end],
            'u': self.voltage_buffer[start:end],
            'error': self.error_buffer[start:end],
            'current': self.current_buffer[start:end]
        }
    
    def get_performance_metrics(self, setpoint):
        """计算系统性能指标"""
        if self.current_index < 10:
            return None
            
        data = self.get_current_data()
        t = data['t']
        y = data['omega']
        
        if len(y) < 10:
            return None
        
        # 计算性能指标
        metrics = {}
        
        # 稳态误差 (最后10%数据)
        steady_start = max(0, int(0.9 * len(y)))
        steady_state = np.mean(y[steady_start:])
        metrics['steady_state_error'] = abs(setpoint - steady_state)
        
        # 超调量
        max_overshoot = np.max(y)
        metrics['overshoot_percent'] = max(0, (max_overshoot - setpoint) / setpoint * 100)
        
        # 上升时间 (10% 到 90%)
        try:
            idx_10 = np.where(y >= 0.1 * setpoint)[0][0]
            idx_90 = np.where(y >= 0.9 * setpoint)[0][0]
            metrics['rise_time'] = t[idx_90] - t[idx_10]
        except:
            metrics['rise_time'] = float('nan')
        
        # 调节时间 (进入±2%误差带)
        try:
            settling_threshold = 0.02 * setpoint
            settling_idx = np.where(np.abs(y - setpoint) <= settling_threshold)[0]
            if len(settling_idx) > 0:
                # 找到持续在误差带内的点
                metrics['settling_time'] = t[settling_idx[0]]
            else:
                metrics['settling_time'] = float('nan')
        except:
            metrics['settling_time'] = float('nan')
        
        # IAE (绝对误差积分)
        metrics['iae'] = np.trapz(np.abs(data['error']), t)
        
        # ITAE (时间乘绝对误差积分)
        metrics['itae'] = np.trapz(t * np.abs(data['error']), t)
        
        # 控制努力
        metrics['control_effort'] = np.trapz(np.abs(data['u']), t)
        
        metrics['real_time_factor'] = self.real_time_factor
        
        return metrics
    
    def get_system_info(self):
        """获取系统状态信息"""
        return {
            'simulation_time': self.simulation_time,
            'step_count': self.step_count,
            'buffer_usage': f"{self.current_index}/{self.buffer_size}",
            'real_time_factor': self.real_time_factor,
            'motor_state': self.motor.get_state(),
            'controller_params': self.controller.get_parameters()
        }