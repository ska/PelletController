#include "wDial.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#define DIAL_START_DEG  (225.0)     //Bottom left, 0 = 3 o'clock, counterclockwise
#define DIAL_SPAN_DEG   (270.0)     //Clockwise to the bottom right

wDial::wDial(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

void wDial::setMinimum(double v)                { m_minimum = v; update(); }
void wDial::setMaximum(double v)                { m_maximum = v; update(); }
void wDial::setCaption(const QString &caption)  { m_caption = caption; update(); }
void wDial::setTrackColor(const QColor &c)      { m_trackColor = c; update(); }
void wDial::setArcColor(const QColor &c)        { if(c != m_arcColor) { m_arcColor = c; update(); } }
void wDial::setMarkerColor(const QColor &c)     { m_markerColor = c; update(); }
void wDial::setTextColor(const QColor &c)       { m_textColor = c; update(); }
void wDial::setMutedColor(const QColor &c)      { m_mutedColor = c; update(); }
void wDial::setHoleColor(const QColor &c)       { m_holeColor = c; update(); }

/* The stove sends every value at each read: repaint only on a change */
void wDial::setValue(double value)
{
    if(m_hasValue && qFuzzyCompare(value, m_value))
        return;
    m_value = value;
    m_hasValue = true;
    update();
}

void wDial::setSetpoint(double setpoint)
{
    if(m_hasSetpoint && qFuzzyCompare(setpoint, m_setpoint))
        return;
    m_setpoint = setpoint;
    m_hasSetpoint = true;
    update();
}

void wDial::clearValue()
{
    m_hasValue = false;
    update();
}

void wDial::clearSetpoint()
{
    m_hasSetpoint = false;
    update();
}

double wDial::fraction(double v) const
{
    if(m_maximum <= m_minimum)
        return 0.0;
    return qBound(0.0, (v - m_minimum) / (m_maximum - m_minimum), 1.0);
}

void wDial::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    //Square in the middle of the widget; sizes from the 268 px mockup
    const double side = qMin(width(), height());
    const double k = side / 268.0;
    const double stroke = 18.0 * k;
    const double markerR = 11.0 * k;
    const double radius = side / 2.0 - qMax(stroke / 2.0, markerR + 2.0 * k) - 2.0 * k;
    const QPointF c(width() / 2.0, height() / 2.0);
    const QRectF ring(c.x() - radius, c.y() - radius, 2 * radius, 2 * radius);

    QPen pen(m_trackColor, stroke, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    p.drawArc(ring, int(DIAL_START_DEG * 16), int(-DIAL_SPAN_DEG * 16));

    if(m_hasValue)
    {
        const double span = DIAL_SPAN_DEG * fraction(m_value);
        if(span > 0.5)
        {
            pen.setColor(m_arcColor);
            p.setPen(pen);
            p.drawArc(ring, int(DIAL_START_DEG * 16), int(-span * 16));
        }
    }

    if(m_hasSetpoint)
    {
        const double a = qDegreesToRadians(DIAL_START_DEG - DIAL_SPAN_DEG * fraction(m_setpoint));
        const QPointF m(c.x() + radius * qCos(a), c.y() - radius * qSin(a));
        p.setPen(QPen(m_holeColor, 4.0 * k));
        p.setBrush(m_markerColor);
        p.drawEllipse(m, markerR, markerR);
    }

    //Caption, room value, setpoint
    QFont f = font();
    f.setPixelSize(int(16 * k));
    f.setBold(false);
    p.setFont(f);
    const QFontMetricsF fmSmall(f);

    QFont big = font();
    big.setPixelSize(int(76 * k));
    big.setWeight(QFont::Black);
    big.setLetterSpacing(QFont::AbsoluteSpacing, -2.0 * k);
    //The widest value ("88.8") must fit inside the ring with a margin: the
    //system font is wider than the one of the mockup
    const double room = 2.0 * (radius - stroke) * 0.82;
    const double wide = QFontMetricsF(big).horizontalAdvance(QStringLiteral("88.8") + QChar(0x00B0));
    if(wide > room)
        big.setPixelSize(qMax(1, int(big.pixelSize() * room / wide)));
    const QFontMetricsF fmBig(big);

    QFont set = f;
    set.setBold(true);

    const QString value = m_hasValue ? QString::number(m_value, 'f', 1) + QChar(0x00B0) : QStringLiteral("--");
    const QString setText = m_hasSetpoint ? QString("Set %1").arg(QString::number(m_setpoint, 'f', 1)) + QChar(0x00B0)
                                          : QStringLiteral("Set --");

    const double hBig = fmBig.ascent();
    const double total = fmSmall.height() + hBig + fmSmall.height() + 8 * k;
    double y = c.y() - total / 2.0;

    p.setPen(m_mutedColor);
    p.drawText(QRectF(0, y, width(), fmSmall.height()), Qt::AlignHCenter | Qt::AlignVCenter, m_caption);
    y += fmSmall.height() + 4 * k;

    p.setFont(big);
    p.setPen(m_textColor);
    p.drawText(QRectF(0, y - (fmBig.height() - hBig) / 2.0, width(), fmBig.height()),
               Qt::AlignHCenter | Qt::AlignVCenter, value);
    y += hBig + 4 * k;

    p.setFont(set);
    p.setPen(m_markerColor);
    p.drawText(QRectF(0, y, width(), fmSmall.height()), Qt::AlignHCenter | Qt::AlignVCenter, setText);
}
