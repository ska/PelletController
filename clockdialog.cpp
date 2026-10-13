#include "clockdialog.h"

#include "wClockFace.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

enum { FieldDay, FieldMonth, FieldYear };

ClockDialog::ClockDialog(const QDateTime &start, QWidget *parent)
    : QDialog(parent)
{
    setObjectName("timePicker");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);

    const QDateTime dt = start.isValid() ? start : QDateTime(QDate(2026, 1, 1), QTime(12, 0));
    m_date = dt.date();
    m_hours = dt.time().hour();
    m_minutes = dt.time().minute();

    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(24, 20, 20, 20);
    root->setSpacing(20);

    QVBoxLayout *left = new QVBoxLayout;
    left->setSpacing(8);
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *title = new QLabel("Stove clock", this);
    title->setObjectName("pickerTitle");
    m_weekday = new QLabel(this);
    m_weekday->setObjectName("caption");
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_weekday);
    left->addLayout(titleRow);

    m_day = dateRow(left, "Day", FieldDay);
    m_month = dateRow(left, "Month", FieldMonth);
    m_year = dateRow(left, "Year", FieldYear);

    QHBoxLayout *time = new QHBoxLayout;
    time->setSpacing(4);
    m_hoursButton = new QPushButton(this);
    m_minutesButton = new QPushButton(this);
    QButtonGroup *group = new QButtonGroup(this);
    for(QPushButton *b : { m_hoursButton, m_minutesButton })
    {
        b->setObjectName("pickerPart");
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(104, 88);
        group->addButton(b);
    }
    m_hoursButton->setAccessibleName("Hours");
    m_minutesButton->setAccessibleName("Minutes");
    QLabel *colon = new QLabel(":", this);
    colon->setObjectName("pickerColon");
    time->addWidget(m_hoursButton);
    time->addWidget(colon);
    time->addWidget(m_minutesButton);
    time->addStretch();
    left->addLayout(time);
    left->addStretch();

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->setSpacing(12);
    QPushButton *cancel = new QPushButton("Cancel", this);
    QPushButton *set = new QPushButton("Set clock", this);
    set->setObjectName("pickerSave");
    for(QPushButton *b : { cancel, set })
    {
        b->setFocusPolicy(Qt::NoFocus);
        b->setMinimumHeight(56);
    }
    buttons->addWidget(cancel);
    buttons->addWidget(set);
    left->addLayout(buttons);
    root->addLayout(left, 1);

    m_face = new wClockFace(this);
    m_face->setMinuteStep(1);
    m_face->setFixedSize(300, 300);
    root->addWidget(m_face, 0, Qt::AlignVCenter);

    connect(m_hoursButton, &QPushButton::clicked, this, &ClockDialog::showHours);
    connect(m_minutesButton, &QPushButton::clicked, this, &ClockDialog::showMinutes);
    connect(m_face, &wClockFace::valueChanged, this, [this](int v) {
        if(wClockFace::Hours == m_face->mode())
            m_hours = v;
        else
            m_minutes = v;
        showValue();
    });
    connect(m_face, &wClockFace::picked, this, [this] {
        if(wClockFace::Hours == m_face->mode())
            showMinutes();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(set, &QPushButton::clicked, this, &QDialog::accept);

    showHours();
}

/* "Day   [-]  13  [+]" */
QLabel *ClockDialog::dateRow(QVBoxLayout *box, const QString &name, int field)
{
    QHBoxLayout *row = new QHBoxLayout;
    row->setSpacing(10);
    QLabel *caption = new QLabel(name, this);
    caption->setObjectName("caption");
    caption->setFixedWidth(56);
    QPushButton *minus = new QPushButton(QString(QChar(0x2212)), this);
    QPushButton *plus = new QPushButton("+", this);
    for(QPushButton *b : { minus, plus })
    {
        b->setObjectName("stepButton");
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(64, 52);
        b->setAutoRepeat(true);
        b->setAutoRepeatDelay(400);
        b->setAutoRepeatInterval(120);
    }
    minus->setAccessibleName(name + " earlier");
    plus->setAccessibleName(name + " later");
    QLabel *value = new QLabel(this);
    value->setObjectName("dateValue");
    value->setAlignment(Qt::AlignCenter);
    value->setFixedWidth(92);
    row->addWidget(caption);
    row->addWidget(minus);
    row->addWidget(value);
    row->addWidget(plus);
    row->addStretch();
    box->addLayout(row);
    connect(minus, &QPushButton::clicked, this, [this, field] { stepDate(field, -1); });
    connect(plus, &QPushButton::clicked, this, [this, field] { stepDate(field, 1); });
    return value;
}

/* Day and month wrap inside their range; the day follows the month length */
void ClockDialog::stepDate(int field, int delta)
{
    int y = m_date.year();
    int m = m_date.month();
    int d = m_date.day();
    if(FieldYear == field)
        y = qBound(2000, y + delta, 2099);
    else if(FieldMonth == field)
        m = (m - 1 + 12 + delta) % 12 + 1;
    const int days = QDate(y, m, 1).daysInMonth();
    if(FieldDay == field)
        d = (d - 1 + days + delta) % days + 1;
    m_date = QDate(y, m, qMin(d, days));
    showValue();
}

void ClockDialog::showHours()
{
    m_hoursButton->setChecked(true);
    m_face->setMode(wClockFace::Hours, m_hours);
    showValue();
}

void ClockDialog::showMinutes()
{
    m_minutesButton->setChecked(true);
    m_face->setMode(wClockFace::Minutes, m_minutes);
    showValue();
}

void ClockDialog::showValue()
{
    m_day->setText(QString::number(m_date.day()));
    m_month->setText(QLocale::c().monthName(m_date.month(), QLocale::ShortFormat));
    m_year->setText(QString::number(m_date.year()));
    m_weekday->setText(QLocale::c().dayName(m_date.dayOfWeek()));
    m_hoursButton->setText(QString("%1").arg(m_hours, 2, 10, QChar('0')));
    m_minutesButton->setText(QString("%1").arg(m_minutes, 2, 10, QChar('0')));
}

QDateTime ClockDialog::dateTime() const
{
    return QDateTime(m_date, QTime(m_hours, m_minutes));
}
