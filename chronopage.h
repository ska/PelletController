#ifndef CHRONOPAGE_H
#define CHRONOPAGE_H

#include <QHash>
#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;
class SerialProto;
class TimePickerDialog;
class wToggle;

/*
 * Chrono page (A-Chrono adapted to B, 800 x 480): Back, chrono on/off,
 * tabs Weekend / Daily / Weekly. Weekend and daily: program on/off and two
 * slots, each start and stop time set with the picker or off. Weekly:
 * program on/off, slot 1-4 selector, start, stop and the days of the
 * selected slot (map checked against the stove display).
 */
class ChronoPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChronoPage(SerialProto *proto, QWidget *parent = nullptr);

    void closePicker();                     //Leaving the page: an open picker is cancelled

signals:
    void backRequested();

private:
    QWidget *programTab(const QString &title, const QString &subtitle, int enableIndex,
                        const QVector<int> &slotIndexes, bool editable);
    QPushButton *timeButton(const QString &caption, int index, bool editable, QWidget *parent);
    void addToggle(wToggle *toggle, int index, bool editable);
    void setValue(quint8 index, quint8 value);
    void showWeeklySlot(int slot);
    void editTime(int index);
    int weeklyBase() const;                 //RequestsIndex of the selected weekly slot start

    SerialProto *m_proto;
    QVector<int> m_values;                  //Per RequestsIndex, -1 = not read yet
    QHash<int, QLabel *> m_timeLabels;      //Time shown on a start/stop button
    QHash<int, wToggle *> m_toggles;
    QHash<int, QString> m_timeNames;        //"Weekend slot 1 start", for the picker
    QStackedWidget *m_tabs;
    TimePickerDialog *m_picker = nullptr;

    int m_weeklySlot = 0;
    QPushButton *m_weeklySlotButtons[4];
    QLabel *m_weeklyStart;
    QLabel *m_weeklyStop;
    QPushButton *m_weeklyDays[7];
};

#endif // CHRONOPAGE_H
