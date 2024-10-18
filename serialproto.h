#ifndef SERIALPROTO_H
#define SERIALPROTO_H

#include <QDebug>
#include <QTimer>
#include <QObject>
#include <QDateTime>
//#include <QJSEngine>
//#include <QQmlEngine>
#include <QStandardItemModel>

#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>


#define requestsMapSize         (sizeof(requestsMap)/sizeof(requestsMap[0]))
/* Stove Definitions */
#define readCmd                 0x00
#define writeCmd                0x80

#define ramBank                 0x00
#define eepromBank              0x20

/* Ram Address */
#define ambTempAddr             0x01
#define cochleaTurnsAddr        0x0D
#define stoveStateAddr          0x21
#define flamePowerAddr          0x34
#define smokeFanSpeedAddr       0x37
#define smokeTempAddr           0x5A

#define secondsCurrentAddr      0x65
#define dayOfWeekAddr           0x66
#define hoursCurrentAddr        0x67
#define minutesCurrentAddr      0x68
#define dayOfMonthCurrentAddr   0x69
#define monthCurrentAddr        0x6A
#define yearCurrentAddr         0x6b
//
/* EEprom Address */
#define tempSetAddr             0x7D
#define powerSetAddr            0x7F

#define chronoEnableAddr        0x4C
#define chrono1OnAddr           0x4D
#define chrono1OffAddr          0x4E
#if 0
struct serialPortStruct: public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString descr MEMBER m_descr);
    Q_PROPERTY(QString value MEMBER m_value);

public:
    QString m_descr;
    QString m_value;
};
Q_DECLARE_METATYPE(serialPortStruct)
#endif
class SerialProto : public QObject
{
    Q_OBJECT
    //Q_PROPERTY(QList<serialPortStruct *> serialPortStructs READ serialPortStructs NOTIFY serialPortStructsChanged);
    //Q_PROPERTY(QString serPort                             WRITE setSerPort)

public:

    enum StoveState {
        Off = 0,
        Starting,
        PelletLoading,
        Ignition,
        Working,
        BrazierCleaning,
        FinalCleaning,
        Standby,
        PelletMissing,
        IgnitionFailure,
        Alarm
    };
    Q_ENUM(StoveState)
    const QStringList m_stoveStateStr = {
        "Off",
        "Starting",
        "Pellet Loading",
        "Ignition",
        "Working",
        "Brazier Cleaning",
        "Final Cleaning",
        "Standby",
        "Pellet Missing",
        "Ignition Failure",
        "Alarm"
    };

    enum RequestsIndex
    {
        ambTempIndex,
        cochleaTurnsIndex,
        stoveStateIndex,
        flamePowerIndex,
        smokeFanSpeedIndex,
        smokeTempIndex,
        secondsCurrentIndex,
        dayOfWeekIndex,
        hoursCurrentIndex,
        minutesCurrentIndex,
        dayOfMonthCurrentIndex,
        monthCurrentIndex,
        yearCurrentIndex,
        tempSetIndex,
        powerSetIndex,
        chronoEnableIndex,
        chrono1OnIndex,
        chrono1OffIndex
    };
    Q_ENUM(RequestsIndex)

    const struct {
        uint8_t page;
        uint8_t address;
        char    description[50];
        uint8_t offset;
        float scale;
    } requestsMap[19] = {
        /* 00 */ramBank,    ambTempAddr,            "Temp. ambiente",       0,  2.0,
        /* 01 */ramBank,    cochleaTurnsAddr,       "Ton di ON coclea",     0,  0.1,
        /* 02 */ramBank,    stoveStateAddr,         "Stato funzionamento",  0,  1,
        /* 03 */ramBank,    flamePowerAddr,         "Potenza fiamma",       0,  1,
        /* 04 */ramBank,    smokeFanSpeedAddr,      "Giri ventola fumi",  250,  10,
        /* 05 */ramBank,    smokeTempAddr,          "Temperatura fumi",     0,  1,
        /* 06 */ramBank,    secondsCurrentAddr,     "Secondi attuali",      0,  1,
        /* 07 */ramBank,    dayOfWeekAddr,          "Giorno settimana",     0,  1,
        /* 08 */ramBank,    hoursCurrentAddr,       "Ora attuale",          0,  1,
        /* 09 */ramBank,    minutesCurrentAddr,     "Minuti attuali",       0,  1,
        /* 10 */ramBank,    dayOfMonthCurrentAddr,  "Giorno",             0,  1,
        /* 11 */ramBank,    monthCurrentAddr,       "Mese",                 0,  1,
        /* 12 */ramBank,    yearCurrentAddr,        "Anno",                 0,  1,

        /* 13 */eepromBank, tempSetAddr,            "Temp SetPoint",        0,  2.0,
        /* 14 */eepromBank, powerSetAddr,           "Potenza impostata",    0,  1,

        /* 15 */eepromBank, chronoEnableAddr,       "Crono abilitato",      0,  1,
        /* 16 */eepromBank, chrono1OnAddr,          "Crono 1 ON",           0,  10,
        /* 17 */eepromBank, chrono1OffAddr,         "Crono 1 OFF",          0,  10,

        /* LAST */0x00,     0x00,               "",                     0,  0,
    };

    static SerialProto *getInstance();
    //static QObject* createSingletonInstance(QQmlEngine *engine,  QJSEngine *scriptEngine);
    //QList<serialPortStruct *> serialPortStructs();
    void setSerPort(const QString &serPortName);
    void openSerPort();
    void closeSerPort();
    void startSerLoop();
    void stopSerLoop();
    void writeStoveStateOn();
    void writeStoveStateOff();
    void writeStoveStateOffForce();
    void writeStoveDecPower();
    void writeStoveIncPower();
    void writeStoveDecSetPoint();
    void writeStoveIncSetPoint();
private:
    static SerialProto *instance;
    explicit SerialProto(QObject *parent = nullptr);

    //QList<serialPortStruct *> m_serialPortStructs;
    QString m_selSerial;
    QSerialPort m_serial;
    QTimer m_loopTimer;
    quint8 m_state, m_previousState, m_nextState;

    quint8 m_stoveState, m_smokeTemp, m_flamePower, m_setPower, m_smokeFanSpeed;
    quint16 m_ambTempDC, m_setTempDC;
    quint32 m_txMessages, m_rxMessages, m_rxErrors;
    float m_ambTemp, m_setTemp, m_tonCochlea;
    quint8 m_stoveYear, m_stoveMonth, m_stoveDay, m_stoveDoW,
        m_stoveHour, m_stoveMinutes, m_stoveSeconds;

    void readStoveInfo(quint8, quint8);
    void writeStoveCmd(quint8 page, quint8 address, quint8 value);
    void sendData(QByteArray data);
signals:
    void serialPortStructsChanged();
    void serialOpened();
    void serialClosed();
    void serialError(QString);
    void loopStarted();
    void loopStopped();
    void updateStats(quint32 txMessages, quint32 rxMessages);
    void updateAmbTemp(float ambTemp);
    void updateSetTemp(float setTemp);
    void updateStoveState(quint8 stoveState, QString stoveStateStr);
    void updateAmbTemp(quint16 ambTempDC, quint16 setTempDC);
    void updatePower(quint8 setPower, quint8 flamePower);
    void updateSmoke(quint8 smokeTemp, quint8 smokeFanSpeed);

    //, quint16 ambTempDC, quint8 fumesTemp, quint8 flamePower);
    void updateStoveDateTime(QDateTime stoveDateTime);

private slots:
    void checkStoveReply();
    void getStoveInfos();
    void bytesWritten(qint64 bytes);
};

#endif // SERIALPROTO_H
