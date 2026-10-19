#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QDateTime>
#include <QWidget>

class HomeAssistant;
class QFrame;
class QLabel;
class QPushButton;
class SerialProto;
class wDial;
class wKeyCap;
class wPowerBars;

/*
 * Main page (800 x 480): header with the stove clock and the stove and
 * network status; left card with the state, flame, smoke and the chrono
 * button; in the middle the dial; right card with the power bars; at the
 * bottom the key bar, one keycap over each of the six keys under the
 * screen (touch buttons on a panel without keys):
 *   1 On/Off, Reset in the alarm states (hold 2 s)   2 free
 *   3-4 setpoint -/+                                  5-6 power -/+
 */
class MainPage : public QWidget
{
    Q_OBJECT

public:
    explicit MainPage(SerialProto *proto, QWidget *parent = nullptr);

    /* Network row of the header, hidden without MQTT (nullptr) */
    void setHomeAssistant(HomeAssistant *ha);

    QDateTime stoveDateTime() const { return m_stoveDateTime; }  //Last read, invalid before

    /* Physical key 1-6 pressed or released */
    void keyEvent(int key, bool pressed);

signals:
    void chronoRequested();
    void diagRequested();
    void clockRequested();

private:
    /* What the page shows, from the stove state and the link */
    enum class Look { Waiting, Off, Active, Stopping, Alarm, Unknown, NoLink };
    /* What key 1 does now */
    enum class OnOff { None, TurnOn, TurnOff, Reset };

    void setStoveState(quint8 state, const QString &name);
    void setLink(quint8 linkState);
    void setNetwork(bool online);
    void fitStatus();
    void setChrono(bool on);
    void setPower(quint8 setPower, quint8 flamePower);
    void setDateTime(const QDateTime &dt);
    void applyLook();
    void onOffActivated();
    static void repolish(QWidget *w);
    static bool setStyleProperty(QWidget *w, const char *name, const QVariant &value);

    SerialProto *m_proto;

    QPushButton *m_clockButton;
    QLabel *m_clock;
    QLabel *m_date;
    QPushButton *m_statusButton;
    QLabel *m_stoveDot;
    QLabel *m_stoveText;
    QLabel *m_networkName;
    QLabel *m_networkDot;
    QLabel *m_networkText;

    QFrame *m_stateCard;
    QFrame *m_stateChip;
    QLabel *m_chipIcon;
    QLabel *m_chipText;
    QLabel *m_flameValue;
    QLabel *m_smokeValue;
    QPushButton *m_chronoButton;

    wDial *m_dial;

    QLabel *m_powerValue;
    wPowerBars *m_bars;

    wKeyCap *m_keys[6];

    QDateTime m_stoveDateTime;
    quint8 m_state = 0;
    QString m_stateName;
    bool m_stateRead = false;
    quint8 m_link = 0;                      //SerialProto::LinkState
    Look m_look = Look::Waiting;
    OnOff m_onOff = OnOff::None;
    bool m_lookShown = false;               //applyLook() ran once
    QString m_chipIconKey;                  //Icon and color shown in the chip
    int m_chrono = -1;                      //Chrono shown: -1 none yet, 0 off, 1 on
};

#endif // MAINPAGE_H
