#ifndef WTIMEPICKER_H
#define WTIMEPICKER_H

#include <QWidget>
#include <QTime>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {
class TimePicker;
}
QT_END_NAMESPACE

class wTimePicker : public QWidget
{
    Q_OBJECT

public:
    explicit wTimePicker(QWidget *parent = nullptr);
    ~wTimePicker();

    quint32 getTensOfMins() const;
    void setTensOfMins(quint32 newTensOfMins);

    QTime getTime() const;
    void setTime(const QTime &newTime);
    void showHours();
    void showMins();

signals:

private slots:
    void on_Hours_clicked();
    void on_Mins_clicked();
    void on_clockwidget_currentIndexChanged();
    void on_amClbl_clicked();
    void on_pmClbl_clicked();

private:
    Ui::TimePicker *ui;
    bool m_am = true;
    QTime time;

    Q_PROPERTY(quint32 tensOfMins READ getTensOfMins WRITE setTensOfMins)
    Q_PROPERTY(QTime time READ getTime WRITE setTime)
};

#endif // WTIMEPICKER_H
