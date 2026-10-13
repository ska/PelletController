#include "wPowerBars.h"

#include <QPainter>

wPowerBars::wPowerBars(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void wPowerBars::setLevel(int level)
{
    m_level = qBound(0, level, m_maximum);
    update();
}

void wPowerBars::setMaximum(int maximum)
{
    m_maximum = qMax(1, maximum);
    m_level = qMin(m_level, m_maximum);
    update();
}

void wPowerBars::setActiveColor(const QColor &c)    { m_activeColor = c; update(); }
void wPowerBars::setInactiveColor(const QColor &c)  { m_inactiveColor = c; update(); }

void wPowerBars::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    //Bars as in the mockup: 22 px wide, 8 px apart, from 30 % to 100 % high
    const double gap = 8.0;
    const double barW = qMin(22.0, (width() - gap * (m_maximum - 1)) / m_maximum);
    const double total = barW * m_maximum + gap * (m_maximum - 1);
    double x = (width() - total) / 2.0;
    const double radius = barW * 5.0 / 22.0;

    for(int i = 0; i < m_maximum; i++)
    {
        const double h = height() * (0.3 + 0.7 * i / qMax(1, m_maximum - 1));
        p.setBrush(i < m_level ? m_activeColor : m_inactiveColor);
        p.drawRoundedRect(QRectF(x, height() - h, barW, h), radius, radius);
        x += barW + gap;
    }
}
