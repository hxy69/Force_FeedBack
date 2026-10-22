# -*- coding: utf-8 -*-
"""
水箱物理模型
实现课件中的水池模型：
- 目标水位：1.0米
- 出水口：固定出水量 0.2 单位/秒
- 进水口：通过水泵控制，控制量u(t)由PID计算得出
"""

class WaterTank:
    """水箱物理模型"""
    
    def __init__(self, config):
        """
        初始化水箱
        
        Args:
            config: 配置字典，包含高度、底面积等参数
        """
        self.height = config['height']           # 水箱高度 (米)
        self.area = config['area']               # 底面积 (平方米)
        self.max_level = config['max_water_level']  # 最大安全水位
        self.current_level = 0.0  # 初始水位设为0，与课件一致
        
        # 出水口配置
        self.outflow_rate = config.get('fixed_outflow_rate', 0.2)  # 固定出水流速 (m³/s)，与课件一致
        self.max_outflow = config.get('max_outflow_rate', 0.5)
        
        # 进水口配置
        self.inflow_rate = 0.0   # 当前进水流速
        self.max_inflow = config.get('max_inflow_rate', 1.0)
        
        # 目标水位 (与课件一致)
        self.target_level = config.get('target_level', 1.0)
        
        # 历史记录
        self.level_history = []
        self.inflow_history = []
        self.outflow_history = []
        self.time_history = []
        
        # 物理参数
        self.dt = config.get('time_step', 0.1)  # 时间步长
        
        # 系统惯性系数 (新增，用于模拟超调和振荡)
        self.inertia = config.get('inertia', 0.5)
    
    def update(self, control_signal, dt=None):
        """
        更新水箱状态
        
        Args:
            control_signal: PID控制信号 (0-1之间)
            dt: 时间步长 (秒)，如果为None则使用默认值
            
        Returns:
            float: 当前水位
        """
        if dt is None:
            dt = self.dt
        
        # 计算进水流速 (限幅)
        target_inflow = max(0, min(control_signal, 1)) * self.max_inflow
        
        # 引入惯性：进水流速不会突变，而是逐渐变化，这样不会即时响应pid输出
        inflow_change = target_inflow - self.inflow_rate
        self.inflow_rate += inflow_change * self.inertia
        
        # 计算净流量 (进水量 - 出水量)
        net_flow = self.inflow_rate - self.outflow_rate
        
        # 计算水位变化 (水位变化 = 净流量 * 时间 / 底面积)
        level_change = (net_flow * dt) / self.area
        
        # 更新水位 (限制在0到最大安全水位之间)
        self.current_level = max(0, min(self.current_level + level_change, self.max_level))
        
        return self.current_level
    
    def record(self, current_time):
        """记录当前状态到历史"""
        self.level_history.append(self.current_level)
        self.inflow_history.append(self.inflow_rate)
        self.outflow_history.append(self.outflow_rate)
        self.time_history.append(current_time)
    
    def get_error(self):
        """计算当前误差"""
        return self.target_level - self.current_level
    
    def set_outflow_rate(self, rate):
        """设置出水流速"""
        self.outflow_rate = min(max(rate, 0), self.max_outflow)
        return self.outflow_rate
    
    def set_target_level(self, target):
        """设置目标水位"""
        self.target_level = max(0, min(target, self.max_level))
        return self.target_level
    
    def reset(self, initial_level=0.0):
        """重置水箱状态"""
        self.current_level = initial_level  # 重置为0
        self.inflow_rate = 0.0
        self.level_history = []
        self.inflow_history = []
        self.outflow_history = []
        self.time_history = []
    
    def get_stats(self):
        """获取当前统计信息"""
        return {
            'current_level': self.current_level,
            'target_level': self.target_level,
            'inflow_rate': self.inflow_rate,
            'outflow_rate': self.outflow_rate,
            'error': self.get_error()
        }
    
    def get_history(self):
        """获取历史数据"""
        return {
            'time': self.time_history,
            'level': self.level_history,
            'inflow': self.inflow_history,
            'outflow': self.outflow_history
        }