/**
 * @file    traj_planner.c
 * @brief   梯形速度轨迹规划器实现
 * @author  Dr. GAO
 * @date    2026-07-09
 * @version V1.0
 * @website https://genbotter.taobao.com
 *
 * @note    梯形速度曲线（加速 → 匀速 → 减速）：
 *          每个控制周期输出一个中间目标给 PID 跟踪。
 *
 *          关键公式：
 *          位移 S  = v_max * (t_const + t_accel_decel) （梯形面积）
 *          减速距离 = v² / (2a)（保证正好到终点速度归零）
 */

#include "traj_planner.h"

/*==============================================================================
 * 初始化
 *============================================================================*/

void TrajPlanner_Init(TrajPlanner *tp, float v_max, float accel)
{
    if (!tp) return;

    tp->start_pos    = 0;
    tp->target_pos   = 0;
    tp->cur_setpoint = 0;
    tp->cur_speed    = 0.0f;
    tp->v_max        = v_max;
    tp->accel        = accel;
    tp->total_dist   = 0;
    tp->travelled    = 0;
    tp->phase        = TRAJ_PHASE_DONE;
}

/*==============================================================================
 * 启动新轨迹
 *============================================================================*/

void TrajPlanner_Start(TrajPlanner *tp, int32_t from, int32_t to)
{
    if (!tp) return;

    tp->start_pos  = from;
    tp->target_pos = to;
    tp->total_dist = (uint32_t)((to > from) ? (to - from) : (from - to));
    tp->travelled  = 0;

    tp->cur_setpoint = from;
    tp->cur_speed    = 0.0f;

    /* 极短行程：不规划，直接到位 */
    if (tp->total_dist == 0)
    {
        tp->phase = TRAJ_PHASE_DONE;
        tp->cur_setpoint = to;
        return;
    }

    tp->phase = TRAJ_PHASE_ACCEL;
}

/*==============================================================================
 * 推进一步（每个控制周期调用一次）
 *============================================================================*/

int32_t TrajPlanner_Update(TrajPlanner *tp, float dt)
{
    if (!tp || tp->phase == TRAJ_PHASE_DONE) return tp->cur_setpoint;

    int32_t direction = (tp->target_pos >= tp->start_pos) ? 1 : -1;
    uint32_t remaining = tp->total_dist - tp->travelled;

    switch (tp->phase)
    {
    case TRAJ_PHASE_ACCEL:
        /* 加速：v += a·dt */
        tp->cur_speed += tp->accel * dt;

        /* 到最大速度 → 进入匀速 */
        if (tp->cur_speed >= tp->v_max)
        {
            tp->cur_speed = tp->v_max;
        }

        /* 预判切换点：
         * 如果剩余距离 ≤ 当前速度减速到 0 所需距离(v²/2a)，
         * 立即进减速段——多跑一帧都刹不住。
         * 短行程时会在加速中途直接跳过匀速段。 */
        {
            float decel_dist = (tp->cur_speed * tp->cur_speed) / (2.0f * tp->accel);
            if ((float)remaining <= decel_dist)
            {
                tp->phase = TRAJ_PHASE_DECEL;
            }
            else if (tp->cur_speed >= tp->v_max)
            {
                tp->phase = TRAJ_PHASE_CONST;
            }
        }
        break;

    case TRAJ_PHASE_CONST:
        /* 匀速：v = v_max，判断是否需要减速 */
        {
            float decel_dist = (tp->v_max * tp->v_max) / (2.0f * tp->accel);
            if ((float)remaining <= decel_dist)
            {
                tp->phase = TRAJ_PHASE_DECEL;
            }
        }
        break;

    case TRAJ_PHASE_DECEL:
        /* 减速：v -= a·dt */
        tp->cur_speed -= tp->accel * dt;
        if (tp->cur_speed < 0.0f) tp->cur_speed = 0.0f;
        break;

    case TRAJ_PHASE_DONE:
        /* 函数入口已 return，此分支不可达，仅为消除 -Wswitch */
        break;
    }

    /* 更新中间目标：位移 = 速度 × 时间 */
    float step = tp->cur_speed * dt;
    tp->cur_setpoint += (int32_t)(step * (float)direction);
    tp->travelled    += (uint32_t)step;

    /* 到位判断：超目标 或 减速到零 */
    if ((direction > 0 && tp->cur_setpoint >= tp->target_pos) ||
        (direction < 0 && tp->cur_setpoint <= tp->target_pos) ||
        (tp->phase == TRAJ_PHASE_DECEL && tp->cur_speed <= 0.0f))
    {
        tp->cur_setpoint = tp->target_pos;
        tp->cur_speed    = 0.0f;
        tp->travelled    = tp->total_dist;
        tp->phase        = TRAJ_PHASE_DONE;
    }

    return tp->cur_setpoint;
}

/*==============================================================================
 * 状态查询
 *============================================================================*/

bool TrajPlanner_IsDone(const TrajPlanner *tp)
{
    return (tp == NULL) || (tp->phase == TRAJ_PHASE_DONE);
}

float TrajPlanner_GetProgress(const TrajPlanner *tp)
{
    if (!tp || tp->total_dist == 0) return 100.0f;
    return (float)tp->travelled / (float)tp->total_dist * 100.0f;
}

int32_t TrajPlanner_GetSetpoint(const TrajPlanner *tp)
{
    return tp ? tp->cur_setpoint : 0;
}

float TrajPlanner_GetSpeed(const TrajPlanner *tp)
{
    return tp ? tp->cur_speed : 0.0f;
}

const char* TrajPlanner_PhaseName(TrajPhase_t phase)
{
    switch (phase)
    {
        case TRAJ_PHASE_ACCEL:  return "ACCEL";
        case TRAJ_PHASE_CONST:  return "CONST";
        case TRAJ_PHASE_DECEL:  return "DECEL";
        case TRAJ_PHASE_DONE:   return "DONE";
        default:                return "?";
    }
}
