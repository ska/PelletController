#include "timepicker.h"

#include "wClockFace.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#define SLOT_OFF    (144)

TimePickerDialog::TimePickerDialog(const QString &title, quint8 tensOfMins, QWidget *parent)
    : QDialog(parent)
{
    setObjectName("timePicker");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);

    //An "off" time starts from 07:00, just a starting point to change
    if(tensOfMins < SLOT_OFF)
    {
        m_hours = tensOfMins / 6;
        m_minutes = (tensOfMins % 6) * 10;
    }

    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(24, 20, 20, 20);
    root->setSpacing(24);

    /* Left: title, time, buttons */
    QVBoxLayout *left = new QVBoxLayout;
    left->setSpacing(14);
    QLabel *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("pickerTitle");
    titleLabel->setWordWrap(true);
    left->addWidget(titleLabel);

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

    QPushButton *off = new QPushButton("Turn slot off", this);
    off->setObjectName("pickerOff");
    QPushButton *cancel = new QPushButton("Cancel", this);
    QPushButton *save = new QPushButton("Save", this);
    save->setObjectName("pickerSave");
    for(QPushButton *b : { off, cancel, save })
    {
        b->setFocusPolicy(Qt::NoFocus);
        b->setMinimumHeight(56);
    }
    left->addWidget(off);
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->setSpacing(12);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    left->addLayout(buttons);
    root->addLayout(left, 1);

    /* Right: clock face */
    m_face = new wClockFace(this);
    m_face->setFixedSize(300, 300);
    root->addWidget(m_face);

    connect(m_hoursButton, &QPushButton::clicked, this, &TimePickerDialog::showHours);
    connect(m_minutesButton, &QPushButton::clicked, this, &TimePickerDialog::showMinutes);
    connect(m_face, &wClockFace::valueChanged, this, [this](int v) {
        if(wClockFace::Hours == m_face->mode())
            m_hours = v;
        else
            m_minutes = v;
        showValue();
    });
    //Hour chosen: on to the minutes, as on a phone
    connect(m_face, &wClockFace::picked, this, [this] {
        if(wClockFace::Hours == m_face->mode())
            showMinutes();
    });
    connect(off, &QPushButton::clicked, this, [this] { m_value = SLOT_OFF; accept(); });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, [this] { m_value = quint8(m_hours * 6 + m_minutes / 10); accept(); });

    showHours();
}

void TimePickerDialog::showHours()
{
    m_hoursButton->setChecked(true);
    m_face->setMode(wClockFace::Hours, m_hours);
    showValue();
}

void TimePickerDialog::showMinutes()
{
    m_minutesButton->setChecked(true);
    m_face->setMode(wClockFace::Minutes, m_minutes);
    showValue();
}

void TimePickerDialog::showValue()
{
    m_hoursButton->setText(QString("%1").arg(m_hours, 2, 10, QChar('0')));
    m_minutesButton->setText(QString("%1").arg(m_minutes, 2, 10, QChar('0')));
}
