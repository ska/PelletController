#include "serialproto.h"
#include <QThread>

SerialProto *SerialProto::instance = nullptr;

SerialProto::SerialProto(QObject *parent)
    : QObject{parent}
{
    m_selSerial = "";
    m_state = 0;
    m_stoveState = m_smokeTemp = m_flamePower = m_setPower = m_smokeFanSpeed = 0;
    m_ambTempDC = m_setTempDC = 0;
    m_txMessages = m_rxMessages = m_rxErrors = 0;
    m_ambTemp = m_setTemp = m_tonCochlea = 0.0;
    m_stoveYear = m_stoveMonth = m_stoveDay = m_stoveDoW = m_stoveHour = m_stoveMinutes = m_stoveSeconds = 0;

#if 0
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        qDebug() << "Name : " << info.portName();
        qDebug() << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();
#if 0
        serialPortStruct *tmpSerStruct = new serialPortStruct;
        tmpSerStruct->m_descr = info.portName() + " - " + info.description();
        tmpSerStruct->m_value = info.portName();
        m_serialPortStructs << tmpSerStruct;
#endif
    }
    //qDebug() << "m_serialPortStructs.length():" << m_serialPortStructs.length();
#endif
    connect(&m_serial, &QSerialPort::readyRead, this, &SerialProto::checkStoveReply);
    connect(&m_serial, &QSerialPort::bytesWritten, this, &SerialProto::bytesWritten);

    //connect(&m_loopTimer, SIGNAL(timeout()), this, SLOT(getStoveInfos()));
    connect(&m_loopTimer, SIGNAL(timeout()), this, SLOT(getStoveInfos()));
}

SerialProto *SerialProto::getInstance()
{
    if (!instance)
        instance = new SerialProto();

    qDebug() << "SerialProto adr  : " << &instance;
    return instance;
}

#if 0
QObject* SerialProto::createSingletonInstance(QQmlEngine *engine, QJSEngine *scriptEngine){
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    if(instance==nullptr)
    {
        instance = new SerialProto;
    }
    return instance;
}
QList<serialPortStruct *> SerialProto::serialPortStructs()
{
    return m_serialPortStructs;
}

#endif

void SerialProto::setSerPort(const QString &serPortName)
{
    m_selSerial = serPortName;
}

void SerialProto::openSerPort()
{
    qDebug() << Q_FUNC_INFO;
#if 0
    if( m_serialPortStructs.length() == 0)
        return;

    if(m_selSerial == "" && m_serialPortStructs.length()>0)
        m_selSerial = m_serialPortStructs.at(0)->m_value;
#else
    setSerPort("/dev/ttyUSB0");
#endif

    m_serial.setPortName(m_selSerial);
    m_serial.setBaudRate(QSerialPort::BaudRate::Baud1200);
    m_serial.setDataBits(QSerialPort::DataBits::Data8);
    m_serial.setParity(QSerialPort::Parity::NoParity);
    m_serial.setStopBits(QSerialPort::StopBits::TwoStop);
    m_serial.setFlowControl(QSerialPort::FlowControl::NoFlowControl);

    if (m_serial.open(QIODevice::ReadWrite))
    {
        qInfo() << "openSerPort() open " << m_selSerial << "OK";
        emit serialError("Serial " +m_selSerial+ " open OK!");
        emit serialOpened();
    } else {
        qWarning() << "openSerPort() ERROR: open " << m_selSerial << "KO!";
        emit serialError("Serial " +m_selSerial+ " open error!");
        m_serial.close();
    }
}

void SerialProto::closeSerPort()
{
    qDebug() << Q_FUNC_INFO;
    m_loopTimer.stop();
    if(m_serial.isOpen())
        m_serial.close();
    emit serialClosed();
}

void SerialProto::bytesWritten(qint64 bytes)
{
    qDebug() << "bytesWritten : " << bytes;
}

void SerialProto::sendData(QByteArray data)
{
    if(m_serial.isOpen())
    {
        m_serial.write(data);
        m_serial.waitForBytesWritten(50);
        m_txMessages++;
    }
}

void SerialProto::startSerLoop()
{
    m_loopTimer.start(500);
    m_txMessages = m_rxMessages = 0;
    emit loopStarted();
}

void SerialProto::stopSerLoop()
{
    m_loopTimer.stop();
    m_state = 0;
    emit loopStopped();
}

void SerialProto::checkStoveReply()
{
    QByteArray stoveRxData;
    while( m_serial.waitForReadyRead(100))
        stoveRxData.append(m_serial.readAll());

    quint8 dtLen = stoveRxData.length();

    if(dtLen == 4)
    {
        //Rimuovo i messavvi inviati ed in echo
        stoveRxData.remove(0,2);
        dtLen = 2;
    }

    if(dtLen==0)
    {
        return;
    }
    else if (dtLen == 2)
    {
        quint8 val = stoveRxData[1];
        quint8 checksum = stoveRxData[0];
        quint8 param = checksum - val - requestsMap[m_previousState].page;

        if(param != requestsMap[m_previousState].address)
        {
            qWarning() << "Invalid response or checksum";
            qDebug() << QString("Resp: Chk: %1 - val: %2 - param: %3").arg(checksum, 0, 16)
                            .arg(val, 0, 16)
                            .arg(param, 0, 16);
            return;
        }

        m_rxMessages++;
        switch (param)
        {
            case ambTempAddr:
                m_ambTemp = (float)val / 2;
                m_ambTempDC = (((quint16)val)*10)/2;
                emit updateAmbTemp(m_ambTemp);
                break;

            case cochleaTurnsAddr:
                m_tonCochlea = (float)val / 10;
                qDebug() << "Resp: cochleaTurnsAddr: " << m_tonCochlea;
                break;

            case stoveStateAddr:
                m_stoveState = val;
                qDebug() << "Resp: stoveStateAddr: " << m_stoveState;
                break;

            case flamePowerAddr:
                if (m_stoveState < 6)
                {
                    //From 0-16 to 10-100%
                    if (m_stoveState > 0)
                        m_flamePower = (((quint16)val*90)/16)+10;
                }
                else
                {
                    m_flamePower = 0;
                }
                qDebug() << "Resp: flamePowerAddr: " << m_flamePower;
                break;

            case smokeFanSpeedAddr:
                m_smokeFanSpeed = (val*10)+250;
                qDebug() << "Resp: smokeFanSpeedAddr: " << m_smokeFanSpeed;
                break;

            case smokeTempAddr:
                //corretta
                m_smokeTemp = val;
                qDebug() << "Resp: smokeTempAddr: " << m_smokeTemp;
                break;

            case secondsCurrentAddr:
                m_stoveSeconds = val;
                qDebug() << "Resp: secondsCurrentAddr: " << m_stoveSeconds;
                break;
            case dayOfWeekAddr:
                m_stoveDoW = val;
                break;
            case hoursCurrentAddr:
                m_stoveHour = val;
                break;
            case minutesCurrentAddr:
                m_stoveMinutes = val;
                break;
            case dayOfMonthCurrentAddr:
                m_stoveDay = val;
                break;
            case monthCurrentAddr:
                m_stoveMonth = val;
                break;
            case yearCurrentAddr:
                m_stoveYear = val;
                break;
            case tempSetAddr:
                m_setTemp = (float)val / 2;
                m_setTempDC = (((quint16)val)*10)/2;
                //qDebug() << Q_FUNC_INFO << "tempSetAddr: " << m_setTemp;
                emit updateSetTemp(m_setTemp);
                break;
            case powerSetAddr:
                m_setPower = val;
                break;

            default:
                qDebug() << QString("Resp: Chk: %1 - val: %2 - param: %3").arg(checksum, 0, 16)
                                .arg(val, 0, 16)
                                .arg(param, 0, 16);
                //se il messaggio non e' parsato lo rollo indietro
                m_rxMessages--;
                break;
        }
    } else {
        qWarning() << "Received: " << dtLen << "bytes";
        m_rxErrors++;
    }
}

void SerialProto::getStoveInfos()
{
    if(0==m_state)
    {
        m_loopTimer.stop();
        m_loopTimer.start(500);
    }

    m_previousState   = m_state;
    /*Ultimo address nella mappa mi serve per attendere arrivo ultimi dati prima di emit*/
    if(m_state < requestsMapSize)
        readStoveInfo( requestsMap[m_state].page,  requestsMap[m_state].address );
    m_state++;

    if(m_state >= requestsMapSize)
    {
        m_state = 0;
        m_loopTimer.stop();

        /*
         * */
        emit updateStoveDateTime(QDateTime( QDate(2000+m_stoveYear, m_stoveMonth, m_stoveDay), QTime(m_stoveHour, m_stoveMinutes, m_stoveSeconds) ));
        m_loopTimer.start(5000);
    }

    //emit updateStats(m_txMessages, m_rxMessages);
    //emit updateStoveState(m_stoveState, m_stoveStateStr.at(m_stoveState));
    //emit updateAmbTemp(m_ambTempDC, m_setTempDC);
    //emit updatePower(m_setPower, m_flamePower);
    //emit updateSmoke(m_smokeTemp, m_smokeFanSpeed);
}

void SerialProto::readStoveInfo(quint8 page, quint8 address)
{
    QByteArray data;
    data.clear();
    data.append((quint8)readCmd + (quint8)page);
    data.append((quint8)address);
    sendData(data);
}
