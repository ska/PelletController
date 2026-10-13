#ifndef WPOWERBARS_H
#define WPOWERBARS_H

#include <QColor>
#include <QWidget>

/*
 * Power level as rising bars: maximum bars, the first level filled.
 * Colors are properties for the style sheet (qproperty-activeColor, ...).
 */
class wPowerBars : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int level            READ level          WRITE setLevel)
    Q_PROPERTY(int maximum          READ maximum        WRITE setMaximum)
    Q_PROPERTY(QColor activeColor   READ activeColor    WRITE setActiveColor)
    Q_PROPERTY(QColor inactiveColor READ inactiveColor  WRITE setInactiveColor)

public:
    explicit wPowerBars(QWidget *parent = nullptr);

    int level() const { return m_level; }
    int maximum() const { return m_maximum; }
    QColor activeColor() const { return m_activeColor; }
    QColor inactiveColor() const { return m_inactiveColor; }

    void setMaximum(int maximum);
    void setActiveColor(const QColor &c);
    void setInactiveColor(const QColor &c);

    QSize sizeHint() const override { return QSize(150, 100); }

public slots:
    void setLevel(int level);               //0 = none filled

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_level = 0;
    int m_maximum = 5;
    QColor m_activeColor = "#F28C28";
    QColor m_inactiveColor = "#343A41";
};

#endif // WPOWERBARS_H
