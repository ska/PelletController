#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "common.h"


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
    ui->stackedWidget->setCurrentIndex(0);
    ui->chronoTabWdg->setCurrentIndex(0);
    ui->swVersion->setText(SW_NAME_VER);
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

    ui->chronoClbl->setPixmap(":/icons/wall-clock-drk.png");

    m_serProto = SerialProto::getInstance();
    m_chronoShowTimer = new QTimer();
    connect(m_chronoShowTimer, SIGNAL(timeout()), this, SLOT(showMainStack()));

    connect(m_serProto, SIGNAL(updateAmbTemp(float)),                this, SLOT(updateAmbTemp(float)));
    connect(m_serProto, SIGNAL(updateSetTemp(float)),                this, SLOT(updateSetTemp(float)));
    connect(m_serProto, SIGNAL(updateStoveState(quint8,QString)),    this, SLOT(updateStoveState(quint8,QString)));
    connect(m_serProto, SIGNAL(updatePower(quint8,quint8)),          this, SLOT(updatePower(quint8,quint8)));
    connect(m_serProto, SIGNAL(updateStoveDateTime(QDateTime)),      this, SLOT(updateStoveDateTime(QDateTime)));
    connect(m_serProto, SIGNAL(updateStats(quint32,quint32,quint32)),this, SLOT(updateStats(quint32,quint32,quint32)));
    connect(m_serProto, SIGNAL(updateChronoEnable(bool)),            this, SLOT(updateChronoEnable(bool)));
    connect(m_serProto, SIGNAL(updateChronoWkEEnable(bool)),         this, SLOT(updateChronoWkeEnable(bool)));
    connect(m_serProto, SIGNAL(updateChronoWkE1On(quint8)),          this, SLOT(updateChronoWkE1On(quint8)));
    connect(m_serProto, SIGNAL(updateChronoWkE1Off(quint8)),         this, SLOT(updateChronoWkE1Off(quint8)));
    connect(m_serProto, SIGNAL(updateChronoWkE2On(quint8)),          this, SLOT(updateChronoWkE2On(quint8)));
    connect(m_serProto, SIGNAL(updateChronoWkE2Off(quint8)),         this, SLOT(updateChronoWkE2Off(quint8)));
    connect(m_serProto, SIGNAL(updateChronoDayEnable(bool)),         this, SLOT(updateChronoDayEnable(bool)));
    connect(m_serProto, SIGNAL(updateChronoDay1On(quint8)),          this, SLOT(updateChronoDay1On(quint8)));
    connect(m_serProto, SIGNAL(updateChronoDay1Off(quint8)),         this, SLOT(updateChronoDay1Off(quint8)));
    connect(m_serProto, SIGNAL(updateChronoDay2On(quint8)),          this, SLOT(updateChronoDay2On(quint8)));
    connect(m_serProto, SIGNAL(updateChronoDay2Off(quint8)),         this, SLOT(updateChronoDay2Off(quint8)));

    connect(ui->stateBtn,           SIGNAL(released()),              this, SLOT(handleStatteBtn()));
    connect(ui->stateOffForceBtn,   SIGNAL(released()),              this, SLOT(handleStateForceBtn()));
    connect(ui->powerMinBtn,        SIGNAL(released()),              this, SLOT(handlePowerMinBtn()));
    connect(ui->powerPlusBtn,       SIGNAL(released()),              this, SLOT(handlePowerPlusBtn()));
    connect(ui->setPointMinBtn,     SIGNAL(released()),              this, SLOT(handleSetPointMinBtn()));
    connect(ui->setPointPlusBtn,    SIGNAL(released()),              this, SLOT(handleSetPointPlusBtn()));

    connect(ui->chronoClbl,         SIGNAL(clicked()),               this, SLOT(handleClickableLabel()));





    m_serProto->openSerPort();
    m_serProto->startSerLoop();
}

MainWindow::~MainWindow()
{
    delete m_serProto;
    delete ui;
}

bool MainWindow::event(QEvent *event)
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
        if(m_chronoShowTimer && m_chronoShowTimer->isActive())
            m_chronoShowTimer->start(10 * 1000);
        break;
    default:
        break;
    }
    return QWidget::event(event);
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
        ui->stateBtn->setText("Reset");
        ui->stateBtn->setEnabled(true);
        break;

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

void MainWindow::updateChronoEnable(bool b)
{
    if(b) {
        ui->chronoClbl->setPixmap(":/icons/wall-clock-v.png");
        ui->chronoEnableCB->setChecked(Qt::Checked);
    } else {
        ui->chronoClbl->setPixmap(":/icons/wall-clock-drk.png");
        ui->chronoEnableCB->setChecked(Qt::Unchecked);
    }
}

void MainWindow::updateChronoWdg(QCheckBox *cb, QTimeEdit *te, quint8 u)
{
    if(144 == u)
    {
        cb->setChecked(Qt::Unchecked);
        te->setTime(QTime(0, 0, 0, 0));
        te->setEnabled(false);
    } else {
        cb->setChecked(Qt::CheckState::Checked);
        te->setTime(QTime( (quint8)u/6, (u%6)*10) );
        te->setEnabled(true);
    }
}


void MainWindow::updateChronoWkeEnable(bool b)
{
    ui->chronoWkEEnableCB->setChecked( b ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::updateChronoWkE1On(quint8 u)
{
    updateChronoWdg(ui->chronoWkEStart1CB, ui->chronoWkEStart1TE, u);
}

void MainWindow::updateChronoWkE1Off(quint8 u)
{
    updateChronoWdg(ui->chronoWkEStop1CB, ui->chronoWkEStop1TE, u);
}

void MainWindow::updateChronoWkE2On(quint8 u)
{
    updateChronoWdg(ui->chronoWkEStart2CB, ui->chronoWkEStart2TE, u);
}

void MainWindow::updateChronoWkE2Off(quint8 u)
{
    updateChronoWdg(ui->chronoWkEStop2CB, ui->chronoWkEStop2TE, u);
}







void MainWindow::updateChronoDayEnable(bool b)
{
    ui->chronoDayEnableCB->setChecked( b ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::updateChronoDay1On(quint8 u)
{
    updateChronoWdg(ui->chronoDayStart1CB, ui->chronoDayStart1TE, u);
}

void MainWindow::updateChronoDay1Off(quint8 u)
{
    updateChronoWdg(ui->chronoDayStop1CB, ui->chronoDayStop1TE, u);
}

void MainWindow::updateChronoDay2On(quint8 u)
{
    updateChronoWdg(ui->chronoDayStart2CB, ui->chronoDayStart2TE, u);
}

void MainWindow::updateChronoDay2Off(quint8 u)
{
    updateChronoWdg(ui->chronoDayStop2CB, ui->chronoDayStop2TE, u);
}



void MainWindow::handleClickableLabel()
{
    qDebug() << Q_FUNC_INFO;
    ui->stackedWidget->setCurrentIndex(1);
    m_chronoShowTimer->start(10 * 1000);
    m_serProto->setChronoSerialGet(true);
}

void MainWindow::showMainStack()
{
    qDebug() << Q_FUNC_INFO;
    ui->stackedWidget->setCurrentIndex(0);
    m_chronoShowTimer->stop();
    m_serProto->setChronoSerialGet(false);
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
        abortButton = abortButton;
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
        abortButton = abortButton;
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

void MainWindow::on_chronoEnableCB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
    if( Qt::CheckState::Checked == (Qt::CheckState)arg1 )
        m_serProto->writeChronoEnable(true);
    else
        m_serProto->writeChronoEnable(false);

}


void MainWindow::on_chronoWkEStart1CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
}


void MainWindow::on_chronoWkEStop1CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
}


void MainWindow::on_chronoWkEStart2CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
}


void MainWindow::on_chronoWkEStop2CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
}

