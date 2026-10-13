#include "homeassistant.h"

#include "common.h"
#include "serialproto.h"

#include <QDateTime>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

#define TOPIC_ROOT          "PelletController"
#define HA_STATUS           "homeassistant/status"
#define HA_PREFIX           "homeassistant"
#define STATE_MIN_MS        (10 * 1000)     //At most one state message every 10 s
#define CONNECT_TIMEOUT_MS  (30 * 1000)     //No CONNACK: give up and retry

MqttConfig MqttConfig::load(QSettings &settings)
{
    MqttConfig c;
    c.host = settings.value("MQTT/broker_addr").toString().trimmed();
    c.port = quint16(settings.value("MQTT/broker_port", 1883).toUInt());
    c.user = settings.value("MQTT/broker_uname").toString();
    c.password = settings.value("MQTT/broker_password").toString();
    c.uniqueId = settings.value("MQTT/unique_id").toString().trimmed();
    return c;
}

HomeAssistant::HomeAssistant(SerialProto *proto, const MqttConfig &config, QObject *parent)
    : QObject(parent), m_proto(proto)
{
    //Topic and entity id: [A-Za-z0-9_-] only
    m_id = config.uniqueId.isEmpty() ? QHostInfo::localHostName() : config.uniqueId;
    m_id.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
    m_base = QString(TOPIC_ROOT "/%1").arg(m_id);

    for(const char *key : { "room_temp", "setpoint", "state", "state_code", "alarm", "flame",
                            "smoke_temp", "smoke_fan", "power", "power_in_use" })
        m_state.insert(key, QJsonValue::Null);

    m_client = new QMqttClient(this);
    m_client->setHostname(config.host);
    m_client->setPort(config.port);
    if(!config.user.isEmpty() && !config.password.isEmpty())
    {
        m_client->setUsername(config.user);
        m_client->setPassword(config.password);
    }
    m_client->setClientId(QString(TOPIC_ROOT "-%1").arg(m_id));
    m_client->setKeepAlive(30);
    m_client->setWillTopic(m_base + "/status");
    m_client->setWillMessage("offline");
    m_client->setWillQoS(1);
    m_client->setWillRetain(true);
    qInfo() << "MQTT" << m_client->clientId() << "broker" << config.host << config.port;

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_connectTimer = new QTimer(this);
    m_connectTimer->setSingleShot(true);
    m_stateTimer = new QTimer(this);
    m_stateTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &HomeAssistant::connectToBroker);
    connect(m_connectTimer, &QTimer::timeout, m_client, &QMqttClient::disconnectFromHost);
    connect(m_stateTimer, &QTimer::timeout, this, [this] { publishState(); });
    connect(m_client, &QMqttClient::stateChanged, this, &HomeAssistant::onStateChanged);
    connect(m_client, &QMqttClient::messageReceived, this, &HomeAssistant::onMessage);

    /* Stove values */
    connect(m_proto, &SerialProto::updateAmbTemp, this, [this](float t) { m_state["room_temp"] = double(t); stateChanged(); });
    connect(m_proto, &SerialProto::updateSetTemp, this, [this](float t) { m_state["setpoint"] = double(t); stateChanged(); });
    connect(m_proto, &SerialProto::updateStoveState, this, [this](quint8 code, const QString &name) {
        m_state["state"] = name;
        m_state["state_code"] = code;
        m_state["alarm"] = (code >= SerialProto::PelletMissing && code <= SerialProto::Alarm);
        stateChanged();
    });
    connect(m_proto, &SerialProto::updatePower, this, [this](quint8 set, quint8 flame) {
        m_state["power"] = set;
        m_state["flame"] = flame;
        stateChanged();
    });
    connect(m_proto, &SerialProto::updateSmoke, this, [this](quint8 temp, quint16 rpm) {
        m_state["smoke_temp"] = temp;
        m_state["smoke_fan"] = rpm;
        stateChanged();
    });
    connect(m_proto, &SerialProto::updatePowerInUse, this, [this](quint8 p) {
        m_state["power_in_use"] = (p >= POWER_MIN && p <= POWER_MAX) ? QJsonValue(p) : QJsonValue(0);
        stateChanged();
    });
    connect(m_proto, &SerialProto::linkStateChanged, this, [this](quint8 l) { setLink(SerialProto::LinkOk == l); });
    m_link = (SerialProto::LinkOk == m_proto->linkState());

    connectToBroker();
}

void HomeAssistant::connectToBroker()
{
    if(m_shuttingDown)
        return;
    m_connectTimer->start(CONNECT_TIMEOUT_MS);
    m_client->connectToHost();
}

/* Backoff 1, 2, 5, 10, then 30 s, as RoomSense and AcquaThermoNet */
void HomeAssistant::scheduleReconnect()
{
    static const int backoff[] = { 1000, 2000, 5000, 10000, 30000 };
    if(m_shuttingDown || m_reconnectTimer->isActive())
        return;
    m_reconnectTimer->start(backoff[qMin(m_connectTry, 4)]);
    m_connectTry++;
}

void HomeAssistant::onStateChanged(QMqttClient::ClientState state)
{
    emit clientStateChanged(int(state));
    if(QMqttClient::Connected == state)
    {
        qInfo() << "MQTT connected";
        m_connectTimer->stop();
        m_connectTry = 0;
        m_client->subscribe(QMqttTopicFilter(HA_STATUS), 1);
        publishAll();
    }
    else if(QMqttClient::Disconnected == state)
    {
        m_connectTimer->stop();
        if(!m_shuttingDown)
            qInfo() << "MQTT disconnected, error" << m_client->error();
        scheduleReconnect();
    }
}

/* Home Assistant restarted: discovery and states again */
void HomeAssistant::onMessage(const QByteArray &message, const QMqttTopicName &topic)
{
    if(topic.name() == HA_STATUS && message == "online")
    {
        qInfo() << "Home Assistant online, discovery published again";
        publishAll();
    }
}

void HomeAssistant::publish(const QString &topic, const QByteArray &payload, bool retain)
{
    if(QMqttClient::Connected != m_client->state())
        return;
    if(m_client->publish(QMqttTopicName(topic), payload, 1, retain) >= 0)
        emit publishedChanged(++m_published);
}

void HomeAssistant::publishAll()
{
    publish(m_base + "/status", "online", true);
    for(const auto &d : discovery())
        publish(d.first, d.second, true);
    publish(m_base + "/link", m_link ? "online" : "offline", true);
    publishState(true);
}

void HomeAssistant::setLink(bool ok)
{
    if(ok == m_link)
        return;
    m_link = ok;
    publish(m_base + "/link", ok ? "online" : "offline", true);
    stateChanged();
}

/* A value changed: at once if the last state is old enough, otherwise when
 * STATE_MIN_MS has passed (the values read meanwhile go in the same message) */
void HomeAssistant::stateChanged()
{
    if(m_stateTimer->isActive())
        return;
    const qint64 since = QDateTime::currentMSecsSinceEpoch() - m_lastState;
    m_stateTimer->start(int(qBound<qint64>(0, STATE_MIN_MS - since, STATE_MIN_MS)));
}

/* Only when it changed (the polling reads the same values again and
 * again), but always after a connection or a Home Assistant restart */
void HomeAssistant::publishState(bool force)
{
    if(QMqttClient::Connected != m_client->state())
        return;
    const QByteArray payload = statePayload();
    if(!force && payload == m_lastPayload)
        return;
    m_lastPayload = payload;
    m_lastState = QDateTime::currentMSecsSinceEpoch();
    publish(m_base + "/state", payload, true);
}

QByteArray HomeAssistant::statePayload() const
{
    QJsonObject s = m_state;
    s.insert("link", m_link);
    return QJsonDocument(s).toJson(QJsonDocument::Compact);
}

/* One discovery config per entity, read only (no command topics): the
 * values on the state topic, available while the panel is online and, but
 * for the link itself, while the stove answers */
QList<QPair<QString, QByteArray>> HomeAssistant::discovery() const
{
    struct Entity {
        const char *component, *key, *name, *unit, *deviceClass, *stateClass, *icon, *value;
    };
    static const Entity entities[] = {
        { "sensor", "room_temp", "Room temperature", "°C", "temperature", "measurement", nullptr, nullptr },
        { "sensor", "setpoint", "Setpoint", "°C", "temperature", nullptr, "mdi:thermostat", nullptr },
        { "sensor", "state", "State", nullptr, nullptr, nullptr, "mdi:fireplace", nullptr },
        { "binary_sensor", "alarm", "Alarm", nullptr, "problem", nullptr, nullptr,
          "{{ 'ON' if value_json.alarm else 'OFF' }}" },
        { "sensor", "flame", "Flame", "%", nullptr, "measurement", "mdi:fire", nullptr },
        { "sensor", "smoke_temp", "Smoke temperature", "°C", "temperature", "measurement", nullptr, nullptr },
        { "sensor", "smoke_fan", "Smoke fan", "rpm", nullptr, "measurement", "mdi:fan", nullptr },
        { "sensor", "power", "Power", nullptr, nullptr, nullptr, "mdi:speedometer", nullptr },
        { "sensor", "power_in_use", "Power in use", nullptr, nullptr, "measurement", "mdi:speedometer", nullptr },
        { "binary_sensor", "link", "Stove link", nullptr, "connectivity", nullptr, nullptr,
          "{{ 'ON' if value_json.link else 'OFF' }}" },
    };

    QJsonObject device;
    device.insert("identifiers", QJsonArray { QString(TOPIC_ROOT "_%1").arg(m_id) });
    device.insert("name", "Pellet stove");
    device.insert("model", SW_NAME);
    device.insert("sw_version", SW_VER);

    QList<QPair<QString, QByteArray>> out;
    for(const Entity &e : entities)
    {
        const QString uid = QString(TOPIC_ROOT "_%1_%2").arg(m_id, e.key);
        QJsonObject c;
        c.insert("name", e.name);
        c.insert("unique_id", uid);
        c.insert("state_topic", m_base + "/state");
        c.insert("value_template", e.value ? QString(e.value) : QString("{{ value_json.%1 }}").arg(e.key));
        if(e.unit)          c.insert("unit_of_measurement", QString::fromUtf8(e.unit));
        if(e.deviceClass)   c.insert("device_class", e.deviceClass);
        if(e.stateClass)    c.insert("state_class", e.stateClass);
        if(e.icon)          c.insert("icon", e.icon);
        QJsonArray availability { QJsonObject { { "topic", m_base + "/status" } } };
        if(QString(e.key) != "link")
            availability.append(QJsonObject { { "topic", m_base + "/link" } });
        c.insert("availability", availability);
        c.insert("availability_mode", "all");
        c.insert("device", device);
        out.append(qMakePair(QString(HA_PREFIX "/%1/%2/config").arg(e.component, uid),
                             QJsonDocument(c).toJson(QJsonDocument::Compact)));
    }
    return out;
}

void HomeAssistant::shutdown()
{
    m_shuttingDown = true;
    m_reconnectTimer->stop();
    m_connectTimer->stop();
    m_stateTimer->stop();
    if(QMqttClient::Connected == m_client->state())
        m_client->publish(QMqttTopicName(m_base + "/status"), "offline", 1, true);
    m_client->disconnectFromHost();
}
