#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtMqtt/QMqttClient>

class QSettings;
class QTimer;
class SerialProto;

/* setting.ini [MQTT], the keys of RoomSense / AcquaThermoNet */
struct MqttConfig
{
    QString host;                           //broker_addr, empty: MQTT off
    quint16 port = 1883;                    //broker_port
    QString user;                           //broker_uname, used with the password
    QString password;                       //broker_password
    QString uniqueId;                       //unique_id, empty: host name

    bool enabled() const { return !host.isEmpty(); }
    static MqttConfig load(QSettings &settings);
};

/*
 * Home Assistant through MQTT, read only: the stove values on one retained
 * JSON state topic, MQTT discovery of the entities (sensors and binary
 * sensors, no commands), availability of the panel (status, last will)
 * and of the stove (link). Topics under PelletController/<id>/.
 */
class HomeAssistant : public QObject
{
    Q_OBJECT

public:
    HomeAssistant(SerialProto *proto, const MqttConfig &config, QObject *parent = nullptr);

    /* Exit: offline published, then disconnect (no reconnect) */
    void shutdown();

    /* Diagnostic page */
    QMqttClient::ClientState clientState() const { return m_client->state(); }
    QString broker() const { return QString("%1:%2").arg(m_client->hostname()).arg(m_client->port()); }
    QString clientId() const { return m_client->clientId(); }
    quint32 published() const { return m_published; }

    /* Payloads, public for the tests */
    QString baseTopic() const { return m_base; }
    QByteArray statePayload() const;
    QList<QPair<QString, QByteArray>> discovery() const;   //Topic, retained config

signals:
    void clientStateChanged(int state);
    void publishedChanged(quint32 published);

private:
    void connectToBroker();
    void scheduleReconnect();
    void onStateChanged(QMqttClient::ClientState state);
    void onMessage(const QByteArray &message, const QMqttTopicName &topic);
    void publishAll();
    void publish(const QString &topic, const QByteArray &payload, bool retain);
    void stateChanged();                    //A value changed: publish within STATE_MIN_MS
    void publishState(bool force = false);
    void setLink(bool ok);

    SerialProto *m_proto;
    QMqttClient *m_client;
    QTimer *m_reconnectTimer;
    QTimer *m_connectTimer;
    QTimer *m_stateTimer;
    QString m_id;                           //Unique id: topics and entity ids
    QString m_base;                         //PelletController/<id>
    int m_connectTry = 0;
    bool m_shuttingDown = false;
    bool m_link = false;
    quint32 m_published = 0;
    qint64 m_lastState = 0;                 //ms since epoch of the last state published

    QJsonObject m_state;                    //Values read, null until read
    QByteArray m_lastPayload;               //Last state published, not sent again unchanged
};

#endif // HOMEASSISTANT_H
