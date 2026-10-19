#include "mainpage.h"

#include "homeassistant.h"
#include "serialproto.h"
#include "uiicons.h"
#include "wDial.h"
#include "wKeyCap.h"
#include "wPowerBars.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#define ONOFF_HOLD_MS   2000                //Key 1: turn on, turn off and reset only after a hold

/* Palette of the redesign (qss/default.qss has the same values) */
namespace {
const QColor kText("#F3F4F5");
const QColor kMuted("#A3ABB3");
const QColor kGrey("#7D858D");
const QColor kChip("#C9CED3");
const QColor kOrange("#F28C28");
const QColor kBlue("#7DB7FF");
const QColor kTeal("#4FB3A9");
const QColor kRed("#F2555A");
const QColor kButton("#343A41");
const QColor kCard("#22262B");
const QColor kCapBorder("#3A4047");

QLabel *caption(const QString &text, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    l->setObjectName("caption");
    return l;
}

/* Keycap colors: background, border, text, caption/hint */
void paintCap(wKeyCap *cap, const QColor &bg, const QColor &border, const QColor &text, const QColor &caption)
{
    cap->setBackgroundColor(bg);
    cap->setBorderColor(border);
    cap->setTextColor(text);
    cap->setCaptionColor(caption);
}
}

MainPage::MainPage(SerialProto *proto, QWidget *parent)
    : QWidget(parent), m_proto(proto)
{
    setObjectName("mainPage");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    QVBoxLayout *content = new QVBoxLayout;
    content->setContentsMargins(16, 14, 16, 10);
    content->setSpacing(12);
    root->addLayout(content, 1);

    /* Header: stove clock, stove and network status */
    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(12);
    //Stove clock: a tap opens the clock setting
    m_clockButton = new QPushButton(this);
    m_clockButton->setObjectName("clockButton");
    m_clockButton->setFocusPolicy(Qt::NoFocus);
    m_clockButton->setAccessibleName("Set the stove clock");
    QHBoxLayout *clockBox = new QHBoxLayout(m_clockButton);
    clockBox->setContentsMargins(0, 0, 8, 0);
    clockBox->setSpacing(12);
    m_clock = new QLabel("--:--", m_clockButton);
    m_clock->setObjectName("clockLabel");
    m_date = new QLabel(m_clockButton);
    m_date->setObjectName("dateLabel");
    for(QLabel *l : { m_clock, m_date })
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
    clockBox->addWidget(m_clock, 0, Qt::AlignBottom);
    clockBox->addWidget(m_date, 0, Qt::AlignBottom);
    m_clockButton->setMinimumSize(clockBox->sizeHint());
    header->addWidget(m_clockButton);
    header->addStretch();
    //Status rows as RoomSense: name, dot, online/offline; a tap opens diag
    m_statusButton = new QPushButton(this);
    m_statusButton->setObjectName("statusButton");
    m_statusButton->setFocusPolicy(Qt::NoFocus);
    m_statusButton->setAccessibleName("Connections, open diagnostics");
    QGridLayout *status = new QGridLayout(m_statusButton);
    status->setContentsMargins(8, 0, 8, 0);
    status->setHorizontalSpacing(8);
    status->setVerticalSpacing(2);
    QLabel *stoveName = new QLabel("Stove", m_statusButton);
    m_networkName = new QLabel("Network", m_statusButton);
    m_stoveDot = new QLabel(m_statusButton);
    m_networkDot = new QLabel(m_statusButton);
    m_stoveText = new QLabel(m_statusButton);
    m_networkText = new QLabel(m_statusButton);
    for(QLabel *l : { stoveName, m_networkName })
    {
        l->setObjectName("stateName");
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    for(QLabel *l : { m_stoveDot, m_networkDot })
    {
        l->setObjectName("stateDot");
        l->setFixedSize(12, 12);
    }
    for(QLabel *l : { m_stoveText, m_networkText })
        l->setObjectName("stateText");
    const QList<QLabel *> statusLabels = { stoveName, m_stoveDot, m_stoveText,
                                           m_networkName, m_networkDot, m_networkText };
    for(int i = 0; i < statusLabels.size(); i++)
    {
        statusLabels.at(i)->setAttribute(Qt::WA_TransparentForMouseEvents);
        status->addWidget(statusLabels.at(i), i / 3, i % 3, Qt::AlignVCenter);
    }
    header->addWidget(m_statusButton);
    content->addLayout(header);

    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(12);
    content->addLayout(body, 1);

    /* Left card: state, flame, smoke, chrono */
    m_stateCard = new QFrame(this);
    m_stateCard->setObjectName("card");
    m_stateCard->setFixedWidth(200);
    QVBoxLayout *left = new QVBoxLayout(m_stateCard);
    left->setContentsMargins(18, 18, 18, 18);
    left->setSpacing(8);
    m_stateChip = new QFrame(m_stateCard);
    m_stateChip->setObjectName("stateChip");
    QHBoxLayout *chip = new QHBoxLayout(m_stateChip);
    chip->setContentsMargins(12, 6, 14, 6);
    chip->setSpacing(8);
    m_chipIcon = new QLabel(m_stateChip);
    m_chipIcon->setFixedSize(18, 18);
    m_chipText = new QLabel(m_stateChip);
    m_chipText->setObjectName("chipText");
    m_chipText->setWordWrap(true);              //"Ignition Failure" on two lines
    chip->addWidget(m_chipIcon);
    chip->addWidget(m_chipText, 1);
    left->addWidget(m_stateChip, 0, Qt::AlignLeft);
    left->addSpacing(8);
    left->addWidget(caption("Flame", m_stateCard));
    m_flameValue = new QLabel("--", m_stateCard);
    m_flameValue->setObjectName("bigValue");
    left->addWidget(m_flameValue);
    left->addWidget(caption("Smoke", m_stateCard));
    m_smokeValue = new QLabel("--", m_stateCard);
    m_smokeValue->setObjectName("bigValue");
    left->addWidget(m_smokeValue);
    left->addStretch();
    m_chronoButton = new QPushButton(m_stateCard);
    m_chronoButton->setObjectName("chronoButton");
    m_chronoButton->setFocusPolicy(Qt::NoFocus);
    m_chronoButton->setFixedHeight(54);
    m_chronoButton->setIconSize(QSize(20, 20));
    left->addWidget(m_chronoButton);
    body->addWidget(m_stateCard);

    /* Middle: dial (room temperature and setpoint) */
    m_dial = new wDial(this);
    m_dial->setFixedSize(300, 300);
    body->addWidget(m_dial, 1, Qt::AlignCenter);

    /* Right card: power bars */
    QFrame *powerCard = new QFrame(this);
    powerCard->setObjectName("card");
    powerCard->setFixedWidth(200);
    QVBoxLayout *right = new QVBoxLayout(powerCard);
    right->setContentsMargins(18, 18, 18, 18);
    right->setSpacing(14);
    QHBoxLayout *powerHead = new QHBoxLayout;
    powerHead->addWidget(caption("Power", powerCard), 0, Qt::AlignBottom);
    powerHead->addStretch();
    m_powerValue = new QLabel("--", powerCard);
    m_powerValue->setObjectName("powerValue");
    powerHead->addWidget(m_powerValue, 0, Qt::AlignBottom);
    right->addLayout(powerHead);
    m_bars = new wPowerBars(powerCard);
    m_bars->setMaximum(POWER_MAX);
    right->addWidget(m_bars, 1);
    body->addWidget(powerCard);

    /* Key bar: six equal columns, each keycap centered over its key; the
     * whole column takes touches, the cap is drawn 54 px high inside it */
    QFrame *keyBar = new QFrame(this);
    keyBar->setObjectName("keyBar");
    keyBar->setFixedHeight(74);
    QHBoxLayout *keys = new QHBoxLayout(keyBar);
    keys->setContentsMargins(0, 0, 0, 0);
    keys->setSpacing(0);
    for(int i = 0; i < 6; i++)
    {
        m_keys[i] = new wKeyCap(keyBar);
        m_keys[i]->setFixedHeight(74);
        m_keys[i]->setInset(QMargins(5, 10, 5, 10));
        m_keys[i]->setIconSize(QSize(18, 18));
        m_keys[i]->setAccessibleName(QString("Key %1").arg(i + 1));
        keys->addWidget(m_keys[i], 1);
    }
    root->addWidget(keyBar);

    //Key 2: free for now
    m_keys[1]->setDashed(true);
    m_keys[1]->setEnabled(false);
    paintCap(m_keys[1], Qt::transparent, kCapBorder, kText, kMuted);
    //Keys 3-6: setpoint and power steps
    const struct { int key; const char *caption; QChar sign; QColor color; } steps[] = {
        { 2, "Setpoint", QChar(0x2212), kBlue },
        { 3, "Setpoint", QChar('+'),    kBlue },
        { 4, "Power",    QChar(0x2212), kOrange },
        { 5, "Power",    QChar('+'),    kOrange },
    };
    for(const auto &s : steps)
    {
        wKeyCap *cap = m_keys[s.key];
        cap->setCaption(s.caption);
        cap->setText(s.sign);
        cap->setTextPixelSize(28);
        paintCap(cap, kCard, kCapBorder, s.color, kMuted);
    }
    m_keys[0]->setHoldTime(ONOFF_HOLD_MS);
    m_keys[0]->setHint("hold 2 s");

    /* Buttons and keys */
    connect(m_chronoButton, &QPushButton::clicked, this, &MainPage::chronoRequested);
    connect(m_statusButton, &QPushButton::clicked, this, &MainPage::diagRequested);
    connect(m_clockButton, &QPushButton::clicked, this, &MainPage::clockRequested);
    connect(m_keys[0], &wKeyCap::activated, this, &MainPage::onOffActivated);
    connect(m_keys[2], &wKeyCap::activated, m_proto, &SerialProto::writeStoveDecSetPoint);
    connect(m_keys[3], &wKeyCap::activated, m_proto, &SerialProto::writeStoveIncSetPoint);
    connect(m_keys[4], &wKeyCap::activated, m_proto, &SerialProto::writeStoveDecPower);
    connect(m_keys[5], &wKeyCap::activated, m_proto, &SerialProto::writeStoveIncPower);

    /* Values from the stove */
    connect(m_proto, &SerialProto::updateAmbTemp, m_dial, &wDial::setValue);
    connect(m_proto, &SerialProto::updateSetTemp, m_dial, &wDial::setSetpoint);
    connect(m_proto, &SerialProto::updateSmoke, this, [this](quint8 temp, quint16) {
        m_smokeValue->setText(QString("%1 %2C").arg(temp).arg(QChar(0x00B0)));
    });
    connect(m_proto, &SerialProto::updateStoveState, this, &MainPage::setStoveState);
    connect(m_proto, &SerialProto::updatePower, this, &MainPage::setPower);
    connect(m_proto, &SerialProto::updateStoveDateTime, this, &MainPage::setDateTime);
    connect(m_proto, &SerialProto::linkStateChanged, this, &MainPage::setLink);
    connect(m_proto, &SerialProto::updateChronoEnable, this, &MainPage::setChrono);

    setHomeAssistant(nullptr);
    setChrono(false);
    applyLook();
}

void MainPage::repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

/* Dynamic property of the style sheet: polishing again is slow on the
 * panel and the stove sends every value at each read, so only on a change */
bool MainPage::setStyleProperty(QWidget *w, const char *name, const QVariant &value)
{
    if(w->property(name) == value)
        return false;
    w->setProperty(name, value);
    repolish(w);
    return true;
}

void MainPage::setHomeAssistant(HomeAssistant *ha)
{
    for(QLabel *l : { m_networkName, m_networkDot, m_networkText })
        l->setVisible(ha);
    if(ha)
    {
        setNetwork(QMqttClient::Connected == ha->clientState());
        connect(ha, &HomeAssistant::clientStateChanged, this, [this](int state) {
            setNetwork(QMqttClient::Connected == state);
        });
    }
    fitStatus();
}

/* A push button ignores the layout inside it: size it from the labels,
 * after every text change */
void MainPage::fitStatus()
{
    m_statusButton->setMinimumSize(m_statusButton->layout()->sizeHint());
}

void MainPage::keyEvent(int key, bool pressed)
{
    if(key < 1 || key > 6)
        return;
    if(pressed)
        m_keys[key - 1]->keyPress();
    else
        m_keys[key - 1]->keyRelease();
}

void MainPage::setStoveState(quint8 state, const QString &name)
{
    if(m_stateRead && state == m_state && name == m_stateName)
        return;                             //Read again, unchanged
    m_state = state;
    m_stateName = name;
    m_stateRead = true;
    applyLook();
}

/* Port closed or stove not answering: shown in place of the state until
 * the next state read */
void MainPage::setLink(quint8 linkState)
{
    if(m_lookShown && linkState == m_link)
        return;
    m_link = linkState;
    if(SerialProto::LinkOk != linkState)
        m_stateRead = false;
    applyLook();
}

void MainPage::setNetwork(bool online)
{
    m_networkText->setText(online ? "online" : "offline");
    setStyleProperty(m_networkDot, "on", online);
    fitStatus();
}

void MainPage::setChrono(bool on)
{
    if(int(on) == m_chrono)
        return;
    m_chrono = on;
    m_chronoButton->setText(on ? "Chrono on" : "Chrono off");
    m_chronoButton->setIcon(tintedIcon("clock", on ? kBlue : kChip, 20));
    setStyleProperty(m_chronoButton, "on", on);
}

void MainPage::setPower(quint8 setPower, quint8 flamePower)
{
    m_powerValue->setText(QString("%1<span style=\"font-size: 17px; color: #A3ABB3;\"> / %2</span>")
                              .arg(setPower).arg(POWER_MAX));
    m_bars->setLevel(setPower);
    m_flameValue->setText(QString("%1 %").arg(flamePower));
}

void MainPage::setDateTime(const QDateTime &dt)
{
    m_stoveDateTime = dt;
    const QLocale c = QLocale::c();
    m_clock->setText(c.toString(dt, "HH:mm"));
    m_date->setText(c.toString(dt, "ddd d MMM"));
    //A push button ignores the layout inside it: size it from the labels
    m_clockButton->setMinimumSize(m_clockButton->layout()->sizeHint());
}

void MainPage::applyLook()
{
    if(SerialProto::LinkOk != m_link)
        m_look = Look::NoLink;
    else if(!m_stateRead)
        m_look = Look::Waiting;
    else if(m_state == SerialProto::Off)
        m_look = Look::Off;
    else if(m_state == SerialProto::FinalCleaning)
        m_look = Look::Stopping;
    else if(m_state >= SerialProto::PelletMissing && m_state <= SerialProto::Alarm)
        m_look = Look::Alarm;
    else if(m_state <= SerialProto::Standby)
        m_look = Look::Active;
    else
        m_look = Look::Unknown;

    /* State chip */
    QString chipText = m_stateName;
    QString chipIcon = "power";
    QColor chipColor = kChip;
    QString chip = "off";
    switch(m_look)
    {
    case Look::Waiting:     chipText = "Reading..."; break;
    case Look::NoLink:      chipText = (SerialProto::LinkPortClosed == m_link) ? "No serial port" : "No reply";
                            chipIcon = "alarm"; chipColor = kOrange; chip = "warning"; break;
    case Look::Active:      chipIcon = "flame"; chipColor = kOrange; chip = "active"; break;
    case Look::Alarm:       chipIcon = "alarm"; chipColor = kRed; chip = "alarm"; break;
    default:                break;
    }
    const QString iconKey = chipIcon + chipColor.name();
    if(iconKey != m_chipIconKey)
    {
        m_chipIconKey = iconKey;
        m_chipIcon->setPixmap(tintedIcon(chipIcon, chipColor, 18).pixmap(18, 18));
    }
    m_chipText->setText(chipText);
    //The chip text color comes from the chip look
    if(setStyleProperty(m_stateChip, "look", chip))
        repolish(m_chipText);

    setStyleProperty(m_stateCard, "alarm", Look::Alarm == m_look);

    /* Key 1: Turn on when Off, Turn off when Working, Reset in an alarm
     * (On/Off would write Starting or Final Cleaning), disabled otherwise
     * (the stove is moving between states) */
    OnOff onOff = OnOff::None;
    switch(m_look)
    {
    case Look::Off:     onOff = OnOff::TurnOn; break;
    case Look::Active:  if(m_state == SerialProto::Working) onOff = OnOff::TurnOff; break;
    case Look::Alarm:   onOff = OnOff::Reset; break;
    default:            break;
    }
    wKeyCap *cap = m_keys[0];
    //A hold started for another action does nothing
    if(onOff != m_onOff || !m_lookShown)
    {
        if(onOff != m_onOff)
            cap->cancel();
        m_onOff = onOff;
        switch(onOff)
        {
        case OnOff::TurnOn:
            cap->setText("Turn on");
            cap->setIcon(tintedIcon("power", kTeal, 18));
            paintCap(cap, QColor("#1F3B38"), kTeal, kTeal, kMuted);
            break;
        case OnOff::Reset:
            cap->setText("Reset");
            cap->setIcon(tintedIcon("reset", kRed, 18));
            paintCap(cap, QColor("#3A1A1C"), kRed, kRed, QColor("#E8A3A5"));
            break;
        default:
            cap->setText("Turn off");
            cap->setIcon(tintedIcon("power", kText, 18));
            paintCap(cap, kButton, kCapBorder, kText, kMuted);
            break;
        }
    }
    cap->setEnabled(OnOff::None != onOff);

    /* Heat colors only while the stove is burning */
    const bool hot = (Look::Active == m_look);
    m_dial->setArcColor(hot ? kOrange : kGrey);
    m_bars->setActiveColor(hot ? kOrange : kGrey);
    setStyleProperty(m_flameValue, "hot", hot);

    /* Stove status row: the serial link */
    const bool link = (SerialProto::LinkOk == m_link);
    m_stoveText->setText(link ? "online" : "offline");
    setStyleProperty(m_stoveDot, "on", link);
    fitStatus();

    /* Setpoint and power -/+ only with the stove answering */
    for(int i = 2; i < 6; i++)
        m_keys[i]->setEnabled(link);
    m_lookShown = true;
}

void MainPage::onOffActivated()
{
    switch(m_onOff)
    {
    case OnOff::TurnOn:     m_proto->writeStoveStateOn(); break;
    case OnOff::TurnOff:    m_proto->writeStoveStateOff(); break;
    case OnOff::Reset:      m_proto->writeStoveStateOffForce(); break;
    default:                break;
    }
}
