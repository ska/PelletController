// Read only dump of the stove memory: every RAM and EEPROM location.
//
//   stovedump [--port /dev/ttyUSB0] [--bank ram|eeprom|both]
//             [--from 00] [--to FF] [--csv <file>]
//
// Only read requests are sent: 2 bytes, bank 00 (RAM) or 20 (EEPROM),
// see docs/PROTOCOL.md. There is no code here that builds a write.
// Stop the application first: only one program can use the port.

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

#include <QtSerialPort/qserialport.h>

#include <cstdio>

#define DUMP_ATTEMPTS       (3)     //Reads per location before giving up
#define DUMP_REPLY_TIMEOUT  (400)   //ms for the whole reply
#define DUMP_IDLE           (60)    //ms of idle line after the last byte

enum Result { Ok, NoReply, BadReply };

/* One read: [bank, address] -> echo (when the line has it), checksum, value */
static Result readLocation(QSerialPort &port, quint8 bank, quint8 address, quint8 &value, QByteArray &raw)
{
    Q_ASSERT(bank == 0x00 || bank == 0x20);    //Read banks only, never 0x80 / 0xA0

    port.clear(QSerialPort::Input);
    const char request[2] = { char(bank), char(address) };
    port.write(request, 2);
    port.waitForBytesWritten(200);

    raw.clear();
    QElapsedTimer t;
    t.start();
    while(t.elapsed() < DUMP_REPLY_TIMEOUT)
    {
        if(port.waitForReadyRead(raw.size() >= 4 ? DUMP_IDLE : 20))
            raw.append(port.readAll());
        else if(raw.size() >= 4)
            break;                          //Complete and the line is idle
    }

    QByteArray reply = raw;
    if(reply.size() == 4 && reply.startsWith(QByteArray(request, 2)))
        reply.remove(0, 2);                 //Echo of the request
    if(reply.isEmpty() || (reply.size() == 2 && raw.size() == 2 && raw == QByteArray(request, 2)))
        return NoReply;                     //Nothing, or the echo only
    if(reply.size() != 2)
        return BadReply;

    value = quint8(reply.at(1));
    if(quint8(reply.at(0)) != quint8(bank + address + value))
        return BadReply;
    return Ok;
}

static bool parseByte(const QString &s, quint8 &out)
{
    bool ok = false;
    const uint v = s.toUInt(&ok, 16);
    if(!ok || v > 0xFF)
        return false;
    out = quint8(v);
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Read only dump of the stove RAM and EEPROM");
    parser.addHelpOption();
    QCommandLineOption portOpt("port", "Serial port (default /dev/ttyUSB0).", "name", "/dev/ttyUSB0");
    QCommandLineOption bankOpt("bank", "ram, eeprom or both (default both).", "bank", "both");
    QCommandLineOption fromOpt("from", "First address, hex (default 00).", "hex", "00");
    QCommandLineOption toOpt("to", "Last address, hex (default FF).", "hex", "FF");
    QCommandLineOption csvOpt("csv", "Also write bank,address,value lines to this file.", "file");
    parser.addOptions({ portOpt, bankOpt, fromOpt, toOpt, csvOpt });
    parser.process(app);

    quint8 from, to;
    if(!parseByte(parser.value(fromOpt), from) || !parseByte(parser.value(toOpt), to) || from > to)
    {
        fprintf(stderr, "Bad --from / --to\n");
        return 2;
    }
    QList<QPair<QString, quint8>> banks;
    const QString b = parser.value(bankOpt);
    if(b == "ram" || b == "both")       banks << qMakePair(QString("RAM"), quint8(0x00));
    if(b == "eeprom" || b == "both")    banks << qMakePair(QString("EEPROM"), quint8(0x20));
    if(banks.isEmpty())
    {
        fprintf(stderr, "Bad --bank\n");
        return 2;
    }

    QSerialPort port(parser.value(portOpt));
    port.setBaudRate(QSerialPort::Baud1200);
    port.setDataBits(QSerialPort::Data8);
    port.setParity(QSerialPort::NoParity);
    port.setStopBits(QSerialPort::TwoStop);
    port.setFlowControl(QSerialPort::NoFlowControl);
    if(!port.open(QIODevice::ReadWrite))
    {
        fprintf(stderr, "Cannot open %s: %s\n", qPrintable(port.portName()), qPrintable(port.errorString()));
        return 1;
    }

    QFile csv;
    QTextStream csvOut;
    if(parser.isSet(csvOpt))
    {
        csv.setFileName(parser.value(csvOpt));
        if(!csv.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            fprintf(stderr, "Cannot write %s\n", qPrintable(csv.fileName()));
            return 1;
        }
        csvOut.setDevice(&csv);
        csvOut << "bank,address,hex,dec,status\n";
    }

    int failed = 0;
    for(const auto &bank : banks)
    {
        printf("\n%s %02X-%02X (hex values, -- = no reply, ?? = bad reply)\n", qPrintable(bank.first), from, to);
        printf("     ");
        for(int c = 0; c < 16; c++)
            printf(" %X ", c);
        printf("\n");

        const int first = from & 0xF0;
        for(int row = first; row <= to; row += 16)
        {
            printf("%02X: ", row);
            QByteArray ascii;
            for(int a = row; a < row + 16; a++)
            {
                if(a < from || a > to)
                {
                    printf("   ");
                    ascii.append(' ');
                    continue;
                }
                quint8 value = 0;
                QByteArray raw;
                Result r = NoReply;
                for(int i = 0; i < DUMP_ATTEMPTS && r != Ok; i++)
                    r = readLocation(port, bank.second, quint8(a), value, raw);

                ascii.append(r == Ok && value >= 0x20 && value < 0x7F ? char(value) : '.');
                if(r == Ok)
                    printf(" %02X", value);
                else
                {
                    printf(r == NoReply ? " --" : " ??");
                    failed++;
                }
                fflush(stdout);

                if(csv.isOpen())
                    csvOut << bank.first << QString(",%1,%2,%3,").arg(a, 2, 16, QChar('0')).toUpper()
                                                .arg(value, 2, 16, QChar('0')).arg(value)
                           << (r == Ok ? "ok" : r == NoReply ? "no reply" : "bad reply " + raw.toHex(' '))
                           << "\n";
            }
            printf("  %s\n", ascii.constData());
        }
    }

    printf("\n%d location(s) not read\n", failed);
    return failed ? 3 : 0;
}
