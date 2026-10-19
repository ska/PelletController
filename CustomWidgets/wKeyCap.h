#ifndef WKEYCAP_H
#define WKEYCAP_H

#include <QAbstractButton>
#include <QColor>
#include <QElapsedTimer>
#include <QMargins>
#include <QString>

class QTimer;

/*
 * Keycap of the key bar: the label of one physical key under the screen,
 * and a touch button on a panel without keys. Lines from the top: a small
 * caption, the icon and the text, a small hint ("hold 2 s").
 *
 * activated() comes from a tap (touch released on the cap, or the key
 * released) or, with a hold time, once the cap has been held that long:
 * a shorter press does nothing, the cap fills while held. A physical key
 * drives the cap with keyPress() / keyRelease(); clicked() is not used.
 */
class wKeyCap : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(QString caption          READ caption            WRITE setCaption)
    Q_PROPERTY(QString hint             READ hint               WRITE setHint)
    Q_PROPERTY(int holdTime             READ holdTime           WRITE setHoldTime)
    Q_PROPERTY(int textPixelSize        READ textPixelSize      WRITE setTextPixelSize)
    Q_PROPERTY(bool dashed              READ dashed             WRITE setDashed)
    Q_PROPERTY(QColor backgroundColor   READ backgroundColor    WRITE setBackgroundColor)
    Q_PROPERTY(QColor borderColor       READ borderColor        WRITE setBorderColor)
    Q_PROPERTY(QColor textColor         READ textColor          WRITE setTextColor)
    Q_PROPERTY(QColor captionColor      READ captionColor       WRITE setCaptionColor)

public:
    explicit wKeyCap(QWidget *parent = nullptr);

    QString caption() const { return m_caption; }
    QString hint() const { return m_hint; }
    int holdTime() const { return m_holdTime; }
    int textPixelSize() const { return m_textPixelSize; }
    bool dashed() const { return m_dashed; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor borderColor() const { return m_borderColor; }
    QColor textColor() const { return m_textColor; }
    QColor captionColor() const { return m_captionColor; }

    void setCaption(const QString &caption);
    void setHint(const QString &hint);
    void setHoldTime(int ms);               //0: tap
    void setTextPixelSize(int px);
    void setDashed(bool dashed);
    void setBackgroundColor(const QColor &c);
    void setBorderColor(const QColor &c);
    void setTextColor(const QColor &c);
    void setCaptionColor(const QColor &c);
    /* Room between the widget and the cap drawn: the whole widget takes
     * touches, so a finger a little off the cap still presses it */
    void setInset(const QMargins &inset);

    bool isHeld() const { return m_held; }

    QSize sizeHint() const override { return QSize(123, 54); }

public slots:
    void keyPress();                        //Physical key down (not the auto repeat)
    void keyRelease();                      //Physical key up
    void cancel();                          //Press dropped, nothing activated

signals:
    void activated();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    bool onCap(const QPoint &pos) const;
    void begin();
    void end(bool activate);
    void tick();
    double progress() const;                //0-1 of the hold time

    QString m_caption;
    QString m_hint;
    int m_holdTime = 0;
    int m_textPixelSize = 17;
    bool m_dashed = false;
    QColor m_backgroundColor = "#22262B";
    QColor m_borderColor = "#3A4047";
    QColor m_textColor = "#F3F4F5";
    QColor m_captionColor = "#A3ABB3";
    QMargins m_inset;

    bool m_held = false;
    QElapsedTimer m_heldTime;
    QTimer *m_tick;
};

#endif // WKEYCAP_H
