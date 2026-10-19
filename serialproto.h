#ifndef SERIALPROTO_H
#define SERIALPROTO_H

#include <QDebug>
#include <QTimer>
#include <QQueue>
#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>

#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>

#define SERIAL_MESSAGE_DELAY    (200)
#define SERIAL_SESSION_DELAY    (SERIAL_MESSAGE_DELAY*10)
#define SERIAL_RX_IDLE_TIMEOUT  (SERIAL_MESSAGE_DELAY/2)    //Linea muta per questo tempo = risposta completa
#define SERIAL_WRITE_DELAY      (150)                       //Attesa prima di una scrittura
#define SERIAL_WRITE_TIMEOUT    (SERIAL_MESSAGE_DELAY*3)    //Attesa massima risposta a scrittura
#define SERIAL_WRITE_ATTEMPTS   (2)                         //Write sent again once if not confirmed
#define SERIAL_REOPEN_DELAY     (SERIAL_MESSAGE_DELAY*15)   //Port not open or lost: open it again after 3 s
#define SERIAL_NO_REPLY_TIMEOUT (SERIAL_MESSAGE_DELAY*50)   //No valid reply for 10 s: stove not answering
#define SERIAL_PORT_DEFAULT     "/dev/ttyUSB0"
#define CLOCK_SET_HOLD_MS       (120*1000)                  //After a clock set the RAM clock lags: show the time written

/* Setpoint -/+ range, in half degrees (5.0 - 30.0 C), and power range */
#define SETPOINT_MIN_RAW        (5*2)
#define SETPOINT_MAX_RAW        (30*2)
#define POWER_MIN               (1)
#define POWER_MAX               (5)

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
#define powerInUseAddr          0x19

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

/* Clock to set (EEPROM, BCD, not polled): the RAM clock above is read only */
#define clockSet_DoWAddr            0xF8
#define clockSet_HourAddr           0xF9
#define clockSet_MinuteAddr         0xFA
#define clockSet_DayAddr            0xFB
#define clockSet_MonthAddr          0xFC
#define clockSet_YearAddr           0xFD
class SerialProto : public QObject
{
    Q_OBJECT

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

    /* Serial link as shown on the panel: port, stove answering */
    enum LinkState {
        LinkOk = 0,                         //Port open, valid replies (or nothing known yet)
        LinkPortClosed,                     //Port not open or lost, opened again every SERIAL_REOPEN_DELAY
        LinkNoReply                         //Port open, no valid reply for SERIAL_NO_REPLY_TIMEOUT
    };
    Q_ENUM(LinkState)
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
        powerInUseIndex,
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
        /*********/
        clockSet_DoWIndex,                  //Written only (writeClock), never polled
        clockSet_HourIndex,
        clockSet_MinuteIndex,
        clockSet_DayIndex,
        clockSet_MonthIndex,
        clockSet_YearIndex,

        LastIndex
    };
    Q_ENUM(RequestsIndex)

    const struct {
        uint8_t page;
        uint8_t address;
        char    description[50];
        uint8_t offset;
        float scale;
    } requestsMap[LastIndex] = {
        /* 00 */ramBank,    ambTempAddr,                "Room temperature",                   0,  2.0,
        /* 01 */ramBank,    cochleaTurnsAddr,           "Auger on time",                      0,  0.1,
        /* 02 */ramBank,    stoveStateAddr,             "Stove state",                        0,  1,
        /* 03 */ramBank,    flamePowerAddr,             "Flame power",                        0,  1,
        /* 04 */ramBank,    smokeFanSpeedAddr,          "Smoke fan speed",                  250,  10,
        /* 05 */ramBank,    smokeTempAddr,              "Smoke temperature",                  0,  1,
        /* 06 */ramBank,    powerInUseAddr,             "Power in use",                       0,  1,
        /* 07 */ramBank,    secondsCurrentAddr,         "Clock seconds",                      0,  1,
        /* 08 */ramBank,    dayOfWeekAddr,              "Clock day of week",                  0,  1,
        /* 09 */ramBank,    hoursCurrentAddr,           "Clock hours",                        0,  1,
        /* 10 */ramBank,    minutesCurrentAddr,         "Clock minutes",                      0,  1,
        /* 11 */ramBank,    dayOfMonthCurrentAddr,      "Clock day",                          0,  1,
        /* 12 */ramBank,    monthCurrentAddr,           "Clock month",                        0,  1,
        /* 13 */ramBank,    yearCurrentAddr,            "Clock year",                         0,  1,
        /*********/
        /* 14 */eepromBank, tempSetAddr,                "Setpoint",                           0,  2.0,
        /* 15 */eepromBank, powerSetAddr,               "Power",                              0,  1,
        /*********/
        /* 16 */eepromBank, chronoEnableAddr,           "Chrono on/off",                      0,  1,
        /*********/
        /* 17 */eepromBank, chronoWkE_EnableAddr,       "Weekend on/off",                     0,  1,
        /* 18 */eepromBank, chronoWkE_1_OnAddr,         "Weekend 1 start",                    0,  10,
        /* 19 */eepromBank, chronoWkE_1_OffAddr,        "Weekend 1 stop",                     0,  10,
        /* 20 */eepromBank, chronoWkE_2_OnAddr,         "Weekend 2 start",                    0,  10,
        /* 21 */eepromBank, chronoWkE_2_OffAddr,        "Weekend 2 stop",                     0,  10,
        /*********/
        /* 22 */eepromBank, chronoDay_EnableAddr,       "Daily on/off",                       0,  1,
        /* 23 */eepromBank, chronoDay_1_OnAddr,         "Daily 1 start",                      0,  10,
        /* 24 */eepromBank, chronoDay_1_OffAddr,        "Daily 1 stop",                       0,  10,
        /* 25 */eepromBank, chronoDay_2_OnAddr,         "Daily 2 start",                      0,  10,
        /* 26 */eepromBank, chronoDay_2_OffAddr,        "Daily 2 stop",                       0,  10,
        /*********/
        /* 27 */eepromBank, chronoSet_EnableAddr,       "Weekly on/off",                      0,  1,
        /*********/
        /* 28 */eepromBank, chronoSet_1_OnAddr,         "Weekly 1 start",                     0,  10,
        /* 29 */eepromBank, chronoSet_1_OffAddr,        "Weekly 1 stop",                      0,  10,
        /* 30 */eepromBank, chronoSet_1_LunEnabAddr,    "Weekly 1 Monday",                    0,  1,
        /* 31 */eepromBank, chronoSet_1_MarEnabAddr,    "Weekly 1 Tuesday",                   0,  1,
        /* 32 */eepromBank, chronoSet_1_MerEnabAddr,    "Weekly 1 Wednesday",                 0,  1,
        /* 33 */eepromBank, chronoSet_1_GioEnabAddr,    "Weekly 1 Thursday",                  0,  1,
        /* 34 */eepromBank, chronoSet_1_VenEnabAddr,    "Weekly 1 Friday",                    0,  1,
        /* 35 */eepromBank, chronoSet_1_SabEnabAddr,    "Weekly 1 Saturday",                  0,  1,
        /* 36 */eepromBank, chronoSet_1_DomEnabAddr,    "Weekly 1 Sunday",                    0,  1,
        /*********/
        /* 37 */eepromBank, chronoSet_2_OnAddr,         "Weekly 2 start",                     0,  10,
        /* 38 */eepromBank, chronoSet_2_OffAddr,        "Weekly 2 stop",                      0,  10,
        /* 39 */eepromBank, chronoSet_2_LunEnabAddr,    "Weekly 2 Monday",                    0,  1,
        /* 40 */eepromBank, chronoSet_2_MarEnabAddr,    "Weekly 2 Tuesday",                   0,  1,
        /* 41 */eepromBank, chronoSet_2_MerEnabAddr,    "Weekly 2 Wednesday",                 0,  1,
        /* 42 */eepromBank, chronoSet_2_GioEnabAddr,    "Weekly 2 Thursday",                  0,  1,
        /* 43 */eepromBank, chronoSet_2_VenEnabAddr,    "Weekly 2 Friday",                    0,  1,
        /* 44 */eepromBank, chronoSet_2_SabEnabAddr,    "Weekly 2 Saturday",                  0,  1,
        /* 45 */eepromBank, chronoSet_2_DomEnabAddr,    "Weekly 2 Sunday",                    0,  1,
        /*********/
        /* 46 */eepromBank, chronoSet_3_OnAddr,         "Weekly 3 start",                     0,  10,
        /* 47 */eepromBank, chronoSet_3_OffAddr,        "Weekly 3 stop",                      0,  10,
        /* 48 */eepromBank, chronoSet_3_LunEnabAddr,    "Weekly 3 Monday",                    0,  1,
        /* 49 */eepromBank, chronoSet_3_MarEnabAddr,    "Weekly 3 Tuesday",                   0,  1,
        /* 50 */eepromBank, chronoSet_3_MerEnabAddr,    "Weekly 3 Wednesday",                 0,  1,
        /* 51 */eepromBank, chronoSet_3_GioEnabAddr,    "Weekly 3 Thursday",                  0,  1,
        /* 52 */eepromBank, chronoSet_3_VenEnabAddr,    "Weekly 3 Friday",                    0,  1,
        /* 53 */eepromBank, chronoSet_3_SabEnabAddr,    "Weekly 3 Saturday",                  0,  1,
        /* 54 */eepromBank, chronoSet_3_DomEnabAddr,    "Weekly 3 Sunday",                    0,  1,
        /*********/
        /* 55 */eepromBank, chronoSet_4_OnAddr,         "Weekly 4 start",                     0,  10,
        /* 56 */eepromBank, chronoSet_4_OffAddr,        "Weekly 4 stop",                      0,  10,
        /* 57 */eepromBank, chronoSet_4_LunEnabAddr,    "Weekly 4 Monday",                    0,  1,
        /* 58 */eepromBank, chronoSet_4_MarEnabAddr,    "Weekly 4 Tuesday",                   0,  1,
        /* 59 */eepromBank, chronoSet_4_MerEnabAddr,    "Weekly 4 Wednesday",                 0,  1,
        /* 60 */eepromBank, chronoSet_4_GioEnabAddr,    "Weekly 4 Thursday",                  0,  1,
        /* 61 */eepromBank, chronoSet_4_VenEnabAddr,    "Weekly 4 Friday",                    0,  1,
        /* 62 */eepromBank, chronoSet_4_SabEnabAddr,    "Weekly 4 Saturday",                  0,  1,
        /* 63 */eepromBank, chronoSet_4_DomEnabAddr,    "Weekly 4 Sunday",                    0,  1,
        /*********/
        /* 64 */eepromBank, clockSet_DoWAddr,           "Clock set day of week",              0,  1,
        /* 65 */eepromBank, clockSet_HourAddr,          "Clock set hours",                    0,  1,
        /* 66 */eepromBank, clockSet_MinuteAddr,        "Clock set minutes",                  0,  1,
        /* 67 */eepromBank, clockSet_DayAddr,           "Clock set day",                      0,  1,
        /* 68 */eepromBank, clockSet_MonthAddr,         "Clock set month",                    0,  1,
        /* 69 */eepromBank, clockSet_YearAddr,          "Clock set year",                     0,  1,
        /*********/
    };


    static SerialProto *getInstance();
    void setSerPort(const QString &serPortName);
    QString serPort() const { return m_selSerial; }
    LinkState linkState() const { return m_linkState; }
    bool openSerPort();
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
    bool writeChrono(RequestsIndex index, quint8 value);
    bool writeClock(const QDateTime &dt);
    void setChronoSerialGet(bool newChronoSerialGet);

private:
    static SerialProto *instance;
    explicit SerialProto(QObject *parent = nullptr);

    QString m_selSerial;
    QSerialPort m_serial;
    QTimer m_loopTimer;
    QTimer m_rxIdleTimer;
    QTimer m_writeTimer;
    QTimer m_reopenTimer;
    QTimer m_noReplyTimer;
    bool m_loopEnabled;                     //Polling wanted: started when the port opens
    LinkState m_linkState;
    QByteArray m_rxBuffer;

    struct PendingWrite {
        RequestsIndex index;
        quint8 value;
    };
    QQueue<PendingWrite> m_writeQueue;
    PendingWrite m_writeCurrent;            //Write in flight, kept to verify the reply and retry
    QByteArray m_writeSent;                 //Its bytes, expected back as echo
    QByteArray m_readSent;                  //Last read request, expected back as echo
    quint8 m_writeAttempts;
    bool m_writeInFlight;
    bool m_readRetryPending;                //Reply not valid: read the same parameter again once
    bool m_readRetried;
    quint8 m_state, m_previousState, m_nextState;

    quint8 m_stoveState, m_flamePower, m_setPower;
    quint16 m_smokeFanRpm;
    quint8 m_setTempRaw;                    //Setpoint in half degrees, as on the stove
    bool m_setTempRead, m_setPowerRead;     //-/+ ignored until the value has been read
    quint32 m_txMessages, m_rxMessages, m_rxErrors, m_rxChecksumErrors, m_rxTimeouts;
    bool m_readPending;                     //Read sent, nothing received yet
    quint8 m_stoveYear, m_stoveMonth, m_stoveDay, m_stoveDoW,
        m_stoveHour, m_stoveMinutes, m_stoveSeconds;
    bool m_chronoSerialGet;
    QDateTime m_clockSet;                   //Time written by writeClock
    QElapsedTimer m_clockSetAge;

    void resetLoop();
    void checkReadTimeout();
    void setLinkState(LinkState linkState);
    void readStoveInfo(quint8, quint8);
    void storeValue(RequestsIndex index, quint8 val);
    void writeStoveCmd(RequestsIndex index, quint8 value);
    void sendData(QByteArray data);
    void sendWrite();
    bool writeConfirmed(const QByteArray &reply) const;
    quint8 stepBase(RequestsIndex index, quint8 read) const;
signals:
    void writeFailed(quint8 page, quint8 address, quint8 value);
    void linkStateChanged(quint8 linkState);
    void serialOpened();
    void serialClosed();
    void serialError(QString);
    void loopStarted();
    void loopStopped();
    void updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors, quint32 rxChecksumErrors, quint32 rxTimeouts);
    void updateAmbTemp(float ambTemp);
    void updateSetTemp(float setTemp);
    void updateStoveState(quint8 stoveState, QString stoveStateStr);
    void updatePower(quint8 setPower, quint8 flamePower);
    void updateSmoke(quint8 smokeTemp, quint16 smokeFanRpm);
    void updatePowerInUse(quint8 powerInUse);
    void updateChronoEnable(bool chronoEnable);
    void chronoRead(quint8 index, quint8 value);   //Every chrono parameter, index = RequestsIndex

    void updateStoveDateTime(QDateTime stoveDateTime);

private slots:
    void onReadyRead();
    void processStoveReply();
    void sendPendingWrite();
    void getStoveInfos();
    void bytesWritten(qint64 bytes);
    void onSerialError(QSerialPort::SerialPortError error);
};

#endif // SERIALPROTO_H
