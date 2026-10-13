#include "wClockFace.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

namespace {
const QColor kFace("#2A2F35");
const QColor kText("#F3F4F5");
const QColor kMuted("#A3ABB3");
const QColor kAccent("#7DB7FF");
const QColor kOnAccent("#0B1118");
const double kNumberRadius = 21.0;          //Circle behind a number
}

wClockFace::wClockFace(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(240, 240);
}

void wClockFace::setMode(Mode mode, int value)
{
    m_mode = mode;
    m_value = (Hours == mode) ? qBound(0, value, 23)
                              : qBound(0, (value / m_minuteStep) * m_minuteStep, 60 - m_minuteStep);
    update();
}

QPointF wClockFace::center() const
{
    return QPointF(width() / 2.0, height() / 2.0);
}

double wClockFace::outerRadius() const
{
    return qMin(width(), height()) / 2.0 - kNumberRadius - 6.0;
}

double wClockFace::innerRadius() const
{
    return outerRadius() - 2.0 * kNumberRadius - 6.0;
}

/* 0 (or 12, or 00) at the top, clockwise */
QPointF wClockFace::position(int value) const
{
    double r;
    double step;
    if(Hours == m_mode)
    {
        r = (value >= 12) ? innerRadius() : outerRadius();
        step = (value % 12) * 30.0;
    } else {
        r = outerRadius();
        step = value * 6.0;
    }
    const double a = qDegreesToRadians(step - 90.0);
    return center() + QPointF(r * qCos(a), r * qSin(a));
}

int wClockFace::valueAt(const QPointF &p) const
{
    const QPointF d = p - center();
    double deg = qRadiansToDegrees(qAtan2(d.y(), d.x())) + 90.0;
    if(deg < 0)
        deg += 360.0;
    if(Hours == m_mode)
    {
        const int h = qRound(deg / 30.0) % 12;
        const double dist = qSqrt(d.x() * d.x() + d.y() * d.y());
        const bool inner = dist < (outerRadius() + innerRadius()) / 2.0;
        return inner ? h + 12 : h;
    }
    const int perStep = 6 * m_minuteStep;  //Degrees per selectable minute
    return (qRound(deg / perStep) % (60 / m_minuteStep)) * m_minuteStep;
}

void wClockFace::pick(const QPointF &p)
{
    const int v = valueAt(p);
    if(v == m_value)
        return;
    m_value = v;
    update();
    emit valueChanged(v);
}

void wClockFace::mousePressEvent(QMouseEvent *event)
{
    m_pressed = true;
    pick(event->localPos());
}

void wClockFace::mouseMoveEvent(QMouseEvent *event)
{
    if(m_pressed)
        pick(event->localPos());
}

void wClockFace::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_pressed)
        return;
    m_pressed = false;
    pick(event->localPos());
    emit picked(m_value);
}

void wClockFace::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF c = center();
    const double faceR = qMin(width(), height()) / 2.0 - 2.0;
    p.setPen(Qt::NoPen);
    p.setBrush(kFace);
    p.drawEllipse(c, faceR, faceR);

    //Hand and selection
    const QPointF sel = position(m_value);
    p.setPen(QPen(kAccent, 3.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(c, sel);
    p.setPen(Qt::NoPen);
    p.setBrush(kAccent);
    p.drawEllipse(c, 5.0, 5.0);
    p.drawEllipse(sel, kNumberRadius, kNumberRadius);

    //Numbers
    QFont outer = font();
    outer.setPixelSize(20);
    outer.setBold(true);
    QFont inner = font();
    inner.setPixelSize(15);

    //Numbers: 24 hours, or minutes every 10 (chrono) or every 5 (clock)
    const int labelStep = (10 == m_minuteStep) ? 10 : 5;
    const int count = (Hours == m_mode) ? 24 : 60 / labelStep;
    for(int i = 0; i < count; i++)
    {
        const int v = (Hours == m_mode) ? i : i * labelStep;
        const QPointF pos = position(v);
        const QRectF box(pos.x() - kNumberRadius, pos.y() - kNumberRadius, 2 * kNumberRadius, 2 * kNumberRadius);
        const bool isInner = (Hours == m_mode) && v >= 12;
        p.setFont(isInner ? inner : outer);
        p.setPen(v == m_value ? kOnAccent : (isInner ? kMuted : kText));
        p.drawText(box, Qt::AlignCenter, (Hours == m_mode) ? QString::number(v)
                                                           : QString("%1").arg(v, 2, 10, QChar('0')));
    }
    //A minute between two numbers: its value in the selection circle
    if(Minutes == m_mode && m_value % labelStep)
    {
        p.setFont(outer);
        p.setPen(kOnAccent);
        p.drawText(QRectF(sel.x() - kNumberRadius, sel.y() - kNumberRadius, 2 * kNumberRadius, 2 * kNumberRadius),
                   Qt::AlignCenter, QString("%1").arg(m_value, 2, 10, QChar('0')));
    }
}
