#include "mainwindow.h"

#include "chronopage.h"
#include "clockdialog.h"
#include "diagpage.h"
#include "mainpage.h"
#include "serialproto.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
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
    m_mainPage->setHomeAssistant(ha);
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

    //Any touch while another page is shown keeps it open; F1-F6 = keys 1-6
    qApp->installEventFilter(this);

    m_proto->openSerPort();
    m_proto->startSerLoop();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    /* The touch screen reports its last row and column as 480 and 800, one
     * pixel outside the window: a tap on the bottom edge (the key bar) would
     * reach no widget. Moved inside, to the widget under it; Qt then sends
     * the rest of that touch here (it grabbed the press): to the same
     * widget until the release, inside the window or not. */
    if(watched == this && (event->type() == QEvent::MouseButtonPress
                           || event->type() == QEvent::MouseButtonRelease
                           || event->type() == QEvent::MouseButtonDblClick
                           || event->type() == QEvent::MouseMove))
    {
        const QMouseEvent *m = static_cast<QMouseEvent *>(event);
        const bool press = (event->type() == QEvent::MouseButtonPress
                            || event->type() == QEvent::MouseButtonDblClick);
        const QPoint in(qBound(0, m->pos().x(), width() - 1), qBound(0, m->pos().y(), height() - 1));
        if(press && in != m->pos())
            m_edgeTarget = childAt(in);
        if(m_edgeTarget)
        {
            QWidget *target = m_edgeTarget;
            if(event->type() == QEvent::MouseButtonRelease)
                m_edgeTarget = nullptr;
            QMouseEvent moved(m->type(), target->mapFrom(this, in), in, mapToGlobal(in),
                              m->button(), m->buttons(), m->modifiers(), m->source());
            QApplication::sendEvent(target, &moved);
            return true;
        }
    }
    /* Keys under the screen (evdev F1-F6): only the main page uses them,
     * not under a dialog; a release always goes, to end a hold */
    if(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        const int key = static_cast<QKeyEvent *>(event)->key();
        if(key >= Qt::Key_F1 && key <= Qt::Key_F6)
        {
            const bool press = (event->type() == QEvent::KeyPress);
            if(!static_cast<QKeyEvent *>(event)->isAutoRepeat()
               && (!press || (m_stack->currentWidget() == m_mainPage
                              && !QApplication::activeModalWidget()
                              && !QApplication::activePopupWidget())))
                m_mainPage->keyEvent(key - Qt::Key_F1 + 1, press);
            return true;
        }
    }
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
