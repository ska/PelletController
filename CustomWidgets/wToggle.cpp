#include "wToggle.h"

#include <QPainter>

wToggle::wToggle(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());
}

void wToggle::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    if(!isEnabled())
        p.setOpacity(0.45);

    const QRectF track = rect();
    const double r = track.height() / 2.0;
    p.setBrush(isChecked() ? m_onColor : m_offColor);
    p.drawRoundedRect(track, r, r);

    const double knob = track.height() - 6.0;
    const double x = isChecked() ? track.width() - knob - 3.0 : 3.0;
    p.setBrush(isChecked() ? QColor("#F3F4F5") : QColor("#7D858D"));
    p.drawEllipse(QRectF(x, 3.0, knob, knob));
}
