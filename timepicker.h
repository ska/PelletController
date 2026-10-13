#ifndef TIMEPICKER_H
#define TIMEPICKER_H

#include <QDialog>

class QPushButton;
class wClockFace;

/*
 * Chrono time picker: the time on the left (hours and minutes to tap), a
 * clock face on the right (24 hours, then minutes in steps of 10); Off,
 * Cancel or Save. Value in tens of minutes from 00:00 (0-143), 144 = slot
 * off, as on the stove.
 */
class TimePickerDialog : public QDialog
{
    Q_OBJECT

public:
    TimePickerDialog(const QString &title, quint8 tensOfMins, QWidget *parent = nullptr);

    quint8 value() const { return m_value; }  //After accept(): the time chosen, or 144 (Off)

private:
    void showHours();
    void showMinutes();
    void showValue();

    int m_hours = 7;
    int m_minutes = 0;
    quint8 m_value = 144;
    QPushButton *m_hoursButton;
    QPushButton *m_minutesButton;
    wClockFace *m_face;
};

#endif // TIMEPICKER_H
