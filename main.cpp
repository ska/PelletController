#include "mainwindow.h"
#include "serialproto.h"
#include "homeassistant.h"
#include "common.h"

#include <QApplication>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName( SW_NAME );
    QCoreApplication::setApplicationVersion( SW_VER );
    qInfo() << argv[0] << " Build date: "       << PC_BUILD_TIMESTAMP;
    qInfo() << argv[0] << " SW Build name: "    << SW_NAME;
    qInfo() << argv[0] << " Build version: "    << SW_VER << "(" PC_GIT_COMMIT ")";
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

    /* Serial port from setting.ini in the working directory, [SERIAL] port
     * (as AcquaThermoNet); read before MainWindow opens it */
    QSettings settings(QDir::current().filePath("setting.ini"), QSettings::IniFormat);
    SerialProto *s = SerialProto::getInstance();
    s->setSerPort(settings.value("SERIAL/port", SERIAL_PORT_DEFAULT).toString());

    /* Home Assistant (MQTT, read only): only with [MQTT] broker_addr set */
    const MqttConfig mqtt = MqttConfig::load(settings);
    HomeAssistant *ha = mqtt.enabled() ? new HomeAssistant(s, mqtt, &a) : nullptr;
    if(!ha)
        qInfo() << "MQTT off: no [MQTT] broker_addr in setting.ini";

    MainWindow w(ha);
    int ret;

#if DEVICE == DESKTOP
    w.setGeometry(100, 100, 800, 480 );
    w.show();
#else
    w.setWindowFlags( w.windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
    w.showMaximized();
#endif

    ret = a.exec();



    qInfo() << "End Application";
    if(ha)
    {
        //offline on the status topic, then a clean disconnect
        ha->shutdown();
        QElapsedTimer t;
        t.start();
        while(QMqttClient::Disconnected != ha->clientState() && t.elapsed() < 1000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    s->closeSerPort();
    return ret;
}
