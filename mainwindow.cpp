#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug("%s tid=%#llx ", __PRETTY_FUNCTION__, (quintptr) QThread::currentThreadId() );
    /* Load Stylesheet */
    QFile file(":/qss/default.qss");
    file.open( QFile::ReadOnly );
    qApp->setStyleSheet( QString::fromLatin1( file.readAll() ));
    ui->setupUi(this);

    /* Init Gui */
    ui->ambTempCSym->setText("C");
    ui->ambTempDegSym->setText("°");
    ui->ambTemp->setText("xx.x");
    ui->setTempCSym->setText("C");
    ui->setTempDegSym->setText("°");
    ui->setTemp->setText("xx.x");
    ui->state->setText("xx");
    ui->powerLab->setText("Power setted ");
    ui->setPower->setText("xx");
    ui->flameLab->setText("Power flame ");
    ui->setPowerPerc->setText("FEF%");

    ui->stateBtn->setIcon(QIcon(":/icons/power.svg"));
    ui->stateBtn->setIconSize(QSize(30, 30));
    ui->stateBtn->setText("Off");
    ui->stateOffForceBtn->setVisible(false);


    m_serProto = SerialProto::getInstance();
    connect(m_serProto, SIGNAL(updateAmbTemp(float)),                this, SLOT(updateAmbTemp(float)));
    connect(m_serProto, SIGNAL(updateSetTemp(float)),                this, SLOT(updateSetTemp(float)));
    connect(m_serProto, SIGNAL(updateStoveState(quint8,QString)),    this, SLOT(updateStoveState(quint8,QString)));
    connect(m_serProto, SIGNAL(updatePower(quint8,quint8)),          this, SLOT(updatePower(quint8,quint8)));
    connect(m_serProto, SIGNAL(updateStoveDateTime(QDateTime)),      this, SLOT(updateStoveDateTime(QDateTime)));
    connect(m_serProto, SIGNAL(updateStats(quint32,quint32,quint32)),this, SLOT(updateStats(quint32,quint32,quint32)));


    connect(ui->stateBtn,           SIGNAL(released()),              this, SLOT(handleStatteBtn()));
    connect(ui->stateOffForceBtn,   SIGNAL(released()),              this, SLOT(handleStateForceBtn()));
    connect(ui->powerMinBtn,        SIGNAL(released()),              this, SLOT(handlePowerMinBtn()));
    connect(ui->powerPlusBtn,       SIGNAL(released()),              this, SLOT(handlePowerPlusBtn()));

    connect(ui->setPointMinBtn,     SIGNAL(released()),              this, SLOT(handleSetPointMinBtn()));
    connect(ui->setPointPlusBtn,    SIGNAL(released()),              this, SLOT(handleSetPointPlusBtn()));

    m_serProto->openSerPort();
    m_serProto->startSerLoop();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateAmbTemp(float f)
{
    ui->ambTemp->setText(QString::number(f, 'f', 1) );
}

void MainWindow::updateSetTemp(float f)
{
    ui->setTemp->setText(QString::number(f, 'f', 1) );
}

void MainWindow::updateStoveState(quint8 u, QString s)
{
    ui->state->setText(s);


    switch (u) {
    case SerialProto::StoveState::Off:
        ui->stateBtn->setText("On");
        ui->stateBtn->setEnabled(true);
        break;
    case SerialProto::StoveState::Working:
        ui->stateBtn->setText("Off");
        ui->stateBtn->setEnabled(true);
        break;

    case SerialProto::StoveState::PelletMissing:
    case SerialProto::StoveState::IgnitionFailure:
    case SerialProto::StoveState::Alarm:
        ui->stateBtn->setText("Off");
        ui->stateBtn->setEnabled(true);
        break;

    /*
    case     Starting:
    case     PelletLoading:
    case     Ignition:
    case     BrazierCleaning:
    case     FinalCleaning:
    case     Standby:
    case     PelletMissing:
    case     IgnitionFailure:
    case     Alarm:
        break;
    */
    default:
        ui->stateBtn->setEnabled(false);
        ui->stateBtn->setText("Off");
        break;
    }

}

void MainWindow::updatePower(quint8 set, quint8 flame)
{
    ui->setPower->setText(QString::number(set));
    ui->setPowerPerc->setText(QString("%1 %").arg(QString::number(flame)));
}

void MainWindow::updateStoveDateTime(QDateTime dt)
{
    ui->datetime->setText( QString("%1 - %2")
                              .arg(dt.toString("dd/MM/yyyy"))
                              .arg(dt.toString("hh:mm"))
                          );
}

void MainWindow::updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors)
{
    ui->serialStats->setText(QString("Tx: %1 Rx: %2 Err: %3")
                                 .arg(txMessages)
                                 .arg(rxMessages)
                                 .arg(rxErrors));
}

void MainWindow::handleStatteBtn()
{
    QMessageBox msgBox;
    msgBox.setWindowFlags(Qt::FramelessWindowHint |Qt::WindowStaysOnTopHint);

    if( ui->stateBtn->text() == "Off" )
    {
        msgBox.setText("Are you sure you want to turn OFF the stove?");
        QPushButton *actionButton = msgBox.addButton(tr("Turn OFF"), QMessageBox::ActionRole);
        QPushButton *abortButton =  msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);
        msgBox.exec();
        if (msgBox.clickedButton() == actionButton)
        {
            qDebug() << "Turn OFF";
            m_serProto->writeStoveStateOff();
        }
    }
    else if( ui->stateBtn->text() == "On" )
    {
        msgBox.setText("Are you sure you want to turn ON the stove?");
        QPushButton *actionButton = msgBox.addButton(tr("Turn ON"), QMessageBox::ActionRole);
        QPushButton *abortButton = msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);
        msgBox.exec();
        if (msgBox.clickedButton() == actionButton)
        {
            qDebug() << "Turn ON";
            m_serProto->writeStoveStateOn();
        }
    }
}

void MainWindow::handleStateForceBtn()
{
    m_serProto->writeStoveStateOffForce();
}

void MainWindow::handlePowerMinBtn()
{
    m_serProto->writeStoveDecPower();
}

void MainWindow::handlePowerPlusBtn()
{
    m_serProto->writeStoveIncPower();
}

void MainWindow::handleSetPointMinBtn()
{
    m_serProto->writeStoveDecSetPoint();
}

void MainWindow::handleSetPointPlusBtn()
{
    m_serProto->writeStoveIncSetPoint();
}
