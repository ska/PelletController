#include "mainpage.h"

#include "serialproto.h"
#include "uiicons.h"
#include "wDial.h"
#include "wPowerBars.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

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
const QColor kDark("#0B1118");

QLabel *caption(const QString &text, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    l->setObjectName("caption");
    return l;
}

QPushButton *stepButton(const QString &text, const QString &accessible, QWidget *parent)
{
    QPushButton *b = new QPushButton(text, parent);
    b->setObjectName("stepButton");
    b->setAccessibleName(accessible);
    b->setFocusPolicy(Qt::NoFocus);
    return b;
}
}

MainPage::MainPage(SerialProto *proto, QWidget *parent)
    : QWidget(parent), m_proto(proto)
{
    setObjectName("mainPage");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    /* Header: stove clock, chrono, link */
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
    m_chronoButton = new QPushButton(this);
    m_chronoButton->setObjectName("chronoButton");
    m_chronoButton->setFocusPolicy(Qt::NoFocus);
    m_chronoButton->setIconSize(QSize(20, 20));
    m_linkButton = new QPushButton("Stove", this);
    m_linkButton->setObjectName("linkButton");
    m_linkButton->setFocusPolicy(Qt::NoFocus);
    m_linkButton->setIconSize(QSize(16, 16));
    header->addWidget(m_chronoButton);
    header->addWidget(m_linkButton);
    root->addLayout(header);

    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(12);
    root->addLayout(body, 1);

    /* Left card: state, flame, smoke, On/Off */
    m_stateCard = new QFrame(this);
    m_stateCard->setObjectName("card");
    m_stateCard->setFixedWidth(200);
    QVBoxLayout *left = new QVBoxLayout(m_stateCard);
    left->setContentsMargins(18, 18, 18, 18);
    left->setSpacing(10);
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
    left->addSpacing(4);
    left->addWidget(caption("Flame", m_stateCard));
    m_flameValue = new QLabel("--", m_stateCard);
    m_flameValue->setObjectName("bigValue");
    left->addWidget(m_flameValue);
    left->addWidget(caption("Smoke", m_stateCard));
    m_smokeValue = new QLabel("--", m_stateCard);
    m_smokeValue->setObjectName("bigValue");
    left->addWidget(m_smokeValue);
    m_hint = new QLabel(m_stateCard);
    m_hint->setObjectName("hint");
    m_hint->setWordWrap(true);
    left->addWidget(m_hint);
    left->addStretch();
    m_onOffButton = new QPushButton(m_stateCard);
    m_onOffButton->setObjectName("onOffButton");
    m_onOffButton->setFocusPolicy(Qt::NoFocus);
    m_onOffButton->setIconSize(QSize(22, 22));
    left->addWidget(m_onOffButton);
    body->addWidget(m_stateCard);

    /* Middle: dial and setpoint -/+ */
    QVBoxLayout *middle = new QVBoxLayout;
    middle->setSpacing(8);
    m_dial = new wDial(this);
    m_dial->setFixedSize(268, 268);
    middle->addWidget(m_dial, 0, Qt::AlignHCenter);
    middle->addStretch();
    QHBoxLayout *setRow = new QHBoxLayout;
    setRow->setSpacing(16);
    m_setMinus = stepButton(QString(QChar(0x2212)), "Lower the setpoint", this);
    m_setMinus->setFixedSize(96, 72);
    m_setPlus = stepButton("+", "Raise the setpoint", this);
    m_setPlus->setFixedSize(96, 72);
    QVBoxLayout *setBox = new QVBoxLayout;
    setBox->setSpacing(0);
    QLabel *setCaption = caption("Setpoint", this);
    setCaption->setAlignment(Qt::AlignHCenter);
    m_setValue = new QLabel("--", this);
    m_setValue->setObjectName("setValue");
    m_setValue->setAlignment(Qt::AlignHCenter);
    m_setValue->setFixedWidth(120);
    setBox->addWidget(setCaption);
    setBox->addWidget(m_setValue);
    setRow->addStretch();
    setRow->addWidget(m_setMinus);
    setRow->addLayout(setBox);
    setRow->addWidget(m_setPlus);
    setRow->addStretch();
    middle->addLayout(setRow);
    body->addLayout(middle, 1);

    /* Right card: power -/+ and bars */
    QFrame *powerCard = new QFrame(this);
    powerCard->setObjectName("card");
    powerCard->setFixedWidth(200);
    QVBoxLayout *right = new QVBoxLayout(powerCard);
    right->setContentsMargins(18, 18, 18, 18);
    right->setSpacing(12);
    QHBoxLayout *powerHead = new QHBoxLayout;
    powerHead->addWidget(caption("Power", powerCard), 0, Qt::AlignBottom);
    powerHead->addStretch();
    m_powerValue = new QLabel("--", powerCard);
    m_powerValue->setObjectName("powerValue");
    powerHead->addWidget(m_powerValue, 0, Qt::AlignBottom);
    right->addLayout(powerHead);
    m_powerPlus = stepButton("+", "Raise the power", powerCard);
    m_powerPlus->setFixedHeight(64);
    right->addWidget(m_powerPlus);
    m_bars = new wPowerBars(powerCard);
    m_bars->setMaximum(POWER_MAX);
    right->addWidget(m_bars, 1);
    m_powerMinus = stepButton(QString(QChar(0x2212)), "Lower the power", powerCard);
    m_powerMinus->setFixedHeight(64);
    right->addWidget(m_powerMinus);
    body->addWidget(powerCard);

    /* Buttons */
    connect(m_chronoButton, &QPushButton::clicked, this, &MainPage::chronoRequested);
    connect(m_linkButton, &QPushButton::clicked, this, &MainPage::diagRequested);
    connect(m_clockButton, &QPushButton::clicked, this, &MainPage::clockRequested);
    connect(m_onOffButton, &QPushButton::clicked, this, &MainPage::onOffClicked);
    connect(m_setMinus, &QPushButton::clicked, m_proto, &SerialProto::writeStoveDecSetPoint);
    connect(m_setPlus, &QPushButton::clicked, m_proto, &SerialProto::writeStoveIncSetPoint);
    connect(m_powerMinus, &QPushButton::clicked, m_proto, &SerialProto::writeStoveDecPower);
    connect(m_powerPlus, &QPushButton::clicked, m_proto, &SerialProto::writeStoveIncPower);

    /* Values from the stove */
    connect(m_proto, &SerialProto::updateAmbTemp, m_dial, &wDial::setValue);
    connect(m_proto, &SerialProto::updateSetTemp, this, [this](float t) {
        m_dial->setSetpoint(t);
        m_setValue->setText(QString::number(t, 'f', 1) + QChar(0x00B0));
    });
    connect(m_proto, &SerialProto::updateSmoke, this, [this](quint8 temp, quint16) {
        m_smokeValue->setText(QString("%1 %2C").arg(temp).arg(QChar(0x00B0)));
    });
    connect(m_proto, &SerialProto::updateStoveState, this, &MainPage::setStoveState);
    connect(m_proto, &SerialProto::updatePower, this, &MainPage::setPower);
    connect(m_proto, &SerialProto::updateStoveDateTime, this, &MainPage::setDateTime);
    connect(m_proto, &SerialProto::linkStateChanged, this, &MainPage::setLink);
    connect(m_proto, &SerialProto::updateChronoEnable, this, &MainPage::setChrono);

    setChrono(false);
    applyLook();
}

void MainPage::repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void MainPage::setStoveState(quint8 state, const QString &name)
{
    m_state = state;
    m_stateName = name;
    m_stateRead = true;
    applyLook();
}

/* Port closed or stove not answering: shown in place of the state until
 * the next state read */
void MainPage::setLink(quint8 linkState)
{
    m_link = linkState;
    if(SerialProto::LinkOk != linkState)
        m_stateRead = false;
    applyLook();
}

void MainPage::setChrono(bool on)
{
    m_chronoButton->setText(on ? "Chrono on" : "Chrono off");
    m_chronoButton->setIcon(tintedIcon("clock", on ? kBlue : kMuted, 20));
    m_chronoButton->setProperty("on", on);
    repolish(m_chronoButton);
}

void MainPage::setPower(quint8 setPower, quint8 flamePower)
{
    m_powerValue->setText(QString("%1<span style=\"font-size: 16px; color: #A3ABB3;\"> / %2</span>")
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
    m_chipIcon->setPixmap(tintedIcon(chipIcon, chipColor, 18).pixmap(18, 18));
    m_chipText->setText(chipText);
    m_stateChip->setProperty("look", chip);
    repolish(m_stateChip);
    repolish(m_chipText);

    /* Alarm: hint and Reset only (On/Off would write Starting or Final Cleaning) */
    const bool alarm = (Look::Alarm == m_look);
    QString hint;
    if(alarm)
    {
        if(m_state == SerialProto::PelletMissing)
            hint = "Refill the hopper, then press Reset";
        else if(m_state == SerialProto::IgnitionFailure)
            hint = "Clean the brazier, then press Reset";
        else
            hint = "Check the stove display, then press Reset";
    }
    m_hint->setText(hint);
    m_hint->setVisible(alarm);
    m_stateCard->setProperty("alarm", alarm);
    repolish(m_stateCard);

    /* On/Off: Turn on when Off, Turn off when Working, Reset in an alarm,
     * disabled otherwise (the stove is moving between states) */
    QString button = "Turn off";
    QString buttonIcon = "power";
    QColor buttonColor = kText;
    QString buttonLook = "off";
    bool enabled = false;
    switch(m_look)
    {
    case Look::Off:     button = "Turn on"; buttonColor = kDark; buttonLook = "on"; enabled = true; break;
    case Look::Active:  enabled = (m_state == SerialProto::Working); break;
    case Look::Alarm:   button = "Reset"; buttonIcon = "reset"; buttonLook = "reset"; enabled = true; break;
    default:            break;
    }
    if(!enabled && "on" != buttonLook)
        buttonColor = QColor("#555C63");
    m_onOffButton->setText(button);
    m_onOffButton->setIcon(tintedIcon(buttonIcon, buttonColor, 22));
    m_onOffButton->setProperty("look", buttonLook);
    m_onOffButton->setEnabled(enabled);
    repolish(m_onOffButton);

    /* Heat colors only while the stove is burning */
    const bool hot = (Look::Active == m_look);
    m_dial->setArcColor(hot ? kOrange : kGrey);
    m_bars->setActiveColor(hot ? kOrange : kGrey);
    m_flameValue->setProperty("hot", hot);
    repolish(m_flameValue);

    /* Link dot */
    QColor dot = kTeal;
    if(SerialProto::LinkPortClosed == m_link)
        dot = kRed;
    else if(SerialProto::LinkNoReply == m_link)
        dot = kOrange;
    m_linkButton->setIcon(tintedIcon("link", dot, 16));

    /* Setpoint and power -/+ only with the stove answering */
    const bool link = (SerialProto::LinkOk == m_link);
    for(QPushButton *b : { m_setMinus, m_setPlus, m_powerMinus, m_powerPlus })
        b->setEnabled(link);
}

void MainPage::onOffClicked()
{
    if(Look::Alarm == m_look)
    {
        m_proto->writeStoveStateOffForce();
        return;
    }

    const Look before = m_look;
    const bool on = (Look::Off == m_look);
    QMessageBox box(this);
    box.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    box.setText(on ? "Turn the stove on?" : "Turn the stove off?");
    QPushButton *action = box.addButton(on ? "Turn on" : "Turn off", QMessageBox::AcceptRole);
    box.addButton("Cancel", QMessageBox::RejectRole);
    box.exec();
    //The state may have changed while the question was open
    if(box.clickedButton() != action || m_look != before)
        return;
    if(on)
        m_proto->writeStoveStateOn();
    else
        m_proto->writeStoveStateOff();
}
