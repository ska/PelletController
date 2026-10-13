#include "chronopage.h"

#include "serialproto.h"
#include "timepicker.h"
#include "uiicons.h"
#include "wToggle.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#define SLOT_OFF        (144)
#define WEEKLY_STRIDE   (SerialProto::chronoSet_2_OnIndex - SerialProto::chronoSet_1_OnIndex)

namespace {
const char *const kDays[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

QString timeText(int value)
{
    if(value < 0)
        return QStringLiteral("..:..");
    if(value == SLOT_OFF)
        return QStringLiteral("--:--");
    if(value > SLOT_OFF)
        return QStringLiteral("??:??");
    return QString("%1:%2").arg(value / 6, 2, 10, QChar('0')).arg((value % 6) * 10, 2, 10, QChar('0'));
}

void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}
}

ChronoPage::ChronoPage(SerialProto *proto, QWidget *parent)
    : QWidget(parent), m_proto(proto), m_values(SerialProto::LastIndex, -1)
{
    setObjectName("chronoPage");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    /* Header: Back, title, chrono on/off */
    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(16);
    QPushButton *back = new QPushButton("Back", this);
    back->setObjectName("backButton");
    back->setFocusPolicy(Qt::NoFocus);
    back->setIcon(tintedIcon("back", QColor("#F3F4F5"), 22));
    back->setIconSize(QSize(22, 22));
    QLabel *title = new QLabel("Chrono", this);
    title->setObjectName("pageTitle");
    QLabel *chronoLabel = new QLabel("Chrono", this);
    chronoLabel->setObjectName("switchLabel");
    wToggle *chronoToggle = new wToggle(this);
    chronoToggle->setAccessibleName("Chrono on/off");
    addToggle(chronoToggle, SerialProto::chronoEnableIndex, true);
    header->addWidget(back);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(chronoLabel);
    header->addWidget(chronoToggle);
    root->addLayout(header);
    connect(back, &QPushButton::clicked, this, &ChronoPage::backRequested);

    /* Tabs */
    QHBoxLayout *tabRow = new QHBoxLayout;
    tabRow->setSpacing(0);
    m_tabs = new QStackedWidget(this);
    QButtonGroup *tabGroup = new QButtonGroup(this);
    const QStringList names = { "Weekend", "Daily", "Weekly" };
    for(int i = 0; i < names.size(); i++)
    {
        QPushButton *tab = new QPushButton(names.at(i), this);
        tab->setObjectName("tabButton");
        tab->setCheckable(true);
        tab->setFocusPolicy(Qt::NoFocus);
        tab->setChecked(0 == i);
        tabGroup->addButton(tab);
        tabRow->addWidget(tab);
        connect(tab, &QPushButton::clicked, m_tabs, [this, i] { m_tabs->setCurrentIndex(i); });
    }
    root->addLayout(tabRow);

    m_tabs->addWidget(programTab("Weekend program", "Saturday and Sunday, two slots",
                                 SerialProto::chronoWkE_EnableIndex,
                                 { SerialProto::chronoWkE_1_OnIndex, SerialProto::chronoWkE_1_OffIndex,
                                   SerialProto::chronoWkE_2_OnIndex, SerialProto::chronoWkE_2_OffIndex }, true));
    m_tabs->addWidget(programTab("Daily program", "Every day, two slots",
                                 SerialProto::chronoDay_EnableIndex,
                                 { SerialProto::chronoDay_1_OnIndex, SerialProto::chronoDay_1_OffIndex,
                                   SerialProto::chronoDay_2_OnIndex, SerialProto::chronoDay_2_OffIndex }, true));

    /* Weekly: read only, one slot at a time */
    QFrame *weekly = new QFrame(this);
    weekly->setObjectName("card");
    QVBoxLayout *w = new QVBoxLayout(weekly);
    w->setContentsMargins(20, 16, 20, 16);
    w->setSpacing(12);
    QHBoxLayout *wHead = new QHBoxLayout;
    QVBoxLayout *wTitles = new QVBoxLayout;
    wTitles->setSpacing(0);
    QLabel *wTitle = new QLabel("Weekly program", weekly);
    wTitle->setObjectName("programTitle");
    wTitles->addWidget(wTitle);
    wTitles->addWidget(new QLabel("Four slots on chosen days", weekly));
    wTitles->itemAt(1)->widget()->setObjectName("caption");
    wHead->addLayout(wTitles);
    wHead->addStretch();
    wToggle *weeklyToggle = new wToggle(weekly);
    weeklyToggle->setAccessibleName("Weekly program on/off");
    addToggle(weeklyToggle, SerialProto::chronoSet_EnableIndex, true);
    wHead->addWidget(weeklyToggle);
    w->addLayout(wHead);

    QFrame *wRow = new QFrame(weekly);
    wRow->setObjectName("slotRow");
    QHBoxLayout *wr = new QHBoxLayout(wRow);
    wr->setContentsMargins(16, 12, 16, 12);
    wr->setSpacing(10);
    QLabel *slotCaption = new QLabel("Slot", wRow);
    slotCaption->setObjectName("slotName");
    wr->addWidget(slotCaption);
    QButtonGroup *slotGroup = new QButtonGroup(this);
    for(int s = 0; s < 4; s++)
    {
        QPushButton *b = new QPushButton(QString::number(s + 1), wRow);
        b->setObjectName("slotSelect");
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(52, 52);
        slotGroup->addButton(b);
        b->setChecked(0 == s);
        m_weeklySlotButtons[s] = b;
        wr->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, s] { showWeeklySlot(s); });
    }
    wr->addSpacing(12);
    for(int s = 0; s < 4; s++)
    {
        const int base = SerialProto::chronoSet_1_OnIndex + s * WEEKLY_STRIDE;
        m_timeNames.insert(base, QString("Weekly slot %1 start").arg(s + 1));
        m_timeNames.insert(base + 1, QString("Weekly slot %1 stop").arg(s + 1));
    }
    QPushButton *wStart = timeButton("Start", -1, true, wRow);
    m_weeklyStart = wStart->findChild<QLabel *>("timeValue");
    connect(wStart, &QPushButton::clicked, this, [this] { editTime(weeklyBase()); });
    QLabel *wArrow = new QLabel(wRow);
    wArrow->setPixmap(tintedIcon("arrow", QColor("#7D858D")).pixmap(24, 24));
    QPushButton *wStop = timeButton("Stop", -1, true, wRow);
    m_weeklyStop = wStop->findChild<QLabel *>("timeValue");
    connect(wStop, &QPushButton::clicked, this, [this] { editTime(weeklyBase() + 1); });
    wr->addWidget(wStart);
    wr->addWidget(wArrow);
    wr->addWidget(wStop);
    wr->addStretch();
    w->addWidget(wRow);

    QHBoxLayout *days = new QHBoxLayout;
    days->setSpacing(8);
    for(int d = 0; d < 7; d++)
    {
        QPushButton *day = new QPushButton(kDays[d], weekly);
        day->setObjectName("dayChip");
        day->setCheckable(true);
        day->setFocusPolicy(Qt::NoFocus);
        day->setFixedHeight(52);
        m_weeklyDays[d] = day;
        days->addWidget(day);
        //clicked() only from the user: the days read are never written back
        connect(day, &QPushButton::clicked, this, [this, d](bool on) {
            const int index = weeklyBase() + 2 + d;
            if(m_proto->writeChrono(SerialProto::RequestsIndex(index), on ? 1 : 0))
                setValue(quint8(index), on ? 1 : 0);
        });
    }
    w->addLayout(days);
    w->addStretch();
    m_tabs->addWidget(weekly);
    root->addWidget(m_tabs, 1);

    connect(m_proto, &SerialProto::chronoRead, this, &ChronoPage::setValue);
    showWeeklySlot(0);
}

/* Weekend or daily card: title, program on/off, two slot rows */
QWidget *ChronoPage::programTab(const QString &title, const QString &subtitle, int enableIndex,
                                const QVector<int> &slotIndexes, bool editable)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");
    QVBoxLayout *v = new QVBoxLayout(card);
    v->setContentsMargins(20, 16, 20, 16);
    v->setSpacing(12);

    QHBoxLayout *head = new QHBoxLayout;
    QVBoxLayout *titles = new QVBoxLayout;
    titles->setSpacing(0);
    QLabel *t = new QLabel(title, card);
    t->setObjectName("programTitle");
    QLabel *s = new QLabel(subtitle, card);
    s->setObjectName("caption");
    titles->addWidget(t);
    titles->addWidget(s);
    head->addLayout(titles);
    head->addStretch();
    wToggle *toggle = new wToggle(card);
    toggle->setAccessibleName(title + " on/off");
    addToggle(toggle, enableIndex, editable);
    head->addWidget(toggle);
    v->addLayout(head);

    const QString program = title.section(' ', 0, 0);       //"Weekend", "Daily"
    for(int slot = 0; slot < 2; slot++)
    {
        QFrame *row = new QFrame(card);
        row->setObjectName("slotRow");
        QHBoxLayout *r = new QHBoxLayout(row);
        r->setContentsMargins(16, 12, 16, 12);
        r->setSpacing(16);
        QLabel *name = new QLabel(QString("Slot %1").arg(slot + 1), row);
        name->setObjectName("slotName");
        name->setFixedWidth(64);
        const int on = slotIndexes.at(slot * 2);
        const int off = slotIndexes.at(slot * 2 + 1);
        m_timeNames.insert(on, QString("%1 slot %2 start").arg(program).arg(slot + 1));
        m_timeNames.insert(off, QString("%1 slot %2 stop").arg(program).arg(slot + 1));
        QLabel *arrow = new QLabel(row);
        arrow->setPixmap(tintedIcon("arrow", QColor("#7D858D")).pixmap(24, 24));
        r->addWidget(name);
        r->addWidget(timeButton("Start", on, editable, row));
        r->addWidget(arrow);
        r->addWidget(timeButton("Stop", off, editable, row));
        r->addStretch();
        v->addWidget(row);
    }
    v->addStretch();
    return card;
}

/* Start or stop button: caption and time; index -1 = not bound (weekly) */
QPushButton *ChronoPage::timeButton(const QString &caption, int index, bool editable, QWidget *parent)
{
    QPushButton *b = new QPushButton(parent);
    b->setObjectName("timeButton");
    b->setFocusPolicy(Qt::NoFocus);
    b->setFixedSize(132, 64);
    b->setAccessibleName(index >= 0 ? m_timeNames.value(index, caption) : caption);
    QVBoxLayout *l = new QVBoxLayout(b);
    l->setContentsMargins(16, 6, 16, 6);
    l->setSpacing(0);
    QLabel *c = new QLabel(caption, b);
    c->setObjectName("timeCaption");
    QLabel *value = new QLabel(timeText(-1), b);
    value->setObjectName("timeValue");
    for(QLabel *lab : { c, value })
        lab->setAttribute(Qt::WA_TransparentForMouseEvents);
    l->addWidget(c);
    l->addWidget(value);
    b->setEnabled(editable);
    if(index >= 0)
    {
        m_timeLabels.insert(index, value);
        connect(b, &QPushButton::clicked, this, [this, index] { editTime(index); });
    }
    return b;
}

/* clicked(bool) comes only from the user: a value read never goes back to
 * the stove (it would rewrite the EEPROM at every poll) */
void ChronoPage::addToggle(wToggle *toggle, int index, bool editable)
{
    m_toggles.insert(index, toggle);
    toggle->setEnabled(editable);
    connect(toggle, &wToggle::clicked, this, [this, index](bool on) {
        m_proto->writeChrono(SerialProto::RequestsIndex(index), on ? 1 : 0);
    });
}

void ChronoPage::setValue(quint8 index, quint8 value)
{
    if(index >= m_values.size())
        return;
    m_values[index] = value;

    if(wToggle *t = m_toggles.value(index))
        t->setChecked(value != 0);
    if(QLabel *l = m_timeLabels.value(index))
    {
        l->setText(timeText(value));
        l->setProperty("off", value >= SLOT_OFF);
        repolish(l);
    }
    if(index >= SerialProto::chronoSet_1_OnIndex)
        showWeeklySlot(m_weeklySlot);
}

int ChronoPage::weeklyBase() const
{
    return SerialProto::chronoSet_1_OnIndex + m_weeklySlot * WEEKLY_STRIDE;
}

void ChronoPage::showWeeklySlot(int slot)
{
    m_weeklySlot = slot;
    const int base = weeklyBase();
    for(QLabel *l : { m_weeklyStart, m_weeklyStop })
    {
        const int v = m_values.at(base + (l == m_weeklyStop ? 1 : 0));
        l->setText(timeText(v));
        l->setProperty("off", v < 0 || v >= SLOT_OFF);
        repolish(l);
    }
    for(int d = 0; d < 7; d++)
        m_weeklyDays[d]->setChecked(m_values.at(base + 2 + d) == 1);
}

void ChronoPage::editTime(int index)
{
    const int current = m_values.at(index) < 0 ? SLOT_OFF : m_values.at(index);
    TimePickerDialog picker(m_timeNames.value(index), quint8(current), this);
    m_picker = &picker;
    const int result = picker.exec();
    m_picker = nullptr;
    if(QDialog::Accepted != result)
        return;
    if(m_proto->writeChrono(SerialProto::RequestsIndex(index), picker.value()))
        setValue(quint8(index), picker.value());   //Shown at once, the chrono polling confirms it
}

void ChronoPage::closePicker()
{
    if(m_picker)
        m_picker->reject();
}
