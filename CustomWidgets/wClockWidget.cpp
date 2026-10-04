#include "wClockWidget.h"

wClockWidget::wClockWidget(QWidget *parent) :
    QWidget(parent)
{
    custom_indexes.clear();
    resize(100, 100);
}

void wClockWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setRenderHint(QPainter::HighQualityAntialiasing);

    double R = qMin( this->rect().width(), this->rect().height()) / 2;
    quint8 margin = 4;

    QRectF Rect = QRectF(0, 0, 2 * R - margin, 2 * R - margin);
    Rect.moveCenter(this->rect().center());

    painter.setBrush( m_bgColor );
    painter.drawEllipse(Rect);

    QRectF rect = QRectF(0, 0, m_circleRadius, m_circleRadius);

    if( m_currentIndex < m_numberOfSteps )
    {
        QPointF c = center_by_index(m_currentIndex);
        rect.moveCenter(c);
        QPen pen;
        pen.setColor(m_needleColor);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(c, this->rect().center());
        painter.setBrush(m_needleColor);
        painter.drawEllipse(rect);
    }

    for(quint32 i=0; i < m_numberOfSteps; i++)
    {
        quint32 j = m_startIndex + (i * m_indexesGap);
        QPointF c = center_by_index(i);
        rect.moveCenter(c);
        painter.setPen(m_fontColor);
        if(custom_indexes.length() >= m_numberOfSteps)
            painter.drawText(rect,  Qt::AlignCenter, custom_indexes.at(i) );
        else
            painter.drawText(rect,  Qt::AlignCenter, QString("%1").arg(j) );
    }
}

void wClockWidget::mousePressEvent(QMouseEvent *event)
{
    qint8 i = index_by_click(event->pos());
    if(i >= 0)
    {
        setCurrentIndex(i);
    }
}

QPointF wClockWidget::center_by_index(quint8 index)
{
    double R = qMin( rect().width(),  rect().height()) / 2;
    double angle = (DELTA_ANGLE * index) - qDegreesToRadians((double )m_startAngle);
    QPoint center = rect().center();
    QPointF p = center + (R - m_circleRadius) * QPointF(qCos(angle), qSin(angle));
    return p;
}

quint32 wClockWidget::index_by_click(QPoint pos)
{
    for(quint32 i=0; i < m_numberOfSteps; i++)
    {
        QPointF c = center_by_index(i);
        float delta = QVector2D(pos).distanceToPoint(QVector2D(c));
        if(delta < m_circleRadius)
            return i;
    }
    return -1;
}

quint32 wClockWidget::currentIndex() const
{
    return m_currentIndex;
}

void wClockWidget::setCurrentIndex(quint32 newCurrentIndex)
{
    if (m_currentIndex == newCurrentIndex)
        return;
    m_currentIndex = newCurrentIndex;
    update();

    emit currentIndexChanged();
}

QString wClockWidget::currentValue() const
{
    quint32 j = (m_startIndex + (m_currentIndex * m_indexesGap));
    if(custom_indexes.length() >= m_numberOfSteps)
        return( custom_indexes.at(m_currentIndex) );
    else
        return (QString("%1").arg(j));
}

void wClockWidget::setValue(QString val)
{
    quint32 value = val.toInt();
    setValue(value);
}

void wClockWidget::setValue(quint32 value)
{
    setCurrentIndex( (quint32) (value/m_indexesGap)-m_startIndex);
}

/*
void wClockWidget::setCurrent_index(const quint8 newCurrent_index)
{
    if(newCurrent_index == m_currentIndex)
        return;
    m_currentIndex = newCurrent_index;
    update();
    quint8 j = (m_startIndex + (m_currentIndex * m_indexesGap));

    if(custom_indexes.length() >= m_numberOfSteps)
        emit valueChanged( custom_indexes.at(m_currentIndex) );
    else
        emit valueChanged(QString("%1").arg(j));
}
*/


quint32 wClockWidget::startIndex() const
{
    return m_startIndex;
}

void wClockWidget::setStartIndex(quint32 newStartIndex)
{
    if (m_startIndex == newStartIndex)
        return;
    m_startIndex = newStartIndex;
    update();
}

quint32 wClockWidget::indexesGap() const
{
    return m_indexesGap;
}

void wClockWidget::setIndexesGap(quint32 newIndexesGap)
{
    if (m_indexesGap == newIndexesGap)
        return;
    m_indexesGap = newIndexesGap;
    update();
}

quint32 wClockWidget::numberOfSteps() const
{
    return m_numberOfSteps;
}

void wClockWidget::setNumberOfSteps(quint32 newNumberOfSteps)
{
    if (m_numberOfSteps == newNumberOfSteps)
        return;
    m_numberOfSteps = newNumberOfSteps;
    update();
}

double wClockWidget::startAngle() const
{
    return m_startAngle;
}

void wClockWidget::setStartAngle(double newStartAngle)
{
    if (qFuzzyCompare(m_startAngle, newStartAngle))
        return;
    m_startAngle = newStartAngle;
    update();
}

double wClockWidget::circleRadius() const
{
    return m_circleRadius;
}

void wClockWidget::setCircleRadius(double newCircleRadius)
{
    if (qFuzzyCompare(m_circleRadius, newCircleRadius))
        return;
    m_circleRadius = newCircleRadius;
    update();
}

QColor wClockWidget::fontColor() const
{
    return m_fontColor;
}

void wClockWidget::setFontColor(const QColor &newFontColor)
{
    if (m_fontColor == newFontColor)
        return;
    m_fontColor = newFontColor;
    update();
}

QColor wClockWidget::needleColor() const
{
    return m_needleColor;
}

void wClockWidget::setNeedleColor(const QColor &newNeedleColor)
{
    if (m_needleColor == newNeedleColor)
        return;
    m_needleColor = newNeedleColor;
    update();
}

QColor wClockWidget::bgColor() const
{
    return m_bgColor;
}

void wClockWidget::setBgColor(const QColor &newBgColor)
{
    if (m_bgColor == newBgColor)
        return;
    m_bgColor = newBgColor;
    update();
}

void wClockWidget::setCustom_indexes(const QStringList &newCustom_indexes)
{
    custom_indexes = newCustom_indexes;
}



