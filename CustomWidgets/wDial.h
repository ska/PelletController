#ifndef WDIAL_H
#define WDIAL_H

#include <QColor>
#include <QString>
#include <QWidget>

/*
 * Temperature dial: a 270 degree arc from minimum to maximum (5-30 C), the
 * room temperature as a filled arc, the setpoint as a round marker, and in
 * the middle the caption, the room value and the setpoint text.
 *
 * Colors are properties, so the style sheet sets them per state:
 *   wDial { qproperty-arcColor: #F28C28; }
 *   wDial[state="off"] { qproperty-arcColor: #7D858D; }
 */
class wDial : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double minimum       READ minimum        WRITE setMinimum)
    Q_PROPERTY(double maximum       READ maximum        WRITE setMaximum)
    Q_PROPERTY(QString caption      READ caption        WRITE setCaption)
    Q_PROPERTY(QColor trackColor    READ trackColor     WRITE setTrackColor)
    Q_PROPERTY(QColor arcColor      READ arcColor       WRITE setArcColor)
    Q_PROPERTY(QColor markerColor   READ markerColor    WRITE setMarkerColor)
    Q_PROPERTY(QColor textColor     READ textColor      WRITE setTextColor)
    Q_PROPERTY(QColor mutedColor    READ mutedColor     WRITE setMutedColor)
    Q_PROPERTY(QColor holeColor     READ holeColor      WRITE setHoleColor)

public:
    explicit wDial(QWidget *parent = nullptr);

    double minimum() const { return m_minimum; }
    double maximum() const { return m_maximum; }
    QString caption() const { return m_caption; }
    QColor trackColor() const { return m_trackColor; }
    QColor arcColor() const { return m_arcColor; }
    QColor markerColor() const { return m_markerColor; }
    QColor textColor() const { return m_textColor; }
    QColor mutedColor() const { return m_mutedColor; }
    QColor holeColor() const { return m_holeColor; }

    void setMinimum(double v);
    void setMaximum(double v);
    void setCaption(const QString &caption);
    void setTrackColor(const QColor &c);
    void setArcColor(const QColor &c);
    void setMarkerColor(const QColor &c);
    void setTextColor(const QColor &c);
    void setMutedColor(const QColor &c);
    void setHoleColor(const QColor &c);

    QSize sizeHint() const override { return QSize(268, 268); }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override { return w; }

public slots:
    void setValue(double value);            //Room temperature
    void setSetpoint(double setpoint);
    void clearValue();                      //Not read yet: "--"
    void clearSetpoint();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double fraction(double v) const;        //0-1 along the arc, clamped

    double m_minimum = 5.0;
    double m_maximum = 30.0;
    double m_value = 0.0;
    double m_setpoint = 0.0;
    bool m_hasValue = false;
    bool m_hasSetpoint = false;
    QString m_caption = "Room";
    QColor m_trackColor = "#2E343A";
    QColor m_arcColor = "#F28C28";
    QColor m_markerColor = "#7DB7FF";
    QColor m_textColor = "#F3F4F5";
    QColor m_mutedColor = "#A3ABB3";
    QColor m_holeColor = "#15181B";
};

#endif // WDIAL_H
