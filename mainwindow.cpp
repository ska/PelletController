#include "mainwindow.h"

#include "chronopage.h"
#include "clockdialog.h"
#include "diagpage.h"
#include "mainpage.h"
#include "serialproto.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QMessageBox>
#include <QStackedWidget>
#include <QTimer>

#define CHRONO_IDLE_MS  (10 * 1000)
#define DIAG_IDLE_MS    (60 * 1000)     //Time to watch the counters

MainWindow::MainWindow(HomeAssistant *ha, QWidget *parent)
    : QMainWindow(parent)
{
    QFile qss(":/qss/default.qss");
    if(qss.open(QFile::ReadOnly))
        qApp->setStyleSheet(QString::fromLatin1(qss.readAll()));

    m_proto = SerialProto::getInstance();

    m_stack = new QStackedWidget(this);
    m_mainPage = new MainPage(m_proto, m_stack);
    m_chronoPage = new ChronoPage(m_proto, m_stack);
    m_diagPage = new DiagPage(m_proto, m_stack);
    m_diagPage->setHomeAssistant(ha);
    m_stack->addWidget(m_mainPage);
    m_stack->addWidget(m_chronoPage);
    m_stack->addWidget(m_diagPage);
    setCentralWidget(m_stack);

    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);

    connect(m_mainPage, &MainPage::chronoRequested, this, &MainWindow::showChrono);
    connect(m_mainPage, &MainPage::diagRequested, this, &MainWindow::showDiag);
    connect(m_mainPage, &MainPage::clockRequested, this, &MainWindow::setStoveClock);
    connect(m_chronoPage, &ChronoPage::backRequested, this, &MainWindow::showMain);
    connect(m_diagPage, &DiagPage::backRequested, this, &MainWindow::showMain);
    connect(m_idleTimer, &QTimer::timeout, this, &MainWindow::showMain);
    connect(m_proto, &SerialProto::writeFailed, this, &MainWindow::showWriteFailed);

    //Any touch while another page is shown keeps it open
    qApp->installEventFilter(this);

    m_proto->openSerPort();
    m_proto->startSerLoop();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(m_idleTimer->isActive()
       && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin))
        m_idleTimer->start();
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showMain()
{
    m_idleTimer->stop();
    m_chronoPage->closePicker();
    m_stack->setCurrentWidget(m_mainPage);
    m_proto->setChronoSerialGet(false);
}

void MainWindow::showChrono()
{
    m_stack->setCurrentWidget(m_chronoPage);
    m_proto->setChronoSerialGet(true);
    m_idleTimer->start(CHRONO_IDLE_MS);
}

void MainWindow::showDiag()
{
    m_stack->setCurrentWidget(m_diagPage);
    m_idleTimer->start(DIAG_IDLE_MS);
}

/* Stove clock from the dialog, starting from the clock read (the panel
 * runs in UTC, its own time is not used) */
void MainWindow::setStoveClock()
{
    ClockDialog dialog(m_mainPage->stoveDateTime(), this);
    if(QDialog::Accepted == dialog.exec())
        m_proto->writeClock(dialog.dateTime());
}

/* Write not confirmed by the stove even after the retry: not modal,
 * the polling goes on and shows the value the stove really has */
void MainWindow::showWriteFailed(quint8 page, quint8 address, quint8 value)
{
    QMessageBox *box = new QMessageBox(this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    box->setText(QString("The stove did not confirm the change\n"
                         "(parameter %1/%2, value %3).\n"
                         "Check the value shown and the serial line.")
                     .arg(page, 2, 16, QChar('0'))
                     .arg(address, 2, 16, QChar('0'))
                     .arg(value));
    box->addButton(QMessageBox::Ok);
    box->open();
}
