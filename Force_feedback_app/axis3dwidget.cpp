#include "axis3dwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

Axis3DWidget::Axis3DWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
}

void Axis3DWidget::setPosition(double x, double y, double z)
{
    m_posX = x; m_posY = y; m_posZ = z;
    update();   // 请求重绘 → paintEvent 会被 Qt 调用
}

// ── 投影：世界坐标 → 屏幕坐标（带透视：近大远小）──
QPointF Axis3DWidget::project(double x, double y, double z) const
{
    const double cy = qCos(m_yaw),   sy = qSin(m_yaw);
    const double cp = qCos(m_pitch), sp = qSin(m_pitch);

    // 轴约定：X 为横轴（右），Z 为纵轴（上），Y 为纵深（里外）
    const double xi = x;
    const double yi = z;
    const double zi = m_flipY ? -y : y;   // 内在翻转：Y 正值落在实线侧，轴图形不变

    // 1) 绕 Y 轴旋转（左右转视角）
    double x1 =  xi * cy + zi * sy;
    double z1 = -xi * sy + zi * cy;
    double y1 =  yi;
    // 2) 绕 X 轴旋转（上下转视角）
    double y2 = y1 * cp - z1 * sp;
    double z2 = y1 * sp + z1 * cp;   // 进深：>0 靠近观察者
    // 3) 透视投影：越近（z2 越大）放大越多，实现近大远小
    const double scale = m_zoom * (qMin(width(), height()) * 0.32);
    const double pf = 1.0 / (1.0 - m_persp * z2);
    return QPointF(width() / 2.0 + x1 * pf * scale,
                   height() / 2.0 - y2 * pf * scale);
}

// ── 视角旋转后的进深 z2（>0 靠近观察者）──
double Axis3DWidget::viewDepth(double x, double y, double z) const
{
    const double cy = qCos(m_yaw),   sy = qSin(m_yaw);
    const double cp = qCos(m_pitch), sp = qSin(m_pitch);
    const double xi = x, yi = z, zi = m_flipY ? -y : y;
    const double z1 = -xi * sy + zi * cy;
    const double y1 = yi;
    return y1 * sp + z1 * cp;
}

void Axis3DWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 背景：深色底更有空间感
    p.fillRect(rect(), QColor(18, 24, 38));

    // 归一化：把真实数值压到 [-1,1]，超出范围就截断
    auto norm = [this](double v) { return qBound(-1.0, v / m_range, 1.0); };
    const double nx = norm(m_posX);
    const double ny = norm(m_posY);
    const double nz = norm(m_posZ);

    // ── XY 平面网格（Z=0 的地面），增强空间感 ──
    QPen gridPen(QColor(255, 255, 255, 40));
    gridPen.setWidth(1);
    p.setPen(gridPen);
    for (int i = -3; i <= 3; ++i) {
        const double t = i / 3.0;
        p.drawLine(project(-1, t, 0), project(1, t, 0));
        p.drawLine(project(t, -1, 0), project(t, 1, 0));
    }

    // ── 坐标轴：X=红（横） Y=绿（纵深） Z=蓝（纵）──
    // solidAtPos=true：正半轴实线、字母在正端；false：图案颠倒（负半轴实线、字母在负端），正负含义不变
    struct Axis { QColor color; QPointF pos; QPointF neg; QString name; bool solidAtPos; };
    const Axis axes[3] = {
        { QColor(255, 80, 80),   project(1.2, 0, 0), project(-1.1, 0, 0),  "X", true },
        { QColor(80, 255, 80),   project(0, 1.2, 0), project(0, -1.1, 0),  "Y", true },
        { QColor(80, 140, 255),  project(0, 0, 1.2), project(0, 0, -1.1),  "Z", true },
    };
    QFont axisFont = p.font();
    axisFont.setBold(true);
    axisFont.setPointSize(11);
    for (const Axis &a : axes) {
        if (a.solidAtPos) {
            p.setPen(QPen(a.color, 2));
            p.drawLine(project(0, 0, 0), a.pos);             // 正半轴：实线
            p.setPen(QPen(a.color, 1, Qt::DashLine));
            p.drawLine(project(0, 0, 0), a.neg);             // 负半轴：虚线
            p.setPen(a.color);
            p.setFont(axisFont);
            p.drawText(a.pos + QPointF(4, -4), a.name);      // 字母在正端
        } else {
            p.setPen(QPen(a.color, 1, Qt::DashLine));
            p.drawLine(project(0, 0, 0), a.pos);             // 正半轴：虚线（图案颠倒）
            p.setPen(QPen(a.color, 2));
            p.drawLine(project(0, 0, 0), a.neg);             // 负半轴：实线
            p.setPen(a.color);
            p.setFont(axisFont);
            p.drawText(a.neg + QPointF(4, -4), a.name);      // 字母在负端
        }
    }

    // ── 位置箭头：指向 = Position 与固定点 (0,0,24.6) 连线延长线的方向（背离该固定点）──
    // 固定点是设备坐标参考点，要改就在下面一行改
    const double anchorX = 0.0, anchorY = 0.0, anchorZ = 24.6;
    const QPointF pos = project(nx, ny, nz);
    // 从固定点 (0,0,24.6) 连一条虚线到点，帮助看出它的落点
    p.setPen(QPen(QColor(255, 255, 255, 120), 1, Qt::DashLine));
    p.drawLine(project(anchorX / m_range, anchorY / m_range, anchorZ / m_range), pos);

    // 箭头方向 = Position - 固定点(0,0,24.6)，即从固定点射向 Position 并继续延伸
    // 归一化三轴同除 m_range，方向不变
    double dx = m_posX - anchorX;
    double dy = m_posY - anchorY;
    double dz = m_posZ - anchorZ;
    const double vlen = qSqrt(dx*dx + dy*dy + dz*dz);
    if (vlen < 0.05) {
        // 位置接近固定点：默认朝下（世界 -Z，默认视角下即屏幕下方）
        dx = 0.0; dy = 0.0; dz = -1.0;
    } else {
        dx /= vlen; dy /= vlen; dz /= vlen;
    }

    // 画箭头：三维长度固定，两端都走透视投影 → 自动有近大远小 + 透视缩短
    const double dirLen = 0.20;   // 箭头三维长度（归一化单位，约等于原来 16px）
    const QPointF tip  = pos;                           // 尖点 = 位置点
    const QPointF tail = project(nx - dx*dirLen, ny - dy*dirLen, nz - dz*dirLen);

    QPointF shaft = tip - tail;                          // 箭身在屏幕上的方向与长度
    const double shaftLen = qSqrt(shaft.x()*shaft.x() + shaft.y()*shaft.y());
    if (shaftLen < 2.0) {
        // 箭头几乎正对观察者（透视缩短成一点）：画个小圆点表示
        p.setPen(QPen(QColor(255, 200, 40), 2));
        p.setBrush(QColor(255, 200, 40, 200));
        p.drawEllipse(tip, 3, 3);
    } else {
        shaft /= shaftLen;                               // 单位屏幕方向
        const QPointF n2d(-shaft.y(), shaft.x());        // 垂直方向
        const double headLen = 0.35 * shaftLen;          // 箭头头随箭身长度比例

        p.setPen(QPen(QColor(255, 200, 40), 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawLine(tail, tip);
        p.drawLine(tip, tip - shaft*headLen + n2d*(headLen*0.6));
        p.drawLine(tip, tip - shaft*headLen - n2d*(headLen*0.6));
    }

    // 坐标文字
    p.setPen(QColor(255, 255, 255));
    p.drawText(pos + QPointF(12, -12),
               QString("X:%1  Y:%2  Z:%3")
                   .arg(m_posX, 0, 'f', 1)
                   .arg(m_posY, 0, 'f', 1)
                   .arg(m_posZ, 0, 'f', 1));
}

// ── 鼠标拖动旋转视角 ──
void Axis3DWidget::mousePressEvent(QMouseEvent *e)
{
    m_lastDrag = e->pos();
    m_dragging = true;
}

void Axis3DWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_dragging) return;
    m_yaw   += (e->pos().x() - m_lastDrag.x()) * 0.01;  // 横向拖 → 绕Y转
    m_pitch += (e->pos().y() - m_lastDrag.y()) * 0.01;  // 纵向拖 → 绕X转
    m_pitch = qBound(-1.5, m_pitch, 1.5);               // 限制俯仰，防翻转
    m_lastDrag = e->pos();
    update();
}

void Axis3DWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

void Axis3DWidget::wheelEvent(QWheelEvent *e)
{
    m_zoom *= (e->angleDelta().y() > 0) ? 1.1 : 0.9;
    m_zoom = qBound(0.3, m_zoom, 4.0);
    update();
}
