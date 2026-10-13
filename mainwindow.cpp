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


    tm = new TimeEditDialog(this);

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
    ui->stateBtn->setEnabled(false);    //Until the stove state has been read
    ui->stateOffForceBtn->setVisible(false);

    ui->chronoClbl->setMyPixmap(":/icons/wall-clock-drk.png");
    ui->chronoWkEStart1LCD->setTensOfMins(0);
    ui->chronoWkEStart1LCD->setEnabled(false);
    ui->chronoWkEStop1LCD->setTensOfMins(0);
    ui->chronoWkEStop1LCD->setEnabled(false);

    m_serProto = SerialProto::getInstance();
    m_chronoShowTimer = new QTimer();
    connect(m_chronoShowTimer,  SIGNAL(timeout()),                  this, SLOT(showMainStack()));
    connect(tm,                 SIGNAL(touchMouseEvent()),          this, SLOT(restartTimer()));
    connect(tm,                 SIGNAL(tensOfMinsUpdated(quint8)),  this, SLOT(chronoWkE_LCD_update(quint8)));

    connect(m_serProto, SIGNAL(updateAmbTemp(float)),                this, SLOT(on_updateAmbTemp(float)));
    connect(m_serProto, SIGNAL(updateSetTemp(float)),                this, SLOT(on_updateSetTemp(float)));
    connect(m_serProto, SIGNAL(updateStoveState(quint8,QString)),    this, SLOT(on_updateStoveState(quint8,QString)));
    connect(m_serProto, SIGNAL(updatePower(quint8,quint8)),          this, SLOT(on_updatePower(quint8,quint8)));
    connect(m_serProto, SIGNAL(updateStoveDateTime(QDateTime)),      this, SLOT(on_updateStoveDateTime(QDateTime)));
    connect(m_serProto, SIGNAL(updateStats(quint32,quint32,quint32,quint32,quint32)),this, SLOT(on_updateStats(quint32,quint32,quint32,quint32,quint32)));
    connect(m_serProto, SIGNAL(updateChronoEnable(bool)),            this, SLOT(on_updateChronoEnable(bool)));
    connect(m_serProto, SIGNAL(updateChronoWkEEnable(bool)),         this, SLOT(on_updateChronoWkeEnable(bool)));
    connect(m_serProto, SIGNAL(updateChronoWkE1On(quint8)),          this, SLOT(on_updateChronoWkE1On(quint8)));
    connect(m_serProto, SIGNAL(updateChronoWkE1Off(quint8)),         this, SLOT(on_updateChronoWkE1Off(quint8)));
    connect(m_serProto, SIGNAL(writeFailed(quint8,quint8,quint8)),   this, SLOT(on_writeFailed(quint8,quint8,quint8)));
    connect(m_serProto, SIGNAL(linkStateChanged(quint8)),            this, SLOT(on_linkStateChanged(quint8)));

    mapper = new QSignalMapper(this);
    QObject::connect(mapper,SIGNAL(mapped(QWidget *)), this, SLOT(slot_chronoWkE_LCD_clicked(QWidget *)));
    connect(ui->chronoWkEStart1LCD, SIGNAL(clicked()),mapper,SLOT(map()));
    mapper->setMapping(ui->chronoWkEStart1LCD, ui->chronoWkEStart1LCD);
    connect(ui->chronoWkEStop1LCD, SIGNAL(clicked()),mapper,SLOT(map()));
    mapper->setMapping(ui->chronoWkEStop1LCD, ui->chronoWkEStop1LCD);
/*
    QPushButton * but = new QPushButton(this);
    QObject::connect(but, SIGNAL(clicked()),mapper,SLOT(map()));
    mapper->setMapping(but, but);
*/
    m_serProto->openSerPort();
    m_serProto->startSerLoop();

}

MainWindow::~MainWindow()
{
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

void MainWindow::restartTimer()
{
    if(m_chronoShowTimer && m_chronoShowTimer->isActive())
        m_chronoShowTimer->start(10 * 1000);
}

void MainWindow::showMainStack()
{
    qDebug() << Q_FUNC_INFO;
    ui->stackedWidget->setCurrentIndex(0);
    m_chronoShowTimer->stop();
    m_serProto->setChronoSerialGet(false);
    tm->close();
}

/*****************************************************************************
 ***** SLOTs --> Signals from SERIAL
 *****************************************************************************/
void MainWindow::on_updateAmbTemp(float f)
{
    ui->ambTemp->setText(QString::number(f, 'f', 1) );
}

void MainWindow::on_updateSetTemp(float f)
{
    ui->setTemp->setText(QString::number(f, 'f', 1) );
}

void MainWindow::on_updateStoveState(quint8 u, QString s)
{
    ui->state->setText(s);

    switch (u) {
    case SerialProto::StoveState::Off:
        ui->stateBtn->setText("On");
        ui->stateBtn->setEnabled(true);
        ui->stateOffForceBtn->setVisible(false);
        break;
    case SerialProto::StoveState::Working:
        ui->stateBtn->setText("Off");
        ui->stateBtn->setEnabled(true);
        ui->stateOffForceBtn->setVisible(false);
        break;

    case SerialProto::StoveState::PelletMissing:
    case SerialProto::StoveState::IgnitionFailure:
    case SerialProto::StoveState::Alarm:
        //Only Reset: On/Off would write Final Cleaning or Starting (issue 22)
        ui->stateBtn->setEnabled(false);
        ui->stateBtn->setText("Off");
        ui->stateOffForceBtn->setVisible(true);
        ui->stateOffForceBtn->setText("Reset");
        ui->stateOffForceBtn->setEnabled(true);
        break;

    default:
        ui->stateBtn->setEnabled(false);
        ui->stateBtn->setText("Off");
        ui->stateOffForceBtn->setVisible(false);
        break;
    }
}

/* Write not confirmed by the stove after the retry (issue 9): not modal,
 * the polling goes on and shows the value the stove really has */
void MainWindow::on_writeFailed(quint8 page, quint8 address, quint8 value)
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    msgBox->setText(QString("The stove did not confirm the change\n"
                            "(parameter %1/%2, value %3).\n"
                            "Check the value shown and the serial line.")
                        .arg(page, 2, 16, QChar('0'))
                        .arg(address, 2, 16, QChar('0'))
                        .arg(value));
    msgBox->addButton(QMessageBox::Ok);
    msgBox->open();
}

void MainWindow::on_updatePower(quint8 set, quint8 flame)
{
    ui->setPower->setText(QString::number(set));
    ui->setPowerPerc->setText(QString("%1 %").arg(QString::number(flame)));
}

void MainWindow::on_updateStoveDateTime(QDateTime dt)
{
    ui->datetime->setText( QString("%1 - %2")
                              .arg(dt.toString("dd/MM/yyyy"))
                              .arg(dt.toString("hh:mm"))
                          );
}

/* Serial port closed or stove not answering: shown in place of the state
 * until the next state read, On/Off disabled (issue 10) */
void MainWindow::on_linkStateChanged(quint8 linkState)
{
    switch (linkState) {
    case SerialProto::LinkPortClosed:
        ui->state->setText("No serial port");
        break;
    case SerialProto::LinkNoReply:
        ui->state->setText("No reply");
        break;
    default:
        return;
    }
    ui->stateBtn->setEnabled(false);
    ui->stateOffForceBtn->setVisible(false);
}

void MainWindow::on_updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors, quint32 rxChecksumErrors, quint32 rxTimeouts)
{
    ui->serialStats->setText(QString("Tx: %1 Rx: %2 Err: %3 Chk: %4 Tmo: %5")
                                 .arg(txMessages)
                                 .arg(rxMessages)
                                 .arg(rxErrors)
                                 .arg(rxChecksumErrors)
                                 .arg(rxTimeouts));
}

/* Values read from the stove: block the check box signals, the
 * stateChanged slots write to the stove (issue 3) */
void MainWindow::on_updateChronoEnable(bool b)
{
    ui->chronoClbl->setMyPixmap(b ? ":/icons/wall-clock-v.png" : ":/icons/wall-clock-drk.png");
    const QSignalBlocker blocker(ui->chronoEnableCB);
    ui->chronoEnableCB->setChecked(b);
}

void MainWindow::on_updateChronoWkeEnable(bool b)
{
    const QSignalBlocker blocker(ui->chronoWkEEnableCB);
    ui->chronoWkEEnableCB->setChecked(b);
}

/*****************************************************************************
 ***** SLOTs --> Signals from GUI
 *****************************************************************************/
/* GUI LCD Chrono */
/* Set the LCD value (tensOfMins, not only the text: issue 20) before the
 * check box, and never write back what was read (issue 3) */
void MainWindow::updateUiData_ChronoWdg(QCheckBox *cb, wClickableLCDNumber *lcd, quint8 u)
{
    const bool on = (144 != u);
    lcd->setTensOfMins(on ? u : 0);
    lcd->setEnabled(on);
    const QSignalBlocker blocker(cb);
    cb->setChecked(on);
}

void MainWindow::on_updateChronoWkE1On(quint8 u)
{
    updateUiData_ChronoWdg(ui->chronoWkEStart1CB, ui->chronoWkEStart1LCD, u);
}

void MainWindow::on_updateChronoWkE1Off(quint8 u)
{
    updateUiData_ChronoWdg(ui->chronoWkEStop1CB, ui->chronoWkEStop1LCD, u);
}

/* GUI CheckBox */
void MainWindow::on_chronoEnableCB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
    if( Qt::CheckState::Checked == (Qt::CheckState)arg1 )
        m_serProto->writeChronoEnable(true);
    else
        m_serProto->writeChronoEnable(false);
}

void MainWindow::on_chronoWkEEnableCB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
    if( Qt::CheckState::Checked == (Qt::CheckState)arg1 )
        m_serProto->writeChronoWkeEnable(true);
    else
        m_serProto->writeChronoWkeEnable(false);
}

void MainWindow::on_chronoWkEStart1CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
    quint8 data;
    if( Qt::Checked == arg1 )
    {
        data = ui->chronoWkEStart1LCD->tensOfMins();
        qDebug() << "Write LCD: " << data;
        ui->chronoWkEStart1LCD->setEnabled(true);
    } else {
        data = 144;
        qDebug() << "Write: " << data;
        ui->chronoWkEStart1LCD->setEnabled(false);
    }
    m_serProto->writeChronoWke1On(data);
    //ui->chronoWkEStart1LCD->tensOfMins()

}

void MainWindow::on_chronoWkEStop1CB_stateChanged(int arg1)
{
    qDebug() << Q_FUNC_INFO << " - " << (Qt::CheckState)arg1;
    quint8 data;
    if( Qt::Checked == arg1 )
    {
        data = ui->chronoWkEStop1LCD->tensOfMins();
        ui->chronoWkEStop1LCD->setEnabled(true);
    } else {
        data = 144;
        ui->chronoWkEStop1LCD->setEnabled(false);
    }
    m_serProto->writeChronoWke1Off(data);
}

/* GUI Lcd */
/*
void MainWindow::on_chronoWkEStart1LCD_clicked()
{
    qDebug() << "on_chronoWkEStart1LCD_clicked: " << ui->chronoWkEStart1LCD->tensOfMins();
    tm->setTensOfMins(ui->chronoWkEStart1LCD->tensOfMins());
    tm->show();
}
*/
void MainWindow::slot_chronoWkE_LCD_clicked(QWidget *w)
{
    qDebug() << "slot_chronoWkE_LCD_clicked: " << w;

    if(ui->chronoWkEStart1LCD != w && ui->chronoWkEStop1LCD != w)
        return;
    m_editLcd = static_cast<wClickableLCDNumber *>(w);
    tm->setTensOfMins(m_editLcd->tensOfMins());
    tm->show();
}


/* GUI Write TensOfMins: the parameter of the LCD that opened the picker (issue 4) */
void MainWindow::chronoWkE_LCD_update(quint8 m)
{
    qDebug() << "chronoWkE_LCD_update: " << m_editLcd << m;
    if(ui->chronoWkEStart1LCD == m_editLcd)
        m_serProto->writeChronoWke1On(m);
    else if(ui->chronoWkEStop1LCD == m_editLcd)
        m_serProto->writeChronoWke1Off(m);
    m_editLcd = nullptr;
    tm->close();
}

/* GUI Button */
void MainWindow::on_stateBtn_released()
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

void MainWindow::on_stateOffForceBtn_released()
{
    m_serProto->writeStoveStateOffForce();
}

void MainWindow::on_setPointMinBtn_released()
{
    m_serProto->writeStoveDecSetPoint();
}

void MainWindow::on_setPointPlusBtn_released()
{
    m_serProto->writeStoveIncSetPoint();
}

void MainWindow::on_powerMinBtn_released()
{
    m_serProto->writeStoveDecPower();
}

void MainWindow::on_powerPlusBtn_released()
{
    m_serProto->writeStoveIncPower();
}

/* GUI Label */
void MainWindow::on_chronoClbl_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    m_chronoShowTimer->start(10 * 1000);
    m_serProto->setChronoSerialGet(true);
}


