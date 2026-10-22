/**
 * @file    traj_planner.h
 * @brief   梯形速度轨迹规划器
 * @author  Dr. GAO
 * @date    2026-07-09
 * @version V1.0
 * @website https://genbotter.taobao.com
 *
 * @note    三阶段梯形速度曲线：加速 → 匀速 → 减速
 *          每个控制周期推一个中间目标给 PID 跟踪，
 *          从根源消除阶跃超调。
 */

#ifndef __TRAJ_PLANNER_H
#define __TRAJ_PLANNER_H

#include "main.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 轨迹阶段
 *============================================================================*/

typedef enum {
    TRAJ_PHASE_ACCEL  = 0,   /**< 加速段 */
    TRAJ_PHASE_CONST  = 1,   /**< 匀速段 */
    TRAJ_PHASE_DECEL  = 2,   /**< 减速段 */
    TRAJ_PHASE_DONE   = 3,   /**< 轨迹完成 */
} TrajPhase_t;

/*==============================================================================
 * 轨迹规划器控制句柄
 *============================================================================*/

typedef struct {
    int32_t start_pos;           /**< 起始位置 (counts) */
    int32_t target_pos;          /**< 终点位置 (counts) */
    int32_t cur_setpoint;        /**< 当前中间目标 (counts)，喂给 PID */

    float   cur_speed;           /**< 当前速度 (counts/s) */
    float   v_max;               /**< 最大速度 (counts/s) */
    float   accel;               /**< 加速度 (counts/s²) */

    uint32_t total_dist;         /**< 本次行程总距离 (counts) */
    uint32_t travelled;          /**< 已走距离 (counts) */

    TrajPhase_t phase;           /**< 当前阶段 */
} TrajPlanner;

/*==============================================================================
 * API
 *============================================================================*/

/** @brief 初始化轨迹规划器 */
void TrajPlanner_Init(TrajPlanner *tp, float v_max, float accel);

/** @brief 启动一段新轨迹（从 from 到 to，自动计算方向） */
void TrajPlanner_Start(TrajPlanner *tp, int32_t from, int32_t to);

/** @brief 推进一步，返回本周期中间目标给 PID
 *  @param dt  采样时间 (s)，一般为 PID 周期 = 0.05 */
int32_t TrajPlanner_Update(TrajPlanner *tp, float dt);

/** @brief 轨迹是否已完成 */
bool TrajPlanner_IsDone(const TrajPlanner *tp);

/** @brief 获取当前进度百分比 (0.0 ~ 100.0) */
float TrajPlanner_GetProgress(const TrajPlanner *tp);

/** @brief 获取当前中间目标 (counts) */
int32_t TrajPlanner_GetSetpoint(const TrajPlanner *tp);

/** @brief 获取当前速度 (counts/s) */
float TrajPlanner_GetSpeed(const TrajPlanner *tp);

/** @brief 获取当前阶段名称（用于调试显示） */
const char* TrajPlanner_PhaseName(TrajPhase_t phase);

#ifdef __cplusplus
}
#endif

#endif /* __TRAJ_PLANNER_H */
