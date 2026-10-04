#include "wClickableLCDNumber.h"

wClickableLCDNumber::wClickableLCDNumber(QWidget *parent)
    : QLCDNumber(parent)
{
}

wClickableLCDNumber::~wClickableLCDNumber()
{
}


void wClickableLCDNumber::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit clicked();
}

quint32 wClickableLCDNumber::tensOfMins() const
{
    return m_tensOfMins;
}

void wClickableLCDNumber::setTensOfMins(quint32 newTensOfMins)
{
    if (m_tensOfMins == newTensOfMins)
        return;
    m_tensOfMins = newTensOfMins;

    display( QTime((quint8)m_tensOfMins/6, (m_tensOfMins%6)*10).toString("hh:mm") );

}

