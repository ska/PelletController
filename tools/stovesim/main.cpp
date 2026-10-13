// Stove board simulator: run the application without a stove.
//
//   stovesim [--link <path>] [-v]
//
// Then point the application to the link in setting.ini:
//   [SERIAL]
//   port=/tmp/pellet-stove
//
// Commands on stdin: type "help".

#include "stovesim.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <unistd.h>
#include <cstdio>

static void help()
{
    printf("Commands:\n"
           "  state <n>        stove state (0 Off, 4 Working, 10 Alarm, ...)\n"
           "  temp <C>         room temperature, 0.5 steps\n"
           "  setpoint <C>     setpoint, 0.5 steps\n"
           "  power <n>        set power 1-5\n"
           "  mute on|off      echo only, no reply\n"
           "  corrupt <n>      next n replies with a wrong checksum\n"
           "  echo <n>         next n replies with a wrong echo\n"
           "  drop <n>         next n requests lost: no echo, no reply\n"
           "  writes on|off    store the writes or reply with the old value\n"
           "  unplug | plug    remove / bring back the port\n"
           "  show             main values\n"
           "  quit\n");
    fflush(stdout);
}

static void show(const StoveSim &sim)
{
    printf("state %d, room %.1f C, setpoint %.1f C, power %d, chrono %d, weekend %d %d-%d, %s\n",
           sim.ram(0x21), sim.ram(0x01) / 2.0, sim.eeprom(0x7D) / 2.0, sim.eeprom(0x7F),
           sim.eeprom(0x4C), sim.eeprom(0x42), sim.eeprom(0x43), sim.eeprom(0x44),
           sim.isPlugged() ? "plugged" : "unplugged");
    fflush(stdout);
}

static void command(StoveSim &sim, const QString &line)
{
    const QStringList w = line.simplified().split(' ', QString::SkipEmptyParts);
    if(w.isEmpty())
        return;
    const QString c = w.at(0);
    const QString a = w.value(1);
    const bool on = (a == "on");

    if(c == "help")                 help();
    else if(c == "quit")            QCoreApplication::quit();
    else if(c == "show")            show(sim);
    else if(c == "state")           sim.setRam(0x21, quint8(a.toInt()));
    else if(c == "temp")            sim.setRam(0x01, quint8(qRound(a.toDouble() * 2)));
    else if(c == "setpoint")        sim.setEeprom(0x7D, quint8(qRound(a.toDouble() * 2)));
    else if(c == "power")           sim.setEeprom(0x7F, quint8(a.toInt()));
    else if(c == "mute")            sim.setMute(on);
    else if(c == "corrupt")         sim.corruptChecksums(a.toInt());
    else if(c == "echo")            sim.corruptEchoes(a.toInt());
    else if(c == "drop")            sim.dropRequests(a.toInt());
    else if(c == "writes")          sim.setWritesStored(on);
    else if(c == "unplug")          sim.unplug();
    else if(c == "plug")            printf(sim.plug() ? "plugged\n" : "plug failed\n");
    else                            printf("unknown command, type help\n");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Pellet stove board simulator on a pseudo terminal");
    parser.addHelpOption();
    QCommandLineOption linkOpt("link", "Symlink to the port (default /tmp/pellet-stove).", "path", "/tmp/pellet-stove");
    QCommandLineOption verboseOpt("v", "Print every read.");
    parser.addOption(linkOpt);
    parser.addOption(verboseOpt);
    parser.process(app);

    StoveSim sim(parser.value(linkOpt));
    sim.setAutoStates(true);
    if(!sim.plug())
    {
        fprintf(stderr, "Cannot create the pty or %s\n", qPrintable(parser.value(linkOpt)));
        return 1;
    }

    if(parser.isSet(verboseOpt))
        QObject::connect(&sim, &StoveSim::readReceived, [](quint8 bank, quint8 address, quint8 value) {
            printf("read  %02X %02X = %d\n", bank, address, value);
            fflush(stdout);
        });
    QObject::connect(&sim, &StoveSim::writeReceived, [](quint8 bank, quint8 address, quint8 value, bool stored) {
        printf("write %02X %02X = %d%s\n", bank, address, value, stored ? "" : " (not stored)");
        fflush(stdout);
    });

    printf("Stove simulator on %s\n", qPrintable(sim.linkPath()));
    show(sim);
    help();

    //Raw reads: a buffered stream would keep the next lines out of the notifier
    QByteArray pending;
    QSocketNotifier input(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&input, &QSocketNotifier::activated, [&] {
        char buf[256];
        const ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
        if(n <= 0)
        {
            input.setEnabled(false);        //stdin closed: keep running
            return;
        }
        pending.append(buf, int(n));
        int eol;
        while((eol = pending.indexOf('\n')) >= 0)
        {
            command(sim, QString::fromLocal8Bit(pending.left(eol)));
            pending.remove(0, eol + 1);
        }
    });

    return app.exec();
}
