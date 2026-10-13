#ifndef WCLOCKFACE_H
#define WCLOCKFACE_H

#include <QColor>
#include <QWidget>

/*
 * Clock face to pick a time with a finger. Hours: 24 hour face, 0-11 on
 * the outer ring and 12-23 on the inner one. Minutes: 00, 10, ... 50 (the
 * chrono works in tens of minutes), or any minute with setMinuteStep(1)
 * (numbers every 5). Tap or drag; picked() on release.
 */
class wClockFace : public QWidget
{
    Q_OBJECT

public:
    enum Mode { Hours, Minutes };

    explicit wClockFace(QWidget *parent = nullptr);

    Mode mode() const { return m_mode; }
    int value() const { return m_value; }  //Hours 0-23, or minutes 0-59
    void setMinuteStep(int step) { m_minuteStep = (step == 1) ? 1 : 10; }

    QSize sizeHint() const override { return QSize(300, 300); }

public slots:
    void setMode(Mode mode, int value);

signals:
    void valueChanged(int value);
    void picked(int value);                 //Finger lifted

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPointF center() const;
    double outerRadius() const;
    double innerRadius() const;
    QPointF position(int value) const;      //Where a value is drawn
    int valueAt(const QPointF &p) const;
    void pick(const QPointF &p);

    Mode m_mode = Hours;
    int m_value = 0;
    int m_minuteStep = 10;
    bool m_pressed = false;
};

#endif // WCLOCKFACE_H
