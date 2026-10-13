#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "homeassistant.h"
#include "serialproto.h"
#include "testutil.h"

/*
 * Home Assistant payloads, without a broker (port 1 on localhost: the
 * client keeps trying in the background, nothing is published).
 */
class TstHomeAssistant : public QObject
{
    Q_OBJECT

    MqttConfig config(const QString &id) const
    {
        MqttConfig c;
        c.host = "127.0.0.1";
        c.port = 1;
        c.uniqueId = id;
        return c;
    }

    static QJsonObject json(const QByteArray &b) { return QJsonDocument::fromJson(b).object(); }

private slots:
    void settings()
    {
        QTemporaryDir dir;
        QSettings empty(dir.filePath("a.ini"), QSettings::IniFormat);
        const MqttConfig off = MqttConfig::load(empty);
        QVERIFY(!off.enabled());
        QCOMPARE(int(off.port), 1883);

        QSettings s(dir.filePath("b.ini"), QSettings::IniFormat);
        s.setValue("MQTT/broker_addr", " 192.0.2.10 ");
        s.setValue("MQTT/broker_port", 1884);
        s.setValue("MQTT/broker_uname", "u");
        s.setValue("MQTT/broker_password", "p");
        s.setValue("MQTT/unique_id", "stove");
        const MqttConfig on = MqttConfig::load(s);
        QVERIFY(on.enabled());
        QCOMPARE(on.host, QString("192.0.2.10"));
        QCOMPARE(int(on.port), 1884);
        QCOMPARE(on.user, QString("u"));
        QCOMPARE(on.uniqueId, QString("stove"));
    }

    void topics()
    {
        HomeAssistant ha(SerialProto::getInstance(), config("a b/c"));
        QCOMPARE(ha.baseTopic(), QString("PelletController/a_b_c"));
        QCOMPARE(ha.clientId(), QString("PelletController-a_b_c"));
    }

    /* Ten read only entities: no command topic anywhere */
    void discovery()
    {
        HomeAssistant ha(SerialProto::getInstance(), config("t"));
        const auto d = ha.discovery();
        QCOMPARE(d.size(), 10);
        QStringList keys;
        for(const auto &e : d)
        {
            QVERIFY(e.first.startsWith("homeassistant/sensor/PelletController_t_")
                    || e.first.startsWith("homeassistant/binary_sensor/PelletController_t_"));
            QVERIFY(e.first.endsWith("/config"));
            QVERIFY(!e.second.contains("command_topic"));
            const QJsonObject c = json(e.second);
            QCOMPARE(c.value("state_topic").toString(), QString("PelletController/t/state"));
            const QString uid = c.value("unique_id").toString();
            QVERIFY(e.first.contains("/" + uid + "/"));
            keys << uid.mid(QString("PelletController_t_").size());
            const QJsonArray avail = c.value("availability").toArray();
            QCOMPARE(avail.at(0).toObject().value("topic").toString(), QString("PelletController/t/status"));
            //The link entity stays available when the stove does not answer
            QCOMPARE(avail.size(), uid.endsWith("_link") ? 1 : 2);
            QCOMPARE(c.value("device").toObject().value("name").toString(), QString("Pellet stove"));
        }
        for(const char *k : { "room_temp", "setpoint", "state", "alarm", "flame", "smoke_temp",
                              "smoke_fan", "power", "power_in_use", "link" })
            QVERIFY2(keys.contains(k), k);
    }

    /* State JSON from the SerialProto signals */
    void state()
    {
        SerialProto *p = SerialProto::getInstance();
        HomeAssistant ha(p, config("s"));
        QVERIFY(json(ha.statePayload()).value("room_temp").isNull());

        emit p->updateAmbTemp(20.5f);
        emit p->updateSetTemp(21.5f);
        emit p->updateStoveState(SerialProto::PelletMissing, "Pellet Missing");
        emit p->updatePower(3, 0);
        emit p->updateSmoke(64, 0);
        emit p->updatePowerInUse(0x90);
        QJsonObject s = json(ha.statePayload());
        QCOMPARE(s.value("room_temp").toDouble(), 20.5);
        QCOMPARE(s.value("setpoint").toDouble(), 21.5);
        QCOMPARE(s.value("state").toString(), QString("Pellet Missing"));
        QCOMPARE(s.value("state_code").toInt(), 8);
        QCOMPARE(s.value("alarm").toBool(), true);
        QCOMPARE(s.value("power").toInt(), 3);
        QCOMPARE(s.value("smoke_temp").toInt(), 64);
        QCOMPARE(s.value("power_in_use").toInt(), 0);       //0x90 = none

        emit p->updateStoveState(SerialProto::Working, "Working");
        emit p->updatePowerInUse(2);
        s = json(ha.statePayload());
        QCOMPARE(s.value("alarm").toBool(), false);
        QCOMPARE(s.value("power_in_use").toInt(), 2);
    }
};

int runHomeAssistantTests(int argc, char *argv[])
{
    TstHomeAssistant t;
    return runTestClass(&t, argc, argv);
}

#include "tst_homeassistant.moc"
