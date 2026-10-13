#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QDateTime>
#include <QWidget>

class QFrame;
class QLabel;
class QPushButton;
class SerialProto;
class wDial;
class wPowerBars;

/*
 * Main page, proposal B of the redesign (800 x 480): header with the stove
 * clock, chrono and link buttons; left card with the state, flame, smoke
 * and On/Off (Reset in the alarm states); in the middle the dial with the
 * setpoint -/+; right card with the power -/+ and bars.
 */
class MainPage : public QWidget
{
    Q_OBJECT

public:
    explicit MainPage(SerialProto *proto, QWidget *parent = nullptr);

    QDateTime stoveDateTime() const { return m_stoveDateTime; }  //Last read, invalid before

signals:
    void chronoRequested();
    void diagRequested();
    void clockRequested();

private:
    /* What the page shows, from the stove state and the link */
    enum class Look { Waiting, Off, Active, Stopping, Alarm, Unknown, NoLink };

    void setStoveState(quint8 state, const QString &name);
    void setLink(quint8 linkState);
    void setChrono(bool on);
    void setPower(quint8 setPower, quint8 flamePower);
    void setDateTime(const QDateTime &dt);
    void applyLook();
    void onOffClicked();
    static void repolish(QWidget *w);

    SerialProto *m_proto;

    QPushButton *m_clockButton;
    QLabel *m_clock;
    QLabel *m_date;
    QPushButton *m_chronoButton;
    QPushButton *m_linkButton;

    QFrame *m_stateCard;
    QFrame *m_stateChip;
    QLabel *m_chipIcon;
    QLabel *m_chipText;
    QLabel *m_flameValue;
    QLabel *m_smokeValue;
    QLabel *m_hint;
    QPushButton *m_onOffButton;

    wDial *m_dial;
    QLabel *m_setValue;
    QPushButton *m_setMinus;
    QPushButton *m_setPlus;

    QLabel *m_powerValue;
    wPowerBars *m_bars;
    QPushButton *m_powerMinus;
    QPushButton *m_powerPlus;

    QDateTime m_stoveDateTime;
    quint8 m_state = 0;
    QString m_stateName;
    bool m_stateRead = false;
    quint8 m_link = 0;                      //SerialProto::LinkState
    Look m_look = Look::Waiting;
};

#endif // MAINPAGE_H
