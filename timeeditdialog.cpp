#include "timeeditdialog.h"
#include "ui_timeeditdialog.h"

TimeEditDialog::TimeEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TimeEditDialog)
{
    ui->setupUi(this);
    ui->timepicker->setTensOfMins(0);
    setWindowFlags( windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );

}

TimeEditDialog::~TimeEditDialog()
{
    delete ui;
}

bool TimeEditDialog::event(QEvent *event)
{
    //qDebug() << Q_FUNC_INFO << " -> " << event->type();
    //Qualsiasi event mouse mi resetta il timer
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    case QEvent::Leave:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        //qDebug() << "TimeEditDialog::event: " << QTime::currentTime();
        emit touchMouseEvent();
        //emit
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

quint32 TimeEditDialog::tensOfMins() const
{
    return m_tensOfMins;
}

void TimeEditDialog::setTensOfMins(quint32 newTensOfMins)
{
    qDebug() << Q_FUNC_INFO << newTensOfMins;
    if (m_tensOfMins == newTensOfMins)
        return;
    m_tensOfMins = newTensOfMins;

    ui->timepicker->setTensOfMins(newTensOfMins);

}

void TimeEditDialog::on_saveBtn_clicked()
{
    setTensOfMins(ui->timepicker->getTensOfMins() );
    emit tensOfMinsUpdated( (quint8)m_tensOfMins );
}


void TimeEditDialog::on_cancelBtn_clicked()
{
    close();
}

