#include "wKeyCap.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#define KEYCAP_TICK_MS  30                  //Repaint of the hold fill

wKeyCap::wKeyCap(QWidget *parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    m_tick = new QTimer(this);
    m_tick->setInterval(KEYCAP_TICK_MS);
    connect(m_tick, &QTimer::timeout, this, &wKeyCap::tick);
}

/* Repaint only on a change */
template<typename T> static bool change(T &member, const T &value)
{
    if(member == value)
        return false;
    member = value;
    return true;
}

void wKeyCap::setCaption(const QString &caption) { if(change(m_caption, caption)) update(); }
void wKeyCap::setHint(const QString &hint) { if(change(m_hint, hint)) update(); }
void wKeyCap::setTextPixelSize(int px) { if(change(m_textPixelSize, px)) update(); }
void wKeyCap::setDashed(bool dashed) { if(change(m_dashed, dashed)) update(); }
void wKeyCap::setBackgroundColor(const QColor &c) { if(change(m_backgroundColor, c)) update(); }
void wKeyCap::setBorderColor(const QColor &c) { if(change(m_borderColor, c)) update(); }
void wKeyCap::setTextColor(const QColor &c) { if(change(m_textColor, c)) update(); }
void wKeyCap::setCaptionColor(const QColor &c) { if(change(m_captionColor, c)) update(); }
void wKeyCap::setInset(const QMargins &inset) { if(change(m_inset, inset)) update(); }

void wKeyCap::setHoldTime(int ms)
{
    cancel();
    m_holdTime = qMax(0, ms);
}

void wKeyCap::keyPress()
{
    begin();
}

void wKeyCap::keyRelease()
{
    end(true);
}

void wKeyCap::cancel()
{
    end(false);
}

void wKeyCap::begin()
{
    if(m_held || !isEnabled())
        return;
    m_held = true;
    setDown(true);
    if(m_holdTime > 0)
    {
        m_heldTime.start();
        m_tick->start();
    }
}

/* Released: a tap activates now; a hold activated in tick() already, here
 * it is only a press too short */
void wKeyCap::end(bool activate)
{
    if(!m_held)
        return;
    m_held = false;
    m_tick->stop();
    setDown(false);
    update();
    if(activate && 0 == m_holdTime)
        emit activated();
}

void wKeyCap::tick()
{
    if(m_held && m_heldTime.elapsed() >= m_holdTime)
    {
        m_held = false;
        m_tick->stop();
        setDown(false);
        update();
        emit activated();
        return;
    }
    update();
}

double wKeyCap::progress() const
{
    if(!m_held || m_holdTime <= 0)
        return 0.0;
    return qBound(0.0, double(m_heldTime.elapsed()) / m_holdTime, 1.0);
}

void wKeyCap::mousePressEvent(QMouseEvent *event)
{
    if(Qt::LeftButton != event->button())
    {
        event->ignore();
        return;
    }
    begin();
    event->accept();
}

/* On the widget, with a 2 px margin: the touch screen reports its last
 * row one pixel below the window */
bool wKeyCap::onCap(const QPoint &pos) const
{
    return rect().adjusted(-2, -2, 2, 2).contains(pos);
}

/* A finger sliding off the cap drops the press */
void wKeyCap::mouseMoveEvent(QMouseEvent *event)
{
    if(m_held && !onCap(event->pos()))
        cancel();
    event->accept();
}

void wKeyCap::mouseReleaseEvent(QMouseEvent *event)
{
    if(Qt::LeftButton != event->button())
    {
        event->ignore();
        return;
    }
    end(onCap(event->pos()));
    event->accept();
}

void wKeyCap::changeEvent(QEvent *event)
{
    if(QEvent::EnabledChange == event->type() && !isEnabled())
        cancel();
    QAbstractButton::changeEvent(event);
}

void wKeyCap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if(!isEnabled())
        p.setOpacity(0.4);

    const QRectF box = QRectF(rect().marginsRemoved(m_inset)).adjusted(0.5, 0.5, -0.5, -0.5);
    const double r = 12.0;

    /* Body: a tap lights it while pressed, a hold fills it from the left
     * with the text color */
    QPen border(m_borderColor, 1.0);
    if(m_dashed)
        border.setStyle(Qt::DashLine);
    p.setPen(border);
    p.setBrush(isDown() && 0 == m_holdTime ? m_backgroundColor.lighter(140) : m_backgroundColor);
    p.drawRoundedRect(box, r, r);
    const double fill = progress();
    if(fill > 0.0)
    {
        QColor c = m_textColor;
        c.setAlphaF(0.3);
        p.save();
        p.setClipRect(QRectF(box.left(), box.top(), box.width() * fill, box.height()));
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(box, r, r);
        p.restore();
    }

    /* Lines: caption, icon + text, hint, centered as a block */
    QFont small = font();
    small.setPixelSize(m_caption.isEmpty() ? 11 : 12);
    QFont big = font();
    big.setPixelSize(m_textPixelSize);
    big.setBold(true);
    const QFontMetrics fmSmall(small);
    const QFontMetrics fmBig(big);
    const bool hasMain = !text().isEmpty() || !icon().isNull();
    //A big glyph ("+") has empty room above and below: tighter lines
    const int hBig = (m_textPixelSize > 22) ? int(m_textPixelSize * 1.05) : fmBig.height();
    int h = 0;
    if(!m_caption.isEmpty())
        h += fmSmall.height();
    if(hasMain)
        h += hBig;
    if(!m_hint.isEmpty())
        h += fmSmall.height();
    double y = box.center().y() - h / 2.0;

    if(!m_caption.isEmpty())
    {
        p.setFont(small);
        p.setPen(m_captionColor);
        p.drawText(QRectF(0, y, width(), fmSmall.height()), Qt::AlignCenter, m_caption);
        y += fmSmall.height();
    }
    if(hasMain)
    {
        const QSize is = icon().isNull() ? QSize() : iconSize();
        const int gap = is.isValid() && !text().isEmpty() ? 6 : 0;
        const int tw = fmBig.horizontalAdvance(text());
        const double x = (width() - (is.isValid() ? is.width() : 0) - gap - tw) / 2.0;
        if(is.isValid())
            icon().paint(&p, QRect(int(x), int(y + (hBig - is.height()) / 2.0), is.width(), is.height()));
        p.setFont(big);
        p.setPen(m_textColor);
        p.drawText(QRectF(x + (is.isValid() ? is.width() : 0) + gap, y - (fmBig.height() - hBig) / 2.0,
                          tw + 2, fmBig.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, text());
        y += hBig;
    }
    if(!m_hint.isEmpty())
    {
        p.setFont(small);
        p.setPen(m_captionColor);
        p.drawText(QRectF(0, y, width(), fmSmall.height()), Qt::AlignCenter, m_hint);
    }
}
