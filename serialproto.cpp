#include "serialproto.h"
#include <QThread>

SerialProto *SerialProto::instance = nullptr;

SerialProto::SerialProto(QObject *parent)
    : QObject{parent}
{
    m_selSerial = "";
    m_state = m_previousState = m_nextState = 0;
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
    bytes = bytes;
    //qDebug() << "bytesWritten : " << bytes;
}

void SerialProto::sendData(QByteArray data)
{
    if(m_serial.isOpen())
    {
        m_serial.write(data);
        m_serial.waitForBytesWritten(50);
        m_txMessages++;
    } else {
        for(int i=0; i<data.length(); i++)
            qDebug() << "sendData: " << QString("%1").arg((quint8)data.at(i) , 0, 16);
    }
}

void SerialProto::startSerLoop()
{
    m_loopTimer.start(200);
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

    //for(int i=0; i<stoveRxData.length(); i++)
    //    qDebug() << "recvData: " << QString("%1").arg((quint8)stoveRxData.at(i) , 0, 16);

    if(dtLen == 6)
    {
        //Risposta a scrittura
        //Rimuovo i messavvi inviati ed in echo
        stoveRxData.remove(0,4);
        stoveRxData[0] = (quint8)(stoveRxData[0] - writeCmd); //Tolgo 0x80
        dtLen = 2;
        //riavvio poll lettura info
        m_loopTimer.start(1000);
        m_state = 0;
    }
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
        quint8 bank = requestsMap[m_previousState].page;
        if(param != requestsMap[m_previousState].address)
        {
            qWarning() << "Invalid response or checksum";
            qDebug() << QString("Resp: Chk: %1 - val: %2 - param: %3").arg(checksum, 0, 16)
                            .arg(val, 0, 16)
                            .arg(param, 0, 16);
            return;
        }

        m_rxMessages++;

        quint16 bank_param = (((quint16)bank << 8) | param);
        switch (bank_param)
        {
            case ramParam(ambTempAddr ):
                m_ambTemp = (float)val / 2;
                m_ambTempDC = (((quint16)val)*10)/2;
                emit updateAmbTemp(m_ambTemp);
                break;

            case ramParam(cochleaTurnsAddr ):
                m_tonCochlea = (float)val / 10;
                qDebug() << "Resp: cochleaTurnsAddr: " << m_tonCochlea;
                break;

            case ramParam(stoveStateAddr ):
                m_stoveState = val;
                emit updateStoveState(m_stoveState, m_stoveStateStr.at(m_stoveState));
                break;

            case ramParam(flamePowerAddr ):
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

            case ramParam(smokeFanSpeedAddr ):
                m_smokeFanSpeed = (val*10)+250;
                qDebug() << "Resp: smokeFanSpeedAddr: " << m_smokeFanSpeed;
                break;

            case ramParam(smokeTempAddr ):
                //corretta
                m_smokeTemp = val;
                qDebug() << "Resp: smokeTempAddr: " << m_smokeTemp;
                break;

            case ramParam(secondsCurrentAddr ):
                m_stoveSeconds = val;
                break;
            case ramParam(dayOfWeekAddr ):
                m_stoveDoW = val;
                break;
            case ramParam(hoursCurrentAddr ):
                m_stoveHour = val;
                break;
            case ramParam(minutesCurrentAddr ):
                m_stoveMinutes = val;
                break;
            case ramParam(dayOfMonthCurrentAddr ):
                m_stoveDay = val;
                break;
            case ramParam(monthCurrentAddr ):
                m_stoveMonth = val;
                break;
            case ramParam(yearCurrentAddr ):
                m_stoveYear = val;
                break;
            case eepromParam(tempSetAddr ):
                m_setTemp = (float)val / 2;
                m_setTempDC = (((quint16)val)*10)/2;
                emit updateSetTemp(m_setTemp);
                break;
            case eepromParam(powerSetAddr ):
                m_setPower = val;
                qDebug() << "Resp: powerSetAddr: " << m_setPower;
                emit updatePower(m_setPower, m_flamePower);
                break;
                /*********/
            case eepromParam(chronoEnableAddr ):
                m_chronoEnable = (bool)val;
                qDebug() << "Resp: chronoEnableAddr: " << m_chronoEnable;
                emit updateChronoEnable(m_chronoEnable);
                break;



                /*********/
            case eepromParam( chronoDay_EnableAddr ):
                qDebug() << "Resp: chronoDay_EnableAddr: " << val;
                break;
            case eepromParam( chronoDay_1_OnAddr ):
                qDebug() << "Resp: chronoDay_1_OnAddr: " << val;
                break;
            case eepromParam( chronoDay_1_OffAddr ):
                qDebug() << "Resp: chronoDay_1_OffAddr: " << val;
                break;
            case eepromParam( chronoDay_2_OnAddr ):
                qDebug() << "Resp: chronoDay_2_OnAddr: " << val;
                break;
            case eepromParam( chronoDay_2_OffAddr ):
                qDebug() << "Resp: chronoDay_2_OffAddr: " << val;
                break;


            case eepromParam(chronoSet_EnableAddr ):
                qDebug() << "Resp: chronoSet_EnableAddr: " << val;
                break;
            case eepromParam(chronoSet_1_OnAddr ):
                qDebug() << "Resp: chronoSet_1_OnAddr: " << val;
                break;
            case eepromParam( chronoSet_1_OffAddr ):
                qDebug() << "Resp: chronoSet_1_OffAddr: " << val;
                break;
            case eepromParam( chronoSet_1_LunEnabAddr ):
                qDebug() << "Resp: chronoSet_1_LunEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_MarEnabAddr ):
                qDebug() << "Resp: chronoSet_1_MarEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_MerEnabAddr ):
                qDebug() << "Resp: chronoSet_1_MerEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_GioEnabAddr ):
                qDebug() << "Resp: chronoSet_1_GioEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_VenEnabAddr ):
                qDebug() << "Resp: chronoSet_1_VenEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_SabEnabAddr ):
                qDebug() << "Resp: chronoSet_1_SabEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_1_DomEnabAddr ):
                qDebug() << "Resp: chronoSet_1_DomEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_OnAddr ):
                qDebug() << "Resp: chronoSet_2_OnAddr: " << val;
                break;
            case eepromParam( chronoSet_2_OffAddr ):
                qDebug() << "Resp: chronoSet_2_OffAddr: " << val;
                break;
            case eepromParam( chronoSet_2_LunEnabAddr ):
                qDebug() << "Resp: chronoSet_2_LunEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_MarEnabAddr ):
                qDebug() << "Resp: chronoSet_2_MarEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_MerEnabAddr ):
                qDebug() << "Resp: chronoSet_2_MerEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_GioEnabAddr ):
                qDebug() << "Resp: chronoSet_2_GioEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_VenEnabAddr ):
                qDebug() << "Resp: chronoSet_2_VenEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_SabEnabAddr ):
                qDebug() << "Resp: chronoSet_2_SabEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_2_DomEnabAddr ):
                qDebug() << "Resp: chronoSet_2_DomEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_OnAddr ):
                qDebug() << "Resp: chronoSet_3_OnAddr: " << val;
                break;
            case eepromParam( chronoSet_3_OffAddr ):
                qDebug() << "Resp: chronoSet_3_OffAddr: " << val;
                break;
            case eepromParam( chronoSet_3_LunEnabAddr ):
                qDebug() << "Resp: chronoSet_3_LunEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_MarEnabAddr ):
                qDebug() << "Resp: chronoSet_3_MarEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_MerEnabAddr ):
                qDebug() << "Resp: chronoSet_3_MerEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_GioEnabAddr ):
                qDebug() << "Resp: chronoSet_3_GioEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_VenEnabAddr ):
                qDebug() << "Resp: chronoSet_3_VenEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_SabEnabAddr ):
                qDebug() << "Resp: chronoSet_3_SabEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_3_DomEnabAddr ):
                qDebug() << "Resp: chronoSet_3_DomEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_OnAddr ):
                qDebug() << "Resp: chronoSet_4_OnAddr: " << val;
                break;
            case eepromParam( chronoSet_4_OffAddr ):
                qDebug() << "Resp: chronoSet_4_OffAddr: " << val;
                break;
            case eepromParam( chronoSet_4_LunEnabAddr ):
                qDebug() << "Resp: chronoSet_4_LunEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_MarEnabAddr ):
                qDebug() << "Resp: chronoSet_4_MarEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_MerEnabAddr ):
                qDebug() << "Resp: chronoSet_4_MerEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_GioEnabAddr ):
                qDebug() << "Resp: chronoSet_4_GioEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_VenEnabAddr ):
                qDebug() << "Resp: chronoSet_4_VenEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_SabEnabAddr ):
                qDebug() << "Resp: chronoSet_4_SabEnabAddr: " << val;
                break;
            case eepromParam( chronoSet_4_DomEnabAddr ):
                qDebug() << "Resp: chronoSet_4_DomEnabAddr: " << val;
                break;
            case eepromParam( chronoWkE_EnableAddr ):
                qDebug() << "Resp: chronoWkE_EnableAddr: " << val;
                break;
            case eepromParam( chronoWkE_1_OnAddr ):
                qDebug() << "Resp: chronoWkE_1_OnAddr: " << val;
                break;
            case eepromParam( chronoWkE_1_OffAddr ):
                qDebug() << "Resp: chronoWkE_1_OffAddr: " << val;
                break;
            case eepromParam( chronoWkE_2_OnAddr ):
                qDebug() << "Resp: chronoWkE_2_OnAddr: " << val;
                break;
            case eepromParam( chronoWkE_2_OffAddr ):
                qDebug() << "Resp: chronoWkE_2_OffAddr: " << val;
                break;

            default:
                qWarning() << "Unknow message";
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
    emit updateStats(m_txMessages, m_rxMessages, m_rxErrors);
}

void SerialProto::getStoveInfos()
{
    if(0==m_state)
    {
        m_loopTimer.stop();
        m_loopTimer.start(200);
    }

    m_nextState       = m_previousState+1;
    m_previousState   = m_state;
    /*Ultimo address nella mappa mi serve per attendere arrivo ultimi dati prima di emit*/
    if(m_state < requestsMapSize)
        readStoveInfo( requestsMap[m_state].page,  requestsMap[m_state].address );

    /*Un giro si e uno no controllo lo stato */
    if(m_state == stoveStateIndex)
        m_state = m_nextState;
    else
        m_state = stoveStateIndex;

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

void SerialProto::writeStoveCmd(quint8 page, quint8 address, quint8 value)
{
    m_loopTimer.stop();
    QThread::msleep(150);//Aspetto eventuali messaggi incoda.
    QByteArray data;
    data.clear();
    data.append((quint8)writeCmd + (quint8)page);
    data.append((quint8)address);
    data.append((quint8)value);
    data.append((quint8)(writeCmd+page+address+value));
    sendData(data);
}

void SerialProto::writeStoveStateOn()
{
    m_previousState = stoveStateIndex;
    writeStoveCmd(requestsMap[stoveStateIndex].page,
                  requestsMap[stoveStateIndex].address,
                  StoveState::Starting  );
}

void SerialProto::writeStoveStateOff()
{
    m_previousState = stoveStateIndex;
    writeStoveCmd(requestsMap[stoveStateIndex].page,
                  requestsMap[stoveStateIndex].address,
                  StoveState::FinalCleaning  );
}

void SerialProto::writeStoveStateOffForce()
{
    m_previousState = stoveStateIndex;
    writeStoveCmd(requestsMap[stoveStateIndex].page,
                  requestsMap[stoveStateIndex].address,
                  StoveState::Off  );
}

void SerialProto::writeStoveDecPower()
{
    if(m_setPower<=1)
        return;
    m_previousState = powerSetIndex;
    writeStoveCmd(requestsMap[powerSetIndex].page,
                  requestsMap[powerSetIndex].address,
                  m_setPower-1  );
}

void SerialProto::writeStoveIncPower()
{
    if(m_setPower>=5)
        return;
    m_previousState = powerSetIndex;
    writeStoveCmd(requestsMap[powerSetIndex].page,
                  requestsMap[powerSetIndex].address,
                  m_setPower+1  );
}

void SerialProto::writeStoveDecSetPoint()
{
    m_previousState = tempSetIndex;
    writeStoveCmd(requestsMap[tempSetIndex].page,
                  requestsMap[tempSetIndex].address,
                  ((m_setTemp*2)-1)  );
}

void SerialProto::writeStoveIncSetPoint()
{
    m_previousState = tempSetIndex;
    writeStoveCmd(requestsMap[tempSetIndex].page,
                  requestsMap[tempSetIndex].address,
                  ((m_setTemp*2)+1)  );
}
