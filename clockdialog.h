#ifndef CLOCKDIALOG_H
#define CLOCKDIALOG_H

#include <QDateTime>
#include <QDialog>

class QLabel;
class QPushButton;
class wClockFace;

/*
 * Stove clock: date with -/+ for day, month and year, time on the clock
 * face (hours, then any minute). Starts from the stove clock read.
 */
class ClockDialog : public QDialog
{
    Q_OBJECT

public:
    ClockDialog(const QDateTime &start, QWidget *parent = nullptr);

    QDateTime dateTime() const;             //After accept()

private:
    QLabel *dateRow(class QVBoxLayout *box, const QString &name, int field);
    void stepDate(int field, int delta);
    void showHours();
    void showMinutes();
    void showValue();

    QDate m_date;
    int m_hours;
    int m_minutes;
    QLabel *m_day;
    QLabel *m_month;
    QLabel *m_year;
    QLabel *m_weekday;
    QPushButton *m_hoursButton;
    QPushButton *m_minutesButton;
    wClockFace *m_face;
};

#endif // CLOCKDIALOG_H
