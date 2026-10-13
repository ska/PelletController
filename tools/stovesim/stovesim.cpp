#include "stovesim.h"

#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

#include <pty.h>
#include <termios.h>
#include <unistd.h>

#ifndef SIM_REPLY_DELAY
#define SIM_REPLY_DELAY     (40)        //ms between the request and the reply (qmake DEFINES+=SIM_REPLY_DELAY=n)
#endif
#define SIM_STATE_STEP      (10*1000)   //ms in Starting / Final Cleaning with auto states

/* Stove states and addresses used here (docs/PROTOCOL.md) */
enum { Off = 0, Starting = 1, Working = 4, FinalCleaning = 6 };

StoveSim::StoveSim(const QString &linkPath, QObject *parent)
    : QObject(parent), m_linkPath(linkPath)
{
    loadDefaults();
}

StoveSim::~StoveSim()
{
    unplug();
}

/* A stove switched off on a Saturday evening, chrono on, weekend
 * slot 1 07:00-22:00 */
void StoveSim::loadDefaults()
{
    m_ram = QByteArray(256, 0);
    m_eeprom = QByteArray(256, 0);

    setRam(0x01, 40);           //Room temperature 20.0 C
    setRam(0x0D, 15);           //Auger on time
    setRam(0x21, Off);          //State
    setRam(0x34, 8);            //Flame power
    setRam(0x37, 60);           //Smoke fan speed
    setRam(0x5A, 25);           //Smoke temperature
    setRam(0x19, 0x90);         //Power in use (0x90 when Off)
    setRam(0x65, 0);            //Seconds
    setRam(0x66, 6);            //Day of week
    setRam(0x67, 18);           //Hours
    setRam(0x68, 30);           //Minutes
    setRam(0x69, 10);           //Day
    setRam(0x6A, 10);           //Month
    setRam(0x6B, 26);           //Year - 2000

    setEeprom(0x7D, 45);        //Setpoint 22.5 C
    setEeprom(0x7F, 2);         //Power
    setEeprom(0x4C, 1);         //Chrono on
    setEeprom(0x42, 1);         //Weekend on
    setEeprom(0x43, 42);        //Weekend 1 start 07:00
    setEeprom(0x44, 132);       //Weekend 1 stop 22:00
    for(quint8 a : {0x45, 0x46, 0x48, 0x49, 0x4A, 0x4B, 0x51, 0x52, 0x5A, 0x5B, 0x63, 0x64, 0x6C, 0x6D})
        setEeprom(a, 144);      //Other slots off
}

bool StoveSim::plug()
{
    if(m_master >= 0)
        return true;

    int slave = -1;
    char name[128];
    if(openpty(&m_master, &slave, name, nullptr, nullptr) < 0)
    {
        m_master = -1;
        return false;
    }
    struct termios t;
    tcgetattr(m_master, &t);
    cfmakeraw(&t);
    tcsetattr(m_master, TCSANOW, &t);
    ::close(slave);

    QFile::remove(m_linkPath);
    if(!QFile::link(QString::fromLocal8Bit(name), m_linkPath))
    {
        ::close(m_master);
        m_master = -1;
        return false;
    }

    m_in.clear();
    m_notifier = new QSocketNotifier(m_master, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &StoveSim::onReadable);
    return true;
}

void StoveSim::unplug()
{
    if(m_master < 0)
        return;
    delete m_notifier;
    m_notifier = nullptr;
    ::close(m_master);
    m_master = -1;
    QFile::remove(m_linkPath);
}

void StoveSim::corruptChecksums(int count, int address)
{
    m_corruptChecksums = count;
    m_corruptChecksumAddress = address;
}

void StoveSim::corruptEchoes(int count, int address)
{
    m_corruptEchoes = count;
    m_corruptEchoAddress = address;
}

void StoveSim::dropRequests(int count, int address)
{
    m_drops = count;
    m_dropAddress = address;
}

void StoveSim::setAutoStates(bool on)
{
    if(!on)
    {
        delete m_stateTimer;
        m_stateTimer = nullptr;
        return;
    }
    if(!m_stateTimer)
    {
        m_stateTimer = new QTimer(this);
        m_stateTimer->setSingleShot(true);
        connect(m_stateTimer, &QTimer::timeout, this, &StoveSim::stepState);
    }
}

void StoveSim::stepState()
{
    if(ram(0x21) == Starting)
    {
        setRam(0x21, Working);
        setRam(0x19, eeprom(0x7F));
    }
    else if(ram(0x21) == FinalCleaning)
    {
        setRam(0x21, Off);
        setRam(0x19, 0x90);
    }
}

void StoveSim::onReadable()
{
    char buf[64];
    const ssize_t n = ::read(m_master, buf, sizeof(buf));
    if(n <= 0)
        return;
    m_in.append(buf, int(n));

    //Read: 2 bytes, write: 4 bytes (first byte with 0x80)
    while(!m_in.isEmpty())
    {
        const int len = (quint8(m_in.at(0)) & 0x80) ? 4 : 2;
        if(m_in.size() < len)
            break;
        handle(m_in.left(len));
        m_in.remove(0, len);
    }
}

void StoveSim::handle(const QByteArray &request)
{
    m_requests << request.left(2);

    const bool write = quint8(request.at(0)) & 0x80;
    const quint8 page = quint8(request.at(0)) & 0x7F;
    const quint8 address = quint8(request.at(1));
    if(page != 0x00 && page != 0x20)
        return;                             //Unknown bank: no answer
    if(m_drops > 0 && (m_dropAddress < 0 || m_dropAddress == address))
    {
        m_drops--;
        return;                             //Lost on the line: not even the echo
    }

    QByteArray echo = request;
    if(m_corruptEchoes > 0 && (m_corruptEchoAddress < 0 || m_corruptEchoAddress == address))
    {
        m_corruptEchoes--;
        echo[1] = char(address + 1);
    }

    QByteArray reply;
    if(write)
    {
        const quint8 value = quint8(request.at(2));
        const quint8 checksum = quint8(request.at(3));
        const bool valid = checksum == quint8(0x80 + page + address + value);
        const bool stored = valid && m_writesStored;
        if(stored)
        {
            bank(page)[address] = char(value);
            //Clock set in EEPROM F8-FD (BCD): the board shows it in the RAM clock 66-6B
            if(page == 0x20 && address >= 0xF8 && address <= 0xFD)
                setRam(quint8(0x66 + (address - 0xF8)), quint8((value >> 4) * 10 + (value & 0x0F)));
            if(m_stateTimer && page == 0x00 && address == 0x21
               && (value == Starting || value == FinalCleaning))
                m_stateTimer->start(SIM_STATE_STEP);
        }
        emit writeReceived(page, address, value, stored);
        if(valid)
        {
            const quint8 now = quint8(bank(page).at(address));
            reply.append(char(0x80 + page + address + now));
            reply.append(char(now));
        }
    }
    else
    {
        const quint8 value = quint8(bank(page).at(address));
        emit readReceived(page, address, value);
        reply.append(char(page + address + value));
        reply.append(char(value));
    }

    if(!reply.isEmpty() && m_corruptChecksums > 0
       && (m_corruptChecksumAddress < 0 || m_corruptChecksumAddress == address))
    {
        m_corruptChecksums--;
        reply[0] = char(quint8(reply.at(0)) ^ 0x20);       //Bit 5, as seen on the noisy line
    }

    //The echo comes back at once, the board answers a little later
    send(echo);
    if(!m_mute && !reply.isEmpty())
        QTimer::singleShot(SIM_REPLY_DELAY, this, [this, reply]{ send(reply); });
}

void StoveSim::send(const QByteArray &data)
{
    if(m_master < 0)
        return;
    const ssize_t r = ::write(m_master, data.constData(), size_t(data.size()));
    (void) r;
}
