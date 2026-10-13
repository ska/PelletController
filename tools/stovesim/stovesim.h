#ifndef STOVESIM_H
#define STOVESIM_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

class QSocketNotifier;
class QTimer;

/*
 * Stove board simulator on a pseudo terminal, for the tests and for running
 * the application without a stove (docs/PROTOCOL.md).
 *
 * The slave side of the pty is reached through a symlink (linkPath), so the
 * port name stays the same after unplug() / plug(). Every request is echoed,
 * then answered like the stove board: read [checksum, value], write
 * [checksum, value stored]. RAM and EEPROM are two 256 byte banks.
 *
 * Faults: mute (echo only), corrupted checksums, wrong echo, requests lost
 * (nothing comes back), writes not stored, port unplugged.
 */
class StoveSim : public QObject
{
    Q_OBJECT

public:
    explicit StoveSim(const QString &linkPath, QObject *parent = nullptr);
    ~StoveSim();

    bool plug();                            //Creates the pty and the symlink
    void unplug();                          //Closes the pty, removes the symlink
    bool isPlugged() const { return m_master >= 0; }
    QString linkPath() const { return m_linkPath; }

    quint8 ram(quint8 address) const { return quint8(m_ram.at(address)); }
    quint8 eeprom(quint8 address) const { return quint8(m_eeprom.at(address)); }
    void setRam(quint8 address, quint8 value) { m_ram[address] = char(value); }
    void setEeprom(quint8 address, quint8 value) { m_eeprom[address] = char(value); }

    /* Faults */
    void setMute(bool mute) { m_mute = mute; }                     //Echo only, no reply
    void corruptChecksums(int count, int address = -1);            //Next count replies (to address, -1 any)
    void corruptEchoes(int count, int address = -1);               //Next count echoes: address + 1
    void dropRequests(int count, int address = -1);                //Next count requests: no echo, no reply
    void setWritesStored(bool stored) { m_writesStored = stored; } //false: reply with the old value

    /* States move on by themselves: Starting -> Working, Final Cleaning -> Off */
    void setAutoStates(bool on);

    /* Requests received since the last clear: [bank|0x80 for writes, address] */
    QList<QByteArray> requests() const { return m_requests; }
    void clearRequests() { m_requests.clear(); }

signals:
    void readReceived(quint8 bank, quint8 address, quint8 value);
    void writeReceived(quint8 bank, quint8 address, quint8 value, bool stored);

private:
    QString m_linkPath;
    int m_master = -1;
    QSocketNotifier *m_notifier = nullptr;
    QTimer *m_stateTimer = nullptr;
    QByteArray m_in;
    QByteArray m_ram, m_eeprom;
    QList<QByteArray> m_requests;
    bool m_mute = false;
    bool m_writesStored = true;
    int m_corruptChecksums = 0, m_corruptChecksumAddress = -1;
    int m_corruptEchoes = 0, m_corruptEchoAddress = -1;
    int m_drops = 0, m_dropAddress = -1;

    void loadDefaults();
    void onReadable();
    void handle(const QByteArray &request);
    void send(const QByteArray &data);
    void stepState();
    QByteArray &bank(quint8 page) { return page ? m_eeprom : m_ram; }
};

#endif // STOVESIM_H
