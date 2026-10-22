# -*- coding: utf-8 -*-
"""
仿真引擎
管理仿真运行、数据记录和性能计算
"""

import time
import threading
from typing import Callable, Optional

class SimulationEngine:
    """仿真引擎"""
    
    def __init__(self, tank, pid, time_step=0.1, max_time=120.0):  # 最大仿真时间改为120秒
        """
        初始化仿真引擎
        
        Args:
            tank: 水箱模型实例
            pid: PID控制器实例
            time_step: 时间步长 (秒)
            max_time: 最大仿真时间 (秒) - 改为120秒
        """
        self.tank = tank
        self.pid = pid
        
        # 仿真状态
        self.is_running = False
        self.is_paused = False
        self.speed_factor = 1.0  # 仿真速度因子
        self.max_time_reached = False  # 新增：标记是否达到最大时间
        
        # 仿真时间
        self.current_time = 0.0
        self.time_step = time_step
        self.max_time = max_time  # 最大仿真时间
        
        # 性能指标
        self.performance_stats = {
            'rise_time': None,      # 上升时间
            'settling_time': None,  # 调节时间
            'overshoot': None,      # 超调量
            'steady_state_error': None  # 稳态误差
        }
        
        # 线程控制
        self._stop_event = threading.Event()
        self._simulation_thread = None
        
        # 回调函数
        self.data_callback = None
        self.status_callback = None
        
        # 数据记录
        self.simulation_data = []
    
    def start(self):
        """启动仿真"""
        if self.is_running:
            return
        
        self.is_running = True
        self.is_paused = False
        self.max_time_reached = False  # 重置标记
        self._stop_event.clear()
        
        # 创建并启动仿真线程
        self._simulation_thread = threading.Thread(target=self._run_simulation, daemon=True)
        self._simulation_thread.start()
        
        # 通知状态变化
        if self.status_callback:
            self.status_callback('started')
    
    def pause(self):
        """暂停仿真"""
        if self.is_running and not self.is_paused:
            self.is_paused = True
            if self.status_callback:
                self.status_callback('paused')
    
    def resume(self):
        """恢复仿真"""
        if self.is_running and self.is_paused:
            self.is_paused = False
            if self.status_callback:
                self.status_callback('resumed')
    
    def stop(self):
        """停止仿真"""
        self.is_running = False
        self._stop_event.set()
        if self._simulation_thread:
            self._simulation_thread.join(timeout=1.0)
        
        if self.status_callback:
            self.status_callback('stopped')
    
    def reset(self):
        """重置仿真"""
        self.stop()
        self.current_time = 0.0
        self.max_time_reached = False
        self.tank.reset()
        self.pid.reset()
        self.simulation_data = []
        self.performance_stats = {
            'rise_time': None,
            'settling_time': None,
            'overshoot': None,
            'steady_state_error': None
        }
        
        if self.status_callback:
            self.status_callback('reset')
    
    def set_speed(self, factor):
        """设置仿真速度"""
        self.speed_factor = max(0.1, min(factor, 10.0))  # 限制在0.1-10倍速之间
    
    def set_time_step(self, dt):
        """设置时间步长"""
        self.time_step = max(0.01, min(dt, 1.0))  # 限制在0.01-1秒之间
    
    def set_max_time(self, max_time):
        """设置最大仿真时间"""
        self.max_time = max(1.0, min(max_time, 300.0))  # 限制在1-300秒之间
    
    def _run_simulation(self):
        """仿真线程主循环"""
        last_update_time = time.time()
        
        while self.is_running and not self._stop_event.is_set():
            # 检查是否暂停
            if self.is_paused:
                time.sleep(0.1)
                continue
            
            # 检查是否达到最大仿真时间
            if self.current_time >= self.max_time:
                self.max_time_reached = True
                self.stop()
                if self.status_callback:
                    self.status_callback('max_time_reached')
                break
            
            try:
                # 计算误差
                error = self.tank.get_error()
                
                # PID计算控制信号（传入当前水位用于微分计算）
                control_signal = self.pid.calculate(error, self.time_step, 
                                                current_level=self.tank.current_level)
                
                # 更新水箱状态
                self.tank.update(control_signal, self.time_step)
                
                # 记录数据
                self.tank.record(self.current_time)
                
                # 更新性能指标
                self._update_performance_stats()
                
                # 准备回调数据
                data = {
                    'time': self.current_time,
                    'level': self.tank.current_level,
                    'target': self.tank.target_level,
                    'error': error,
                    'control': control_signal,  # 控制信号 (0-1)
                    'inflow': self.tank.inflow_rate,  # 进水流速 (m³/s)
                    'outflow': self.tank.outflow_rate,
                    'performance': self.performance_stats.copy(),
                    'max_time': self.max_time,  # 添加最大时间信息
                    'max_time_reached': self.max_time_reached  # 添加是否达到最大时间的标记
                }
                
                # 添加PID各项数据
                pid_terms = self.pid.get_current_terms()
                data.update(pid_terms)
                
                # 保存数据
                self.simulation_data.append(data)
                
                # 调用数据回调
                if self.data_callback:
                    self.data_callback(data)
                
                # 更新时间
                self.current_time += self.time_step
                
                # 控制仿真速度
                current_time = time.time()
                elapsed = current_time - last_update_time
                target_elapsed = self.time_step / self.speed_factor
                
                if elapsed < target_elapsed:
                    time.sleep(target_elapsed - elapsed)
                
                last_update_time = time.time()
                
            except Exception as e:
                print(f"仿真错误: {e}")
                if self.status_callback:
                    self.status_callback(f'error: {str(e)}')
                break
    
    def _update_performance_stats(self):
        """更新性能指标"""
        # 获取历史数据
        history = self.tank.get_history()
        if not history['time']:
            return
        
        times = history['time']
        levels = history['level']
        target = self.tank.target_level
        
        # 计算稳态误差 (最后10%的数据)
        if len(levels) > 10:
            start_idx = int(len(levels) * 0.9)
            recent_errors = [target - level for level in levels[start_idx:]]
            self.performance_stats['steady_state_error'] = sum(recent_errors) / len(recent_errors)
        
        # 计算超调量
        if len(levels) > 0:
            max_level = max(levels)
            if max_level > target:
                self.performance_stats['overshoot'] = (max_level - target) / target * 100  # 百分比
            else:
                self.performance_stats['overshoot'] = 0.0
        
        # 计算上升时间 (从10%到90%目标值)
        if len(times) > 0 and len(levels) > 0:
            target_10 = target * 0.1
            target_90 = target * 0.9
            
            time_10 = None
            time_90 = None
            
            for t, level in zip(times, levels):
                if time_10 is None and level >= target_10:
                    time_10 = t
                if time_90 is None and level >= target_90:
                    time_90 = t
                    break
            
            if time_10 is not None and time_90 is not None:
                self.performance_stats['rise_time'] = time_90 - time_10
    
    def get_performance_stats(self):
        """获取性能指标"""
        return self.performance_stats.copy()
    
    def get_simulation_time_info(self):
        """获取仿真时间信息"""
        return {
            'current_time': self.current_time,
            'max_time': self.max_time,
            'progress': min(self.current_time / self.max_time * 100, 100) if self.max_time > 0 else 0,
            'max_time_reached': self.max_time_reached
        }
    
    def set_data_callback(self, callback: Callable):
        """设置数据回调函数"""
        self.data_callback = callback
    
    def set_status_callback(self, callback: Callable):
        """设置状态回调函数"""
        self.status_callback = callback
    
    def get_simulation_data(self):
        """获取仿真数据"""
        return self.simulation_data.copy()
    
    def is_complete(self):
        """检查仿真是否完成"""
        return not self.is_running and (self.current_time >= self.max_time or self.max_time_reached)