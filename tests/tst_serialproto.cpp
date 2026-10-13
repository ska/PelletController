#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "serialproto.h"
#include "stovesim.h"

/*
 * SerialProto against the stove simulator (tools/stovesim), in real time:
 * 200 ms per request as on the panel. Never a real port: the port name is
 * a symlink in a temporary directory.
 */
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

    /* Counters of the last updateStats: Tx, Rx, Err, Chk, Tmo */
    static QList<quint32> stats(const QSignalSpy &spy, int i = -1)
    {
        const QList<QVariant> a = spy.at(i < 0 ? spy.count() - 1 : i);
        return { a.at(0).toUInt(), a.at(1).toUInt(), a.at(2).toUInt(), a.at(3).toUInt(), a.at(4).toUInt() };
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
    }

    void init()
    {
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
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 30, 10000);
        QCOMPARE(sequence(30), QString("01 21 0d 21 34 21 37 21 5a 21 65 21 66 21 67 21 68 21 "
                                       "69 21 6a 21 6b 21 7d 21 7f 21 4c 21"));
        QCOMPARE(amb.first().at(0).toFloat(), 20.0f);
        QCOMPARE(set.first().at(0).toFloat(), 22.5f);
        QCOMPARE(state.first().at(1).toString(), QString("Off"));
    }

    /* Wrong checksum: Chk + 1, the same parameter read again once */
    void checksumRetry()
    {
        QSignalSpy st(m_proto, SIGNAL(updateStats(quint32,quint32,quint32,quint32,quint32)));
        m_sim->corruptChecksums(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 5, 3000);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QTRY_VERIFY_WITH_TIMEOUT(st.count() >= 4, 2000);
        QCOMPARE(stats(st)[3], stats(st, 0)[3] + 1);
        QCOMPARE(stats(st)[2], stats(st, 0)[2]);
    }

    /* Wrong echo: Err + 1, read again once (issue 8) */
    void echoRetry()
    {
        QSignalSpy st(m_proto, SIGNAL(updateStats(quint32,quint32,quint32,quint32,quint32)));
        m_sim->corruptEchoes(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 5, 3000);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QTRY_VERIFY_WITH_TIMEOUT(st.count() >= 4, 2000);
        QCOMPARE(stats(st)[2], stats(st, 0)[2] + 1);
    }

    /* Nothing comes back (not even the echo): Tmo + 1, read again once */
    void lostRequest()
    {
        QSignalSpy st(m_proto, SIGNAL(updateStats(quint32,quint32,quint32,quint32,quint32)));
        m_sim->dropRequests(1, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 5, 3000);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
        QTRY_VERIFY_WITH_TIMEOUT(st.count() >= 4, 2000);
        QCOMPARE(stats(st)[4], stats(st, 0)[4] + 1);
        QCOMPARE(stats(st)[2], stats(st, 0)[2]);
        QCOMPARE(stats(st)[3], stats(st, 0)[3]);
    }

    /* The retry fails too: no second retry */
    void retryOnlyOnce()
    {
        m_sim->corruptChecksums(2, 0x21);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 5, 3000);
        QCOMPARE(sequence(5), QString("01 21 21 0d 21"));
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

    /* USB adapter unplugged and plugged back (issue 10) */
    void unplugReplug()
    {
        QSignalSpy link(m_proto, SIGNAL(linkStateChanged(quint8)));
        QSignalSpy failed(m_proto, SIGNAL(writeFailed(quint8,quint8,quint8)));
        QSignalSpy amb(m_proto, SIGNAL(updateAmbTemp(float)));
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!amb.isEmpty(), 2000);
        m_sim->unplug();
        QTRY_VERIFY_WITH_TIMEOUT(!link.isEmpty() && link.last().at(0).toInt() == SerialProto::LinkPortClosed, 3000);
        m_proto->writeChronoEnable(true);
        QCOMPARE(failed.count(), 1);
        QTest::qWait(4000);                 //At least one failed reopen
        QVERIFY(m_sim->plug());
        amb.clear();
        QTRY_VERIFY_WITH_TIMEOUT(link.last().at(0).toInt() == SerialProto::LinkOk, 6000);
        QTRY_VERIFY_WITH_TIMEOUT(!amb.isEmpty(), 10000);
    }

    /* Chrono page: chrono on/off, then 42-74 in order, last one included (issue 6) */
    void chronoRound()
    {
        start(true);
        QTRY_VERIFY_WITH_TIMEOUT(m_sim->requests().size() >= 48, 13000);
        QStringList expected { "4c" };
        for(int a = 0x42; a <= 0x74; a++)
            if(a <= 0x4b || a >= 0x50)
                expected << QString("%1").arg(a, 2, 16, QChar('0'));
        QCOMPARE(sequence(48), expected.join(' '));
    }

    /* Flame power from the stove while working, 0 when Off (issue 11) */
    void flamePower()
    {
        QSignalSpy power(m_proto, SIGNAL(updatePower(quint8,quint8)));
        m_sim->setRam(0x21, 4);
        start();
        QTRY_VERIFY_WITH_TIMEOUT(!power.isEmpty(), 8000);
        QCOMPARE(power.last().at(1).toInt(), 8 * 90 / 16 + 10);
        m_sim->setRam(0x21, 0);
        const int n = power.count();
        QTRY_VERIFY_WITH_TIMEOUT(power.count() > n, 10000);
        QCOMPARE(power.last().at(1).toInt(), 0);
    }
};

int runSerialProtoTests(int argc, char *argv[])
{
    TstSerialProto t;
    return QTest::qExec(&t, argc, argv);
}

#include "tst_serialproto.moc"
