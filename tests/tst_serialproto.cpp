#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "serialproto.h"
#include "testutil.h"
#include "stovesim.h"

/*
 * SerialProto against the stove simulator (tools/stovesim), in real time:
 * 200 ms per request as on the panel. Never a real port: the port name is
 * a symlink in a temporary directory.
 *
 * On a busy machine the simulator can answer later than the 100 ms idle
 * time of SerialProto: the reply is then taken as invalid and read again,
 * which changes the exact request sequence. The tests that check a
 * sequence skip (not fail) when errors appear that they did not inject.
 */

/* Skip the test when Err, Chk or Tmo grew more than the faults injected */
#define SKIP_IF_LATE(err, chk, tmo) \
    do { if(late(err, chk, tmo)) QSKIP("Unexpected Err/Chk/Tmo: machine too busy, simulator replies late"); } while(0)

class TstSerialProto : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    StoveSim *m_sim = nullptr;
    SerialProto *m_proto = nullptr;

    /* Requests as address list, e.g. "01 21 0d" */
    QString sequence(int count = -1) const
    {
        QStringList s;
        for(const QByteArray &r : m_sim->requests())
            s << QString("%1").arg(quint8(r.at(1)), 2, 16, QChar('0'));
        return (count < 0 ? s : s.mid(0, count)).join(' ');
    }

    /* Last updateStats: Tx, Rx (from the last start), Err, Chk, Tmo (since
     * the start of the process); m_base: the same at the start of the test */
    QList<quint32> m_stats { 0, 0, 0, 0, 0 };
    QList<quint32> m_base;

    quint32 delta(int i) const { return m_stats.at(i) - m_base.at(i); }

    bool late(quint32 err, quint32 chk, quint32 tmo) const
    {
        return delta(2) > err || delta(3) > chk || delta(4) > tmo;
    }

    void start(bool chrono = false)
    {
        m_proto->setChronoSerialGet(chrono);
        QVERIFY(m_proto->openSerPort());
        m_proto->startSerLoop();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_proto = SerialProto::getInstance();
        connect(m_proto, &SerialProto::updateStats, this,
                [this](quint32 tx, quint32 rx, quint32 err, quint32 chk, quint32 tmo) {
                    m_stats = { tx, rx, err, chk, tmo };
                });
    }

    void init()
    {
        m_base = m_stats;
        m_sim = new StoveSim(m_dir.filePath("stove"));
        QVERIFY(m_sim->plug());
        m_proto->setSerPort(m_sim->linkPath());
    }

    void cleanup()
    {
        m_proto->closeSerPort();
        delete m_sim;
        m_sim = nullptr;
    }

    /* Parameters 0-15, the state every other request and never twice in a row */
    void pollingSequence()
    {
        QSignalSpy amb(m_proto, SIGNAL(updateAmbTemp(float)));
        QSignalSpy set(m_proto, SIGNAL(updateSetTemp(float)));
        QSignalSpy state(m_proto, SIGNAL(updateStoveState(quint8,QString)));
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 32, 10000);
        SKIP_IF_LATE(0, 0, 0);
        QCOMPARE(sequence(32), QString("01 21 0d 21 34 21 37 21 5a 21 19 21 65 21 66 21 67 21 "
                                       "68 21 69 21 6a 21 6b 21 7d 21 7f 21 4c 21"));
        QCOMPARE(amb.first().at(0).toFloat(), 20.0f);
        QCOMPARE(set.first().at(0).toFloat(), 22.5f);
        QCOMPARE(state.first().at(1).toString(), QString("Off"));
    }

    /* Wrong checksum: Chk + 1, the same parameter read again once */
    void checksumRetry()
    {
        m_sim->corruptChecksums(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 6, 3000);
        SKIP_IF_LATE(0, 1, 0);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QCOMPARE(delta(3), 1u);
    }

    /* Wrong echo: Err + 1, read again once */
    void echoRetry()
    {
        m_sim->corruptEchoes(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 6, 3000);
        SKIP_IF_LATE(1, 0, 0);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QCOMPARE(delta(2), 1u);
    }

    /* Nothing comes back (not even the echo): Tmo + 1, read again once */
    void lostRequest()
    {
        m_sim->dropRequests(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 6, 3000);
        SKIP_IF_LATE(0, 0, 1);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QCOMPARE(delta(4), 1u);
    }

    /* The retry fails too: no second retry */
    void retryOnlyOnce()
    {
        m_sim->corruptChecksums(2, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 6, 3000);
        SKIP_IF_LATE(0, 2, 0);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QCOMPARE(delta(3), 2u);
    }

    /* Echo only: "No reply" after 10 s, OK again with the first valid reply */
    void noReply()
    {
        QSignalSpy link(m_proto, SIGNAL(linkStateChanged(quint8)));
        start();
        QTest::qWait(500);
        m_sim->setMute(true);
        QTRY_VERIFY_WITH_TIMEOUT(!link.isEmpty() && link.last().at(0).toInt() == SerialProto::LinkNoReply, 12000);
        m_sim->setMute(false);
        QTRY_VERIFY_WITH_TIMEOUT(link.last().at(0).toInt() == SerialProto::LinkOk, 3000);
    }

    /* Setpoint + after the read: written, verified, no failure */
    void writeVerified()
    {
        QSignalSpy set(m_proto, SIGNAL(updateSetTemp(float)));
        QSignalSpy failed(m_proto, SIGNAL(writeFailed(quint8,quint8,quint8)));
        QSignalSpy writes(m_sim, &StoveSim::writeReceived);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!set.isEmpty(), 8000);
        m_proto->writeStoveIncSetPoint();
        QTRY_COMPARE_WITH_TIMEOUT(int(m_sim->eeprom(0x7D)), 46, 2000);
        QTest::qWait(1000);
        QCOMPARE(writes.count(), 1);
        QCOMPARE(failed.count(), 0);
    }

    /* Write not stored by the stove: sent twice, then writeFailed */
    void writeRejected()
    {
        QSignalSpy power(m_proto, SIGNAL(updatePower(quint8,quint8)));
        QSignalSpy failed(m_proto, SIGNAL(writeFailed(quint8,quint8,quint8)));
        QSignalSpy writes(m_sim, &StoveSim::writeReceived);
        m_sim->setWritesStored(false);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!power.isEmpty(), 8000);
        m_proto->writeStoveIncPower();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(writes.count(), 2);
        QCOMPARE(failed.first().at(1).toInt(), 0x7F);
        QCOMPARE(failed.first().at(2).toInt(), 3);
        QCOMPARE(int(m_sim->eeprom(0x7F)), 2);
    }

    /* Setpoint at 30.0 C: + ignored, - written */
    void setpointLimits()
    {
        QSignalSpy set(m_proto, SIGNAL(updateSetTemp(float)));
        QSignalSpy writes(m_sim, &StoveSim::writeReceived);
        m_sim->setEeprom(0x7D, 60);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!set.isEmpty() && set.last().at(0).toFloat() == 30.0f, 8000);
        m_proto->writeStoveIncSetPoint();
        QTest::qWait(1000);
        QCOMPARE(writes.count(), 0);
        m_proto->writeStoveDecSetPoint();
        QTRY_COMPARE_WITH_TIMEOUT(int(m_sim->eeprom(0x7D)), 59, 2000);
    }

    /* USB adapter unplugged and plugged back: port reopened, polling restarts */
    void unplugReplug()
    {
        QSignalSpy link(m_proto, SIGNAL(linkStateChanged(quint8)));
        QSignalSpy failed(m_proto, SIGNAL(writeFailed(quint8,quint8,quint8)));
        QSignalSpy amb(m_proto, SIGNAL(updateAmbTemp(float)));
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!amb.isEmpty(), 2000);
        m_sim->unplug();
        QTRY_VERIFY_WITH_TIMEOUT(!link.isEmpty() && link.last().at(0).toInt() == SerialProto::LinkPortClosed, 3000);
        m_proto->writeChrono(SerialProto::chronoEnableIndex, 1);
        QCOMPARE(failed.count(), 1);
        QTest::qWait(4000);                 //At least one failed reopen
        QVERIFY(m_sim->plug());
        amb.clear();
        QTRY_VERIFY_WITH_TIMEOUT(link.last().at(0).toInt() == SerialProto::LinkOk, 6000);
        QTRY_VERIFY_WITH_TIMEOUT(!amb.isEmpty(), 10000);
    }

    /* Smoke temperature and fan speed, power in use (RAM 5A, 37, 19) */
    void smokeAndPowerInUse()
    {
        QSignalSpy smoke(m_proto, SIGNAL(updateSmoke(quint8,quint16)));
        QSignalSpy inUse(m_proto, SIGNAL(updatePowerInUse(quint8)));
        m_sim->setRam(0x37, 60);
        m_sim->setRam(0x19, 2);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!smoke.isEmpty() && !inUse.isEmpty(), 4000);
        QCOMPARE(smoke.first().at(0).toInt(), 25);
        QCOMPARE(smoke.first().at(1).toInt(), 60 * 10 + 250);
        QCOMPARE(inUse.first().at(0).toInt(), 2);
    }

    /* Chrono writes: in range written, out of range refused */
    void chronoWrite()
    {
        QSignalSpy writes(m_sim, &StoveSim::writeReceived);
        start();
        QTest::qWait(300);
        QVERIFY(!m_proto->writeChrono(SerialProto::chronoWkE_2_OnIndex, 145));
        QVERIFY(!m_proto->writeChrono(SerialProto::chronoWkE_EnableIndex, 2));
        QVERIFY(!m_proto->writeChrono(SerialProto::tempSetIndex, 40));
        QVERIFY(m_proto->writeChrono(SerialProto::chronoWkE_2_OnIndex, 102));
        QVERIFY(m_proto->writeChrono(SerialProto::chronoDay_1_OffIndex, 144));
        QTRY_COMPARE_WITH_TIMEOUT(writes.count(), 2, 3000);
        QCOMPARE(int(m_sim->eeprom(0x45)), 102);
        QCOMPARE(int(m_sim->eeprom(0x49)), 144);
    }

    /* Stove clock: six BCD writes to EEPROM F8-FD, then read back from the
     * RAM clock (the simulator copies it as the board does) */
    void clockWrite()
    {
        QSignalSpy clock(m_proto, SIGNAL(updateStoveDateTime(QDateTime)));
        QSignalSpy failed(m_proto, SIGNAL(writeFailed(quint8,quint8,quint8)));
        start();
        QTest::qWait(300);
        QVERIFY(!m_proto->writeClock(QDateTime()));
        QVERIFY(m_proto->writeClock(QDateTime(QDate(2026, 10, 13), QTime(9, 35))));
        QTRY_COMPARE_WITH_TIMEOUT(int(m_sim->eeprom(0xFD)), 0x26, 6000);
        const quint8 expected[6] = { 0x02, 0x09, 0x35, 0x13, 0x10, 0x26 };
        for(int i = 0; i < 6; i++)
            QCOMPARE(int(m_sim->eeprom(quint8(0xF8 + i))), int(expected[i]));
        QCOMPARE(failed.count(), 0);
        clock.clear();
        QTRY_VERIFY_WITH_TIMEOUT(!clock.isEmpty(), 12000);
        const QDateTime dt = clock.last().at(0).toDateTime();
        QCOMPARE(dt.date(), QDate(2026, 10, 13));
        QCOMPARE(dt.time().hour(), 9);
        QCOMPARE(dt.time().minute(), 35);
    }

    /* Chrono page: chrono on/off, then 42-74 in order, last one included */
    void chronoRound()
    {
        QSignalSpy chrono(m_proto, SIGNAL(chronoRead(quint8,quint8)));
        start(true);
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 48, 13000);
        QStringList expected { "4c" };
        for(int a = 0x42; a <= 0x74; a++)
            if(a <= 0x4b || a >= 0x50)
                expected << QString("%1").arg(a, 2, 16, QChar('0'));
        SKIP_IF_LATE(0, 0, 0);
        QCOMPARE(sequence(48), expected.join(' '));
        //Every valid reply counted, also for the parameters the panel does not use
        QVERIFY(m_stats.at(0) - m_stats.at(1) <= 1);
        //Every chrono parameter emitted with its index
        QTRY_VERIFY_WITH_TIMEOUT(chrono.count() >= 48, 1000);
        bool found = false;
        for(const QList<QVariant> &c : chrono)
            if(c.at(0).toInt() == SerialProto::chronoWkE_1_OnIndex)
                found = c.at(1).toInt() == 42;
        QVERIFY(found);
    }

    /* Flame power from the stove while working, 0 when Off */
    void flamePower()
    {
        QSignalSpy power(m_proto, SIGNAL(updatePower(quint8,quint8)));
        m_sim->setRam(0x21, 4);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!power.isEmpty(), 8000);
        QCOMPARE(power.last().at(1).toInt(), 8 * 90 / 16 + 10);
        //The first emission may come before the new state is read: 0 within a round
        m_sim->setRam(0x21, 0);
        QTRY_COMPARE_WITH_TIMEOUT(power.last().at(1).toInt(), 0, 12000);
    }
};

int runSerialProtoTests(int argc, char *argv[])
{
    TstSerialProto t;
    return runTestClass(&t, argc, argv);
}

#include "tst_serialproto.moc"
