#ifndef WTOGGLE_H
#define WTOGGLE_H

#include <QAbstractButton>
#include <QColor>

/*
 * On/off switch (64 x 36, round knob). Checkable button: clicked(bool)
 * comes only from the user, setChecked() from the values read does not
 * emit it, so a value read is never written back.
 */
class wToggle : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(QColor onColor   READ onColor    WRITE setOnColor)
    Q_PROPERTY(QColor offColor  READ offColor   WRITE setOffColor)

public:
    explicit wToggle(QWidget *parent = nullptr);

    QColor onColor() const { return m_onColor; }
    QColor offColor() const { return m_offColor; }
    void setOnColor(const QColor &c) { m_onColor = c; update(); }
    void setOffColor(const QColor &c) { m_offColor = c; update(); }

    QSize sizeHint() const override { return QSize(64, 36); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_onColor = "#4FB3A9";
    QColor m_offColor = "#343A41";
};

#endif // WTOGGLE_H
