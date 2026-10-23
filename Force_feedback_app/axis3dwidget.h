#ifndef AXIS3DWIDGET_H
#define AXIS3DWIDGET_H

#include <QWidget>
#include <QPoint>

class QMouseEvent;
class QWheelEvent;

// 手绘简易三维坐标空间控件
class Axis3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Axis3DWidget(QWidget *parent = nullptr);

    // 外部喂数据的接口：把 Position 的三个值传进来，自动重绘
    void setPosition(double x, double y, double z);

protected:
    void paintEvent(QPaintEvent *event) override;          // 所有绘制都在这
    void mousePressEvent(QMouseEvent *event) override;     // 按下：开始拖动视角
    void mouseMoveEvent(QMouseEvent *event) override;      // 拖动：旋转视角
    void mouseReleaseEvent(QMouseEvent *event) override;   // 松开
    void wheelEvent(QWheelEvent *event) override;          // 滚轮：缩放

private:
    // 核心：3D 坐标 (x,y,z) → 2D 屏幕坐标
    QPointF project(double x, double y, double z) const;

    // 视角旋转后的进深 z2（>0 靠近观察者，用于近大远小）
    double viewDepth(double x, double y, double z) const;

    double m_posX = 0.0, m_posY = 0.0, m_posZ = 0.0; // 当前位置
    bool m_flipY = true;   // Y 方向翻转：正 Y（正半轴实线）投影到屏幕远端；图形与数据一致
    double m_range = 25.0;  // 坐标范围 ±range，数值单位不同就改这里
    double m_yaw = 0.7;      // 观察角：绕Y轴（弧度）
    double m_pitch = 0.45;   // 观察角：绕X轴
    double m_zoom = 1.0;     // 缩放系数
    double m_persp = 0.25;   // 透视强度（近大远小）：0 关闭，建议 0.15~0.35
    QPoint m_lastDrag;       // 拖动记录点
    bool m_dragging = false;
};

#endif // AXIS3DWIDGET_H
