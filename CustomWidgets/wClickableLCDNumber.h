#ifndef WCLICKABLELCDNUMBER_H
#define WCLICKABLELCDNUMBER_H

#include <QWidget>
#include <QTime>
#include <QLCDNumber>

class wClickableLCDNumber : public QLCDNumber
{
    Q_OBJECT

public:
    wClickableLCDNumber(QWidget *parent = nullptr);
    ~wClickableLCDNumber();

    quint32 tensOfMins() const;
    void setTensOfMins(quint32 newTensOfMins);

signals:
    void clicked();

    void tensOfMinsChanged();

protected:
    void mousePressEvent(QMouseEvent* event);

private:
    quint32 m_tensOfMins;
    Q_PROPERTY(quint32 tensOfMins READ tensOfMins WRITE setTensOfMins)
};

#endif // WCLICKABLELCDNUMBER_H
