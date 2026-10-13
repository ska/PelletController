#include "diagpage.h"

#include "common.h"
#include "homeassistant.h"
#include "serialproto.h"
#include "uiicons.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace {
QFrame *card(const QString &title, QGridLayout **grid, QWidget *parent)
{
    QFrame *f = new QFrame(parent);
    f->setObjectName("card");
    QVBoxLayout *v = new QVBoxLayout(f);
    v->setContentsMargins(20, 16, 20, 16);
    v->setSpacing(10);
    QLabel *t = new QLabel(title, f);
    t->setObjectName("programTitle");
    v->addWidget(t);
    *grid = new QGridLayout;
    (*grid)->setHorizontalSpacing(16);
    (*grid)->setVerticalSpacing(6);
    (*grid)->setColumnStretch(1, 1);
    v->addLayout(*grid);
    v->addStretch();
    return f;
}
}

DiagPage::DiagPage(SerialProto *proto, QWidget *parent)
    : QWidget(parent), m_proto(proto)
{
    setObjectName("diagPage");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(16);
    QPushButton *back = new QPushButton("Back", this);
    back->setObjectName("backButton");
    back->setFocusPolicy(Qt::NoFocus);
    back->setIcon(tintedIcon("back", QColor("#F3F4F5"), 22));
    back->setIconSize(QSize(22, 22));
    QLabel *title = new QLabel("Diagnostics", this);
    title->setObjectName("pageTitle");
    header->addWidget(back);
    header->addWidget(title);
    header->addStretch();
    root->addLayout(header);
    connect(back, &QPushButton::clicked, this, &DiagPage::backRequested);

    QHBoxLayout *cards = new QHBoxLayout;
    cards->setSpacing(12);

    QGridLayout *g;
    cards->addWidget(card("Serial link", &g, this), 1);
    m_port = addRow(g, "Port");
    m_link = addRow(g, "Link");
    m_tx = addRow(g, "Requests (Tx)");
    m_rx = addRow(g, "Valid replies (Rx)");
    m_err = addRow(g, "Bad replies (Err)");
    m_chk = addRow(g, "Checksum errors (Chk)");
    m_tmo = addRow(g, "No reply (Tmo)");
    m_rate = addRow(g, "Valid");
    QLabel *ha = new QLabel("Home Assistant");
    ha->setObjectName("diagSection");
    g->addWidget(ha, g->rowCount(), 0, 1, 2);
    m_mqtt = addRow(g, "MQTT");
    m_broker = addRow(g, "Broker");
    m_published = addRow(g, "Messages sent");

    cards->addWidget(card("Stove", &g, this), 1);
    m_state = addRow(g, "State");
    m_powerInUse = addRow(g, "Power in use");
    m_smoke = addRow(g, "Smoke temperature");
    m_fan = addRow(g, "Smoke fan");
    m_clock = addRow(g, "Stove clock");
    QLabel *version = addRow(g, "Panel version");
    version->setText(SW_VER);
    QLabel *build = addRow(g, "Build");
    build->setText(QString("%1, %2").arg(PC_GIT_COMMIT, PC_BUILD_TIMESTAMP));
    build->setWordWrap(true);
    root->addLayout(cards, 1);

    m_port->setText(m_proto->serPort());
    setHomeAssistant(nullptr);
    setLink(m_proto->linkState());

    connect(m_proto, &SerialProto::linkStateChanged, this, &DiagPage::setLink);
    connect(m_proto, &SerialProto::updateStats, this, &DiagPage::setStats);
    connect(m_proto, &SerialProto::serialOpened, this, [this] { m_port->setText(m_proto->serPort()); });
    connect(m_proto, &SerialProto::updateStoveState, this, [this](quint8 state, const QString &name) {
        m_state->setText(QString("%1 (%2)").arg(name).arg(state));
    });
    connect(m_proto, &SerialProto::updatePowerInUse, this, [this](quint8 p) {
        m_powerInUse->setText(p >= POWER_MIN && p <= POWER_MAX ? QString::number(p) : QString("none (%1)").arg(p, 2, 16, QChar('0')));
    });
    connect(m_proto, &SerialProto::updateSmoke, this, [this](quint8 temp, quint16 rpm) {
        m_smoke->setText(QString("%1 %2C").arg(temp).arg(QChar(0x00B0)));
        m_fan->setText(rpm ? QString("%1 rpm").arg(rpm) : QString("stopped"));
    });
    connect(m_proto, &SerialProto::updateStoveDateTime, this, [this](const QDateTime &dt) {
        m_clock->setText(QLocale::c().toString(dt, "ddd dd/MM/yyyy HH:mm"));
    });
}

QLabel *DiagPage::addRow(QGridLayout *grid, const QString &name)
{
    const int row = grid->rowCount();
    QLabel *n = new QLabel(name);
    n->setObjectName("caption");
    QLabel *v = new QLabel("--");
    v->setObjectName("diagValue");
    grid->addWidget(n, row, 0);
    grid->addWidget(v, row, 1);
    return v;
}

void DiagPage::setLink(quint8 linkState)
{
    QString text = "OK";
    QString look = "ok";
    if(SerialProto::LinkPortClosed == linkState)
    {
        text = "No serial port";
        look = "error";
    } else if(SerialProto::LinkNoReply == linkState) {
        text = "No reply";
        look = "warning";
    }
    m_link->setText(text);
    m_link->setProperty("look", look);
    m_link->style()->unpolish(m_link);
    m_link->style()->polish(m_link);
}

void DiagPage::setStats(quint32 tx, quint32 rx, quint32 err, quint32 chk, quint32 tmo)
{
    m_tx->setText(QString::number(tx));
    m_rx->setText(QString::number(rx));
    m_err->setText(QString::number(err));
    m_chk->setText(QString::number(chk));
    m_tmo->setText(QString::number(tmo));
    m_rate->setText(tx ? QString("%1 %").arg(100.0 * rx / tx, 0, 'f', 1) : QString("--"));
}

void DiagPage::setHomeAssistant(HomeAssistant *ha)
{
    if(!ha)
    {
        m_mqtt->setText("off (no broker in setting.ini)");
        m_mqtt->setProperty("look", "");
        m_broker->setText("--");
        m_published->setText("--");
        return;
    }
    m_broker->setText(ha->broker());
    auto showState = [this](int state) {
        const bool up = (QMqttClient::Connected == state);
        m_mqtt->setText(up ? "connected" : (QMqttClient::Connecting == state ? "connecting" : "disconnected"));
        m_mqtt->setProperty("look", up ? "ok" : "warning");
        m_mqtt->style()->unpolish(m_mqtt);
        m_mqtt->style()->polish(m_mqtt);
    };
    showState(ha->clientState());
    m_published->setText(QString::number(ha->published()));
    connect(ha, &HomeAssistant::clientStateChanged, this, showState);
    connect(ha, &HomeAssistant::publishedChanged, this, [this](quint32 n) { m_published->setText(QString::number(n)); });
}
