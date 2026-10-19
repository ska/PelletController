#include "mainwindow.h"
#include "serialproto.h"
#include "homeassistant.h"
#include "singleinstance.h"
#include "common.h"

#include <QApplication>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSocketNotifier>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Unix signals (SIGTERM/SIGINT, the package stop.sh): the handler only
 * writes to a socketpair, the Qt side reads it and quits the event loop, so
 * the normal shutdown runs (MQTT offline, port closed) and the exit status
 * is 0. As AcquaThermoNet and RoomSense; see the Qt doc "Calling Qt
 * Functions From Unix Signal Handlers".
 */
static int sigFd[2];

static void unixSignalHandler(int)
{
    char a = 1;
    ssize_t r = ::write(sigFd[0], &a, sizeof(a));
    (void) r;
}

static bool setupUnixSignals()
{
    if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigFd) != 0)
        return false;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = unixSignalHandler;
    sigemptyset(&sa.sa_mask);
    //One shot: a second signal gets the default action (forced kill)
    sa.sa_flags = SA_RESTART | SA_RESETHAND;
    return sigaction(SIGTERM, &sa, nullptr) == 0 && sigaction(SIGINT, &sa, nullptr) == 0;
}

/* setting.ini created from the template deployed next to it, on the first
 * start (as RoomSense and AcquaThermoNet) */
static void ensureSettings(const QString &settingsPath, const QString &defaultPath)
{
    if(QFile::exists(settingsPath) || !QFile::exists(defaultPath))
        return;
    if(!QFile::copy(defaultPath, settingsPath))
    {
        qWarning().noquote() << "Cannot create" << settingsPath << "from" << defaultPath;
        return;
    }
    //copy keeps the read-only mode of a deployed file: it is edited by hand
    QFile::setPermissions(settingsPath, QFile::permissions(settingsPath) | QFileDevice::WriteOwner);
    qInfo().noquote() << "Created" << settingsPath << "from" << defaultPath;
}

int main(int argc, char *argv[])
{
    //Before QApplication: works without a display
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version"))
        {
            printf("%s\n", SW_BANNER);
            return 0;
        }
    }

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName( SW_NAME );
    QCoreApplication::setApplicationVersion( SW_VER );
    qInfo().noquote() << SW_BANNER;

    /* One instance per target, checked before the serial port and MQTT; the
     * lock lasts as long as this object */
    SingleInstance instance( SW_LOCK_NAME );
    if(!instance.tryLock())
        return EXIT_ALREADY_RUNNING;

    if(setupUnixSignals())
    {
        QSocketNotifier *sigNotifier = new QSocketNotifier(sigFd[1], QSocketNotifier::Read, &a);
        QObject::connect(sigNotifier, &QSocketNotifier::activated, &a, [sigNotifier] {
            sigNotifier->setEnabled(false);
            char c;
            ssize_t r = ::read(sigFd[1], &c, sizeof(c));
            (void) r;
            qInfo() << "Termination signal received, shutting down";
            QCoreApplication::quit();
        });
    }
    else
        qWarning() << "Unix signal handler setup failed";

    QString name = qgetenv("USER");
    if (name.isEmpty())
        name = qgetenv("USERNAME");
    qInfo() << "Running user:" << name;

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
    ensureSettings(QDir::current().filePath("setting.ini"), QDir::current().filePath("setting.default.ini"));
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
