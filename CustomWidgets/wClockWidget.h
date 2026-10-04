#ifndef WCLOCKWIDGET_H
#define WCLOCKWIDGET_H

#include <QMouseEvent>
#include <QWidget>
#include <QPainter>
#include <QtMath>

#define DELTA_ANGLE  (2 * M_PI / m_numberOfSteps)

class wClockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit wClockWidget(QWidget *parent = 0);
    virtual void paintEvent(QPaintEvent *event);

    void setCustom_indexes(const QStringList &newCustom_indexes);


    void setBgColor(const QColor &newBgColor);
    QColor bgColor() const;

    QColor needleColor() const;
    void setNeedleColor(const QColor &newNeedleColor);

    QColor fontColor() const;
    void setFontColor(const QColor &newFontColor);

    double circleRadius() const;
    void setCircleRadius(double newCircleRadius);

    double startAngle() const;
    void setStartAngle(double newStartAngle);

    quint32 numberOfSteps() const;
    void setNumberOfSteps(quint32 newNumberOfSteps);

    quint32 indexesGap() const;
    void setIndexesGap(quint32 newIndexesGap);

    quint32 startIndex() const;
    void setStartIndex(quint32 newStartIndex);

    quint32 currentIndex() const;
    void setCurrentIndex(quint32 newCurrentIndex);

    QString currentValue() const;
    void setValue(QString val);
    void setValue(quint32 value);

signals:
    void currentIndexChanged();

protected:
    void mousePressEvent(QMouseEvent * event);

private:
    QPointF center_by_index(quint8 index);
    quint32 index_by_click(QPoint pos);
    //quint8  m_current_index = 0;

    //qint8   m_start_index= 0;

    QStringList custom_indexes;

    quint32 m_currentIndex  = 0;
    quint32 m_startIndex    = 0;
    quint32 m_indexesGap    = 1;
    quint32 m_numberOfSteps = 12;
    double  m_startAngle    = 0.0;
    double  m_circleRadius  = 20.0;
    QColor  m_bgColor       = Qt::black;
    QColor  m_needleColor   = "#4F0099";
    QColor  m_fontColor     = Qt::white;

    Q_PROPERTY(QColor   bgColor         READ bgColor        WRITE setBgColor        )
    Q_PROPERTY(QColor   needleColor     READ needleColor    WRITE setNeedleColor    )
    Q_PROPERTY(QColor   fontColor       READ fontColor      WRITE setFontColor      )
    Q_PROPERTY(double   circleRadius    READ circleRadius   WRITE setCircleRadius   )
    Q_PROPERTY(double   startAngle      READ startAngle     WRITE setStartAngle     )
    Q_PROPERTY(quint32  numberOfSteps   READ numberOfSteps  WRITE setNumberOfSteps  )
    Q_PROPERTY(quint32  indexesGap      READ indexesGap     WRITE setIndexesGap     )
    Q_PROPERTY(quint32  startIndex      READ startIndex     WRITE setStartIndex     )
    Q_PROPERTY(quint32 currentIndex     READ currentIndex   WRITE setCurrentIndex   NOTIFY currentIndexChanged      FINAL)

    Q_PROPERTY(QString currentValue     READ currentValue   CONSTANT  FINAL)


};

#endif // WCLOCKWIDGET_H
