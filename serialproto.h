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

#define SERIAL_MESSAGE_DELAY    (200)
#define SERIAL_SESSION_DELAY    (SERIAL_MESSAGE_DELAY*10)

#define ramParam(param)         (((quint16)ramBank << 8) | param)
#define eepromParam(param)      (((quint16)eepromBank << 8) | param)
#define requestsMapSize         ((sizeof(requestsMap)/sizeof(requestsMap[0]))-1) //Tolgo uno perchè all'interno della struttura ho una posizione "Fake"
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

#define chronoEnableAddr            0x4C

#define chronoWkE_EnableAddr        0x42
#define chronoWkE_1_OnAddr          0x43
#define chronoWkE_1_OffAddr         0x44
#define chronoWkE_2_OnAddr          0x45
#define chronoWkE_2_OffAddr         0x46

#define chronoDay_EnableAddr        0x47
#define chronoDay_1_OnAddr          0x48
#define chronoDay_1_OffAddr         0x49
#define chronoDay_2_OnAddr          0x4A
#define chronoDay_2_OffAddr         0x4B

#define chronoSet_EnableAddr        0x50
#define chronoSet_1_OnAddr          0x51
#define chronoSet_1_OffAddr         0x52
#define chronoSet_1_LunEnabAddr     0x53
#define chronoSet_1_MarEnabAddr     0x54
#define chronoSet_1_MerEnabAddr     0x55
#define chronoSet_1_GioEnabAddr     0x56
#define chronoSet_1_VenEnabAddr     0x57
#define chronoSet_1_SabEnabAddr     0x58
#define chronoSet_1_DomEnabAddr     0x59
#define chronoSet_2_OnAddr          0x5A
#define chronoSet_2_OffAddr         0x5B
#define chronoSet_2_LunEnabAddr     0x5C
#define chronoSet_2_MarEnabAddr     0x5D
#define chronoSet_2_MerEnabAddr     0x5E
#define chronoSet_2_GioEnabAddr     0x5F
#define chronoSet_2_VenEnabAddr     0x60
#define chronoSet_2_SabEnabAddr     0x61
#define chronoSet_2_DomEnabAddr     0x62
#define chronoSet_3_OnAddr          0x63
#define chronoSet_3_OffAddr         0x64
#define chronoSet_3_LunEnabAddr     0x65
#define chronoSet_3_MarEnabAddr     0x66
#define chronoSet_3_MerEnabAddr     0x67
#define chronoSet_3_GioEnabAddr     0x68
#define chronoSet_3_VenEnabAddr     0x69
#define chronoSet_3_SabEnabAddr     0x6A
#define chronoSet_3_DomEnabAddr     0x6B
#define chronoSet_4_OnAddr          0x6C
#define chronoSet_4_OffAddr         0x6D
#define chronoSet_4_LunEnabAddr     0x6E
#define chronoSet_4_MarEnabAddr     0x6F
#define chronoSet_4_MerEnabAddr     0x70
#define chronoSet_4_GioEnabAddr     0x71
#define chronoSet_4_VenEnabAddr     0x72
#define chronoSet_4_SabEnabAddr     0x73
#define chronoSet_4_DomEnabAddr     0x74
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
        /*********/
        chronoEnableIndex,
        /*********/
        chronoWkE_EnableIndex,
        chronoWkE_1_OnIndex,
        chronoWkE_1_OffIndex,
        chronoWkE_2_OnIndex,
        chronoWkE_2_OffIndex,
        /*********/
        chronoDay_EnableIndex,
        chronoDay_1_OnIndex,
        chronoDay_1_OffIndex,
        chronoDay_2_OnIndex,
        chronoDay_2_OffIndex,
        /*********/
        chronoSet_EnableIndex,
        /*********/
        chronoSet_1_OnIndex,
        chronoSet_1_OffIndex,
        chronoSet_1_LunEnabIndex,
        chronoSet_1_MarEnabIndex,
        chronoSet_1_MerEnabIndex,
        chronoSet_1_GioEnabIndex,
        chronoSet_1_VenEnabIndex,
        chronoSet_1_SabEnabIndex,
        chronoSet_1_DomEnabIndex,
        /*********/
        chronoSet_2_OnIndex,
        chronoSet_2_OffIndex,
        chronoSet_2_LunEnabIndex,
        chronoSet_2_MarEnabIndex,
        chronoSet_2_MerEnabIndex,
        chronoSet_2_GioEnabIndex,
        chronoSet_2_VenEnabIndex,
        chronoSet_2_SabEnabIndex,
        chronoSet_2_DomEnabIndex,
        /*********/
        chronoSet_3_OnIndex,
        chronoSet_3_OffIndex,
        chronoSet_3_LunEnabIndex,
        chronoSet_3_MarEnabIndex,
        chronoSet_3_MerEnabIndex,
        chronoSet_3_GioEnabIndex,
        chronoSet_3_VenEnabIndex,
        chronoSet_3_SabEnabIndex,
        chronoSet_3_DomEnabIndex,
        /*********/
        chronoSet_4_OnIndex,
        chronoSet_4_OffIndex,
        chronoSet_4_LunEnabIndex,
        chronoSet_4_MarEnabIndex,
        chronoSet_4_MerEnabIndex,
        chronoSet_4_GioEnabIndex,
        chronoSet_4_VenEnabIndex,
        chronoSet_4_SabEnabIndex,
        chronoSet_4_DomEnabIndex,

        LastIndex
    };
    Q_ENUM(RequestsIndex)

    const struct {
        uint8_t page;
        uint8_t address;
        char    description[50];
        uint8_t offset;
        float scale;
    } requestsMap[63] = {
        /*********/
        /* 00 */ramBank,    ambTempAddr,                "Temp. ambiente",                   0,  2.0,
        /* 01 */ramBank,    cochleaTurnsAddr,           "Ton di ON coclea",                 0,  0.1,
        /* 02 */ramBank,    stoveStateAddr,             "Stato funzionamento",              0,  1,
        /* 03 */ramBank,    flamePowerAddr,             "Potenza fiamma",                   0,  1,
        /* 04 */ramBank,    smokeFanSpeedAddr,          "Giri ventola fumi",              250,  10,
        /* 05 */ramBank,    smokeTempAddr,              "Temperatura fumi",                 0,  1,
        /* 06 */ramBank,    secondsCurrentAddr,         "Secondi attuali",                  0,  1,
        /* 07 */ramBank,    dayOfWeekAddr,              "Giorno settimana",                 0,  1,
        /* 08 */ramBank,    hoursCurrentAddr,           "Ora attuale",                      0,  1,
        /* 09 */ramBank,    minutesCurrentAddr,         "Minuti attuali",                   0,  1,
        /* 10 */ramBank,    dayOfMonthCurrentAddr,      "Giorno",                           0,  1,
        /* 11 */ramBank,    monthCurrentAddr,           "Mese",                             0,  1,
        /* 12 */ramBank,    yearCurrentAddr,            "Anno",                             0,  1,
        /*********/
        /* 13 */eepromBank, tempSetAddr,                "Temp SetPoint",                    0,  2.0,
        /* 14 */eepromBank, powerSetAddr,               "Potenza impostata",                0,  1,
        /*********/
        /* 15 */eepromBank, chronoEnableAddr,           "Crono abilitato",                  0,  1,
        /*********/
        /* 16 */eepromBank, chronoWkE_EnableAddr,       "Crono WeekEnd. abilitato",         0,  1,
        /* 17 */eepromBank, chronoWkE_1_OnAddr,         "Crono WeekEnd. 1 ON Time",         0,  10,
        /* 18 */eepromBank, chronoWkE_1_OffAddr,        "Crono WeekEnd. 1 ON Time",         0,  10,
        /* 19 */eepromBank, chronoWkE_2_OnAddr,         "Crono WeekEnd. 2 OFF Time",        0,  10,
        /* 20 */eepromBank, chronoWkE_2_OffAddr,        "Crono WeekEnd. 2 OFF Time",        0,  10,
        /*********/
        /* 21 */eepromBank, chronoDay_EnableAddr,       "Crono Gior. abilitato",            0,  1,
        /* 22 */eepromBank, chronoDay_1_OnAddr,         "Crono Gior. 1 ON Time",            0,  10,
        /* 23 */eepromBank, chronoDay_1_OffAddr,        "Crono Gior. 1 ON Time",            0,  10,
        /* 24 */eepromBank, chronoDay_2_OnAddr,         "Crono Gior. 2 OFF Time",           0,  10,
        /* 25 */eepromBank, chronoDay_2_OffAddr,        "Crono Gior. 2 OFF Time",           0,  10,
        /*********/
        /* 26 */eepromBank, chronoSet_EnableAddr,       "Crono Sett. abilitato",            0,  1,
        /* 27 */eepromBank, chronoSet_1_OnAddr,         "Crono Sett. 1 ON Time",            0,  10,
        /* 28 */eepromBank, chronoSet_1_OffAddr,        "Crono Sett. 1 OFF Time",           0,  10,
        /* 29 */eepromBank, chronoSet_1_LunEnabAddr,    "Crono Sett. 1 Lun abilitato",      0,  1,
        /* 30 */eepromBank, chronoSet_1_MarEnabAddr,    "Crono Sett. 1 Mar abilitato",      0,  1,
        /* 31 */eepromBank, chronoSet_1_MerEnabAddr,    "Crono Sett. 1 Mer abilitato",      0,  1,
        /* 32 */eepromBank, chronoSet_1_GioEnabAddr,    "Crono Sett. 1 Gio abilitato",      0,  1,
        /* 33 */eepromBank, chronoSet_1_VenEnabAddr,    "Crono Sett. 1 Ven abilitato",      0,  1,
        /* 34 */eepromBank, chronoSet_1_SabEnabAddr,    "Crono Sett. 1 Sab abilitato",      0,  1,
        /* 35 */eepromBank, chronoSet_1_DomEnabAddr,    "Crono Sett. 1 Dom abilitato",      0,  1,
        /*********/
        /* 36 */eepromBank, chronoSet_2_OnAddr,         "Crono Sett. 2 ON Time",            0,  10,
        /* 37 */eepromBank, chronoSet_2_OffAddr,        "Crono Sett. 2 OFF Time",           0,  10,
        /* 38 */eepromBank, chronoSet_2_LunEnabAddr,    "Crono Sett. 2 Lun abilitato",      0,  1,
        /* 39 */eepromBank, chronoSet_2_MarEnabAddr,    "Crono Sett. 2 Mar abilitato",      0,  1,
        /* 40 */eepromBank, chronoSet_2_MerEnabAddr,    "Crono Sett. 2 Mer abilitato",      0,  1,
        /* 41 */eepromBank, chronoSet_2_GioEnabAddr,    "Crono Sett. 2 Gio abilitato",      0,  1,
        /* 42 */eepromBank, chronoSet_2_VenEnabAddr,    "Crono Sett. 2 Ven abilitato",      0,  1,
        /* 43 */eepromBank, chronoSet_2_SabEnabAddr,    "Crono Sett. 2 Sab abilitato",      0,  1,
        /* 44 */eepromBank, chronoSet_2_DomEnabAddr,    "Crono Sett. 2 Dom abilitato",      0,  1,
        /*********/
        /* 45 */eepromBank, chronoSet_3_OnAddr,         "Crono Sett. 3 ON Time",            0,  10,
        /* 46 */eepromBank, chronoSet_3_OffAddr,        "Crono Sett. 3 OFF Time",           0,  10,
        /* 47 */eepromBank, chronoSet_3_LunEnabAddr,    "Crono Sett. 3 Lun abilitato",      0,  1,
        /* 48 */eepromBank, chronoSet_3_MarEnabAddr,    "Crono Sett. 3 Mar abilitato",      0,  1,
        /* 49 */eepromBank, chronoSet_3_MerEnabAddr,    "Crono Sett. 3 Mer abilitato",      0,  1,
        /* 50 */eepromBank, chronoSet_3_GioEnabAddr,    "Crono Sett. 3 Gio abilitato",      0,  1,
        /* 51 */eepromBank, chronoSet_3_VenEnabAddr,    "Crono Sett. 3 Ven abilitato",      0,  1,
        /* 52 */eepromBank, chronoSet_3_SabEnabAddr,    "Crono Sett. 3 Sab abilitato",      0,  1,
        /* 53 */eepromBank, chronoSet_3_DomEnabAddr,    "Crono Sett. 3 Dom abilitato",      0,  1,
        /*********/
        /* 54 */eepromBank, chronoSet_4_OnAddr,         "Crono Sett. 4 ON Time",            0,  10,
        /* 55 */eepromBank, chronoSet_4_OffAddr,        "Crono Sett. 4 OFF Time",           0,  10,
        /* 56 */eepromBank, chronoSet_4_LunEnabAddr,    "Crono Sett. 4 Lun abilitato",      0,  1,
        /* 57 */eepromBank, chronoSet_4_MarEnabAddr,    "Crono Sett. 4 Mar abilitato",      0,  1,
        /* 58 */eepromBank, chronoSet_4_MerEnabAddr,    "Crono Sett. 4 Mer abilitato",      0,  1,
        /* 59 */eepromBank, chronoSet_4_GioEnabAddr,    "Crono Sett. 4 Gio abilitato",      0,  1,
        /* 60 */eepromBank, chronoSet_4_VenEnabAddr,    "Crono Sett. 4 Ven abilitato",      0,  1,
        /* 61 */eepromBank, chronoSet_4_SabEnabAddr,    "Crono Sett. 4 Sab abilitato",      0,  1,
        /* 62 */eepromBank, chronoSet_4_DomEnabAddr,    "Crono Sett. 4 Dom abilitato",      0,  1,
        /*********/
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
    void writeChronoEnable(bool val);
    void setChronoSerialGet(bool newChronoSerialGet);

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
    bool m_chronoSerialGet;

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
    void updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors);
    void updateAmbTemp(float ambTemp);
    void updateSetTemp(float setTemp);
    void updateStoveState(quint8 stoveState, QString stoveStateStr);
    void updateAmbTemp(quint16 ambTempDC, quint16 setTempDC);
    void updatePower(quint8 setPower, quint8 flamePower);
    void updateSmoke(quint8 smokeTemp, quint8 smokeFanSpeed);
    void updateChronoEnable(bool chronoEnable);
    void updateChronoWkEEnable(bool chronoWkEEnable);
    void updateChronoWkE1On(quint8 chronoWkE1On);
    void updateChronoWkE1Off(quint8 chronoWkE1Off);
    void updateChronoWkE2On(quint8 chronoWkE2On);
    void updateChronoWkE2Off(quint8 chronoWkE2Off);
    void updateChronoDayEnable(bool chronoWkEEnable);
    void updateChronoDay1On(quint8 chronoWkE1On);
    void updateChronoDay1Off(quint8 chronoWkE1Off);
    void updateChronoDay2On(quint8 chronoWkE2On);
    void updateChronoDay2Off(quint8 chronoWkE2Off);

    void updateStoveDateTime(QDateTime stoveDateTime);

private slots:
    void checkStoveReply();
    void getStoveInfos();
    void bytesWritten(qint64 bytes);
};

#endif // SERIALPROTO_H
