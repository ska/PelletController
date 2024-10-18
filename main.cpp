#include "mainwindow.h"
#include "common.h"

#include <QFontDatabase>
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName( SW_NAME );
    QCoreApplication::setApplicationVersion( SW_VER );
    qInfo() << argv[0] << " Build date: "       << BUILD_DATE;
    qInfo() << argv[0] << " SW Build name: "    << SW_NAME;
    qInfo() << argv[0] << " Build version: "    << SW_VER;
    QString name = qgetenv("USER");
    if (name.isEmpty())
        name = qgetenv("USERNAME");
    qInfo() << argv[0] << " Running user: "    << name;

    /*
     * WatchDog
     * * */
#ifdef WATCHDOG_H
    QThread *threadWdt = new QThread();
    WatchDog *wdtWorker = new WatchDog();
    wdtWorker->moveToThread(threadWdt);
    QObject::connect(wdtWorker,      SIGNAL(workRequested()),                threadWdt,   SLOT(start()));
    QObject::connect(threadWdt,      SIGNAL(started()),                      wdtWorker,   SLOT(doWork()));
    QObject::connect(wdtWorker,      SIGNAL(finished()),                     threadWdt,   SLOT(quit()), Qt::DirectConnection);
    wdtWorker->abort();
    threadWdt->wait(); // If the thread is not running, this will immediately return.
    wdtWorker->requestWork();
#endif

    MainWindow w;
    int ret;

#if DEVICE == DESKTOP
    w.setGeometry(100, 100, 800, 480 );
    w.show();
#else
    w.setWindowFlags( w.windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
    w.showMaximized();
#endif



    SerialProto *s = SerialProto::getInstance();

    QFontDatabase::addApplicationFont(":/fonts/AdvancedDotDigital7ls.ttf");
    ret = a.exec();

    qInfo() << "End Application";
    s->closeSerPort();
    return ret;
}
