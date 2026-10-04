#include "wTimePicker.h"
#include "ui_wTimePicker.h"

wTimePicker::wTimePicker(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimePicker)
{
    ui->setupUi(this);
    m_am = true;

    showHours();
    on_amClbl_clicked();
}

wTimePicker::~wTimePicker()
{
    delete ui;
}

void wTimePicker::on_Hours_clicked()
{
    ui->clockwidget->setStartAngle(90);
    ui->clockwidget->setStartIndex(0);
    ui->clockwidget->setIndexesGap(1);
    ui->clockwidget->setNumberOfSteps(12);
    ui->clockwidget->setCurrentIndex(ui->Hours->text().toInt());

    ui->Hours->setStyleSheet("QLabel { color : black; font-weight: bold; font-family: \"Arial\"; font: 16pt; border-radius: 4px; border: 2px solid #4F0099; background-color: #334F0099; }");
    ui->Mins->setStyleSheet( "QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; }");
}


void wTimePicker::on_Mins_clicked()
{
    ui->clockwidget->setStartAngle(90);
    ui->clockwidget->setStartIndex(0);
    ui->clockwidget->setIndexesGap(10);
    ui->clockwidget->setNumberOfSteps(6);
    ui->clockwidget->setCurrentIndex(ui->Mins->text().toInt()/10);

    ui->Mins->setStyleSheet( "QLabel { color : black; font-weight: bold; font-family: \"Arial\"; font: 16pt; border-radius: 4px; border: 2px solid #4F0099; background-color: #334F0099; }");
    ui->Hours->setStyleSheet("QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; }");
}


void wTimePicker::on_clockwidget_currentIndexChanged()
{
    if(12 == ui->clockwidget->numberOfSteps())
    {
        ui->Hours->setText( ui->clockwidget->currentValue() );
        //showMins();
    }
    else if(6 == ui->clockwidget->numberOfSteps())
    {
        ui->Mins->setText( ui->clockwidget->currentValue() );
    }
}


void wTimePicker::on_amClbl_clicked()
{
    m_am = true;
    ui->amClbl->setStyleSheet( "QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; border-radius: 4px; border: 2px solid #4F0099; background-color: #334F0099; }");
    ui->pmClbl->setStyleSheet(" QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; }");
}


void wTimePicker::on_pmClbl_clicked()
{
    m_am = false;
    ui->pmClbl->setStyleSheet( "QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; border-radius: 4px; border: 2px solid #4F0099; background-color: #334F0099; }");
    ui->amClbl->setStyleSheet(" QLabel { color : black; font-weight: normal; font-family: \"Arial\"; font: 16pt; }");
}

QTime wTimePicker::getTime() const
{
    return QTime( (quint8)ui->Hours->text().toInt()+(!m_am*12), (ui->Mins->text().toInt()));
}

void wTimePicker::setTime(const QTime &newTime)
{
    qDebug() << "newTime: " << newTime;
    quint8 h = (quint8)newTime.hour();
    if(h>12)
    {
        h-=12;
        m_am = false;
        on_pmClbl_clicked();
    }
    ui->Hours->setText(QString("%1").arg(h));

    quint8 m = (newTime.minute());
    ui->Mins->setText(QString("%1").arg((m/10)*10));
    qDebug() << "H: " << h << "M: " << m << "AM: " << m_am;
    showHours();
}

void wTimePicker::showHours()
{
    on_Hours_clicked();
}

void wTimePicker::showMins()
{
    on_Mins_clicked();
}

quint32 wTimePicker::getTensOfMins() const
{
    return (quint8)((quint8)(!m_am*12*6) + (quint8)(ui->Hours->text().toInt()*6) + (quint8)(ui->Mins->text().toInt()/10));
}

void wTimePicker::setTensOfMins(quint32 newTensOfMins)
{
    qDebug() << Q_FUNC_INFO << newTensOfMins;
    newTensOfMins = newTensOfMins % 144;
    m_am = true;
    quint8 h = (quint8)newTensOfMins/6;
    if(h>12)
    {
        h-=12;
        m_am = false;
        on_pmClbl_clicked();
    }

    qDebug() << Q_FUNC_INFO << " Hours: " << h;
    newTensOfMins -= (h*6);
    qDebug() << Q_FUNC_INFO << " newTensOfMins: " << newTensOfMins;

    ui->Hours->setText(QString("%1").arg(h));
    if(12 == ui->clockwidget->numberOfSteps())
        ui->clockwidget->setCurrentIndex( h );

    quint8 m = ((newTensOfMins)%6)*10;
    ui->Mins->setText(QString("%1").arg(m));

    qDebug() << Q_FUNC_INFO << " Mins: " << m;
    if(6 == ui->clockwidget->numberOfSteps())
        ui->clockwidget->setValue( m );//SetCurrentValue
}

