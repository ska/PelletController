#include "serialproto.h"

SerialProto *SerialProto::instance = nullptr;

SerialProto::SerialProto(QObject *parent)
    : QObject{parent}
{
    m_selSerial = SERIAL_PORT_DEFAULT;
    m_loopEnabled = false;
    m_linkState = LinkOk;
    m_state = m_previousState = m_nextState = 0;
    m_stoveState = m_smokeTemp = m_flamePower = m_setPower = m_smokeFanSpeed = 0;
    m_ambTempDC = m_setTempDC = 0;
    m_setTempRaw = 0;
    m_setTempRead = m_setPowerRead = false;
    m_txMessages = m_rxMessages = m_rxErrors = m_rxChecksumErrors = m_rxTimeouts = 0;
    m_readPending = false;
    m_writeAttempts = 0;
    m_writeCurrent = {};
    m_readRetryPending = m_readRetried = false;
    m_ambTemp = m_setTemp = m_tonCochlea = 0.0;
    m_stoveYear = m_stoveMonth = m_stoveDay = m_stoveDoW = m_stoveHour = m_stoveMinutes = m_stoveSeconds = 0;
    m_chronoSerialGet = false;
    m_writeInFlight = false;
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
    m_rxIdleTimer.setSingleShot(true);
    m_writeTimer.setSingleShot(true);
    m_reopenTimer.setSingleShot(true);
    m_noReplyTimer.setSingleShot(true);

    connect(&m_serial, &QSerialPort::readyRead, this, &SerialProto::onReadyRead);
    connect(&m_serial, &QSerialPort::bytesWritten, this, &SerialProto::bytesWritten);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &SerialProto::onSerialError);
    connect(&m_reopenTimer, &QTimer::timeout, this, &SerialProto::openSerPort);
    connect(&m_noReplyTimer, &QTimer::timeout, this, [this]{ setLinkState(LinkNoReply); });
    connect(&m_loopTimer, SIGNAL(timeout()), this, SLOT(getStoveInfos()));
    connect(&m_rxIdleTimer, &QTimer::timeout, this, &SerialProto::processStoveReply);
    connect(&m_writeTimer, &QTimer::timeout, this, &SerialProto::sendPendingWrite);
}

void SerialProto::setChronoSerialGet(bool newChronoSerialGet)
{
    m_chronoSerialGet = newChronoSerialGet;
    m_state = 0;
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

/* Opens the port; if it fails, tries again every SERIAL_REOPEN_DELAY until
 * closeSerPort (issue 10). The polling starts here when already wanted. */
bool SerialProto::openSerPort()
{
    m_reopenTimer.stop();
    if(m_serial.isOpen())
        return true;

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
        if(m_loopEnabled)
        {
            m_state = 0;
            m_loopTimer.start(SERIAL_MESSAGE_DELAY);
            m_noReplyTimer.start(SERIAL_NO_REPLY_TIMEOUT);
        }
        return true;
    }

    //Logged once, not at every attempt
    if(m_linkState != LinkPortClosed)
        qWarning() << "openSerPort() ERROR: open " << m_selSerial << "KO:" << m_serial.errorString();
    emit serialError("Serial " +m_selSerial+ " open error!");
    m_serial.close();
    setLinkState(LinkPortClosed);
    m_reopenTimer.start(SERIAL_REOPEN_DELAY);
    return false;
}

void SerialProto::closeSerPort()
{
    qDebug() << Q_FUNC_INFO;
    stopSerLoop();
    m_reopenTimer.stop();
    if(m_serial.isOpen())
        m_serial.close();
    emit serialClosed();
}

/* Port lost (USB adapter unplugged): closed out of the error signal, then
 * opened again by the reopen timer, the polling goes on by itself */
void SerialProto::onSerialError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::ResourceError && error != QSerialPort::ReadError
       && error != QSerialPort::WriteError && error != QSerialPort::UnknownError)
        return;

    qWarning() << "Serial error" << error << m_serial.errorString();
    QTimer::singleShot(0, this, [this]{
        if(!m_serial.isOpen())
            return;
        resetLoop();
        m_serial.close();
        //Values read again before -/+ works (the stove may have changed meanwhile)
        m_setTempRead = m_setPowerRead = false;
        emit serialClosed();
        setLinkState(LinkPortClosed);
        m_reopenTimer.start(SERIAL_REOPEN_DELAY);
    });
}

void SerialProto::setLinkState(LinkState linkState)
{
    if(linkState == m_linkState)
        return;
    m_linkState = linkState;
    if(LinkOk == linkState)
        qInfo() << "Stove link OK";
    else
        qWarning() << "Stove link:" << (LinkPortClosed == linkState ? "port closed" : "no reply");
    emit linkStateChanged(linkState);
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
        m_txMessages++;
    } else {
        //for(int i=0; i<data.length(); i++)
        //    qDebug() << "sendData: " << QString("%1").arg((quint8)data.at(i) , 0, 16);
    }
}

/* Polling wanted: runs while the port is open, restarted when it opens again */
void SerialProto::startSerLoop()
{
    m_loopEnabled = true;
    m_txMessages = m_rxMessages = 0;
    if(m_serial.isOpen())
    {
        m_loopTimer.start(SERIAL_MESSAGE_DELAY);
        m_noReplyTimer.start(SERIAL_NO_REPLY_TIMEOUT);
    }
    emit loopStarted();
}

void SerialProto::stopSerLoop()
{
    m_loopEnabled = false;
    resetLoop();
    emit loopStopped();
}

void SerialProto::resetLoop()
{
    m_loopTimer.stop();
    m_noReplyTimer.stop();
    m_rxIdleTimer.stop();
    m_writeTimer.stop();
    m_rxBuffer.clear();
    m_writeQueue.clear();
    m_writeInFlight = false;
    m_writeAttempts = 0;
    m_readRetryPending = m_readRetried = false;
    m_readPending = false;
    m_state = 0;
}

void SerialProto::onReadyRead()
{
    m_rxBuffer.append(m_serial.readAll());
    //La risposta e' completa quando la linea resta muta per SERIAL_RX_IDLE_TIMEOUT
    m_rxIdleTimer.start(SERIAL_RX_IDLE_TIMEOUT);
}

void SerialProto::processStoveReply()
{
    QByteArray stoveRxData = m_rxBuffer;
    m_rxBuffer.clear();
    m_readPending = false;
    bool valid = false;

    if(m_writeInFlight)
    {
        //Scrittura conclusa (risposta o timeout): verifica, poi prossima scrittura o riavvio poll lettura info
        m_writeInFlight = false;
        const bool confirmed = writeConfirmed(stoveRxData);
        if(!confirmed)
        {
            if(m_writeAttempts < SERIAL_WRITE_ATTEMPTS)
            {
                qWarning() << "Write not confirmed, sending it again:" << stoveRxData.toHex(' ');
                m_writeTimer.start(SERIAL_WRITE_DELAY);
                return;
            }
            qWarning() << "Write failed:" << m_writeSent.toHex(' ') << "reply" << stoveRxData.toHex(' ');
            emit writeFailed(requestsMap[m_writeCurrent.index].page,
                             requestsMap[m_writeCurrent.index].address,
                             m_writeCurrent.value);
        }
        m_writeAttempts = 0;
        if(!m_writeQueue.isEmpty())
        {
            m_writeTimer.start(SERIAL_WRITE_DELAY);
        } else {
            m_state = 0;
            m_loopTimer.start(SERIAL_MESSAGE_DELAY*3);
        }
        if(!confirmed)
            return;

        //Risposta a scrittura confermata: tolgo l'eco dei 4 byte inviati e 0x80
        stoveRxData.remove(0,4);
        stoveRxData[0] = (quint8)(stoveRxData[0] - writeCmd);
        valid = true;
    }
    else if(stoveRxData.length() == 4 && stoveRxData.startsWith(m_readSent))
    {
        //Risposta a lettura: tolgo l'eco della richiesta, controllata (issue 8)
        stoveRxData.remove(0,2);
        valid = true;
    }

    //for(int i=0; i<stoveRxData.length(); i++)
    //    qDebug() << "recvData: " << QString("%1").arg((quint8)stoveRxData.at(i) , 0, 16);

    if(stoveRxData.isEmpty())
    {
        return;
    }
    else if (valid)
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
            m_rxChecksumErrors++;
            m_readRetryPending = !m_readRetried;
            emit updateStats(m_txMessages, m_rxMessages, m_rxErrors, m_rxChecksumErrors, m_rxTimeouts);
            return;
        }

        m_rxMessages++;
        m_noReplyTimer.start(SERIAL_NO_REPLY_TIMEOUT);
        setLinkState(LinkOk);

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
                //qDebug() << "Resp: cochleaTurnsAddr: " << m_tonCochlea;
                break;

            case ramParam(stoveStateAddr ):
                if(val != m_stoveState)
                    qInfo() << "Stove state:" << m_stoveState << "->" << val;
                m_stoveState = val;
                //Stato fuori tabella (scheda stufa piu' recente o risposta rumorosa): niente at() fuori range
                emit updateStoveState(m_stoveState, (m_stoveState < m_stoveStateStr.size())
                                                        ? m_stoveStateStr.at(m_stoveState)
                                                        : QString("Unknown (%1)").arg(m_stoveState));
                break;

            case ramParam(flamePowerAddr ):
                //From 0-16 to 10-100%, 0 when Off or from Final Cleaning on (issue 11)
                if (m_stoveState > Off && m_stoveState < FinalCleaning)
                    m_flamePower = (((quint16)val*90)/16)+10;
                else
                    m_flamePower = 0;
                //qDebug() << "Resp: flamePowerAddr: " << m_flamePower;
                break;

            case ramParam(smokeFanSpeedAddr ):
                m_smokeFanSpeed = (val*10)+250;
                //qDebug() << "Resp: smokeFanSpeedAddr: " << m_smokeFanSpeed;
                break;

            case ramParam(smokeTempAddr ):
                //corretta
                m_smokeTemp = val;
                //qDebug() << "Resp: smokeTempAddr: " << m_smokeTemp;
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
                m_setTempRaw = val;
                m_setTempRead = true;
                emit updateSetTemp(m_setTemp);
                break;
            case eepromParam(powerSetAddr ):
                m_setPower = val;
                m_setPowerRead = true;
                //qDebug() << "Resp: powerSetAddr: " << m_setPower;
                emit updatePower(m_setPower, m_flamePower);
                break;
                /*********/
            case eepromParam(chronoEnableAddr ):
                //qDebug() << "Resp: chronoEnableAddr: " << val;
                emit updateChronoEnable((bool)val);
                break;
                /*********/
            case eepromParam( chronoWkE_EnableAddr ):
                emit updateChronoWkEEnable((bool)val);
                break;
            case eepromParam( chronoWkE_1_OnAddr ):
                emit updateChronoWkE1On((quint8)val);
                break;
            case eepromParam( chronoWkE_1_OffAddr ):
                emit updateChronoWkE1Off((quint8)val);
                break;
            case eepromParam( chronoWkE_2_OnAddr ):
                emit updateChronoWkE2On((quint8)val);
                break;
            case eepromParam( chronoWkE_2_OffAddr ):
                emit updateChronoWkE2Off((quint8)val);
                break;
#if 0
                /*********/
            case eepromParam( chronoDay_EnableAddr ):
                qDebug() << "Resp: chronoDay_EnableAddr: " << val;
                emit updateChronoDayEnable((bool)val);
                break;
            case eepromParam( chronoDay_1_OnAddr ):
                qDebug() << "Resp: chronoDay_1_OnAddr: " << val;
                emit updateChronoDay1On((quint8)val);
                break;
            case eepromParam( chronoDay_1_OffAddr ):
                qDebug() << "Resp: chronoDay_1_OffAddr: " << val;
                emit updateChronoDay1Off((quint8)val);
                break;
            case eepromParam( chronoDay_2_OnAddr ):
                qDebug() << "Resp: chronoDay_2_OnAddr: " << val;
                emit updateChronoDay2On((quint8)val);
                break;
            case eepromParam( chronoDay_2_OffAddr ):
                qDebug() << "Resp: chronoDay_2_OffAddr: " << val;
                emit updateChronoDay2Off((quint8)val);
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
#endif

            default:
                //Valid reply to a parameter not used by the panel: still counted in Rx (issue 27)
                break;
        }
    } else {
        //Lunghezza sbagliata o eco diversa dalla richiesta (solo eco = stufa muta)
        qWarning() << "Invalid reply:" << stoveRxData.toHex(' ') << "to" << m_readSent.toHex(' ');
        m_rxErrors++;
        m_readRetryPending = !m_readRetried;
    }
    emit updateStats(m_txMessages, m_rxMessages, m_rxErrors, m_rxChecksumErrors, m_rxTimeouts);
}

void SerialProto::getStoveInfos()
{

    /*Risposta in ricezione o scrittura in corso: riprovo al prossimo tick*/
    if(m_rxIdleTimer.isActive() || m_writeInFlight || m_writeTimer.isActive())
        return;

    checkReadTimeout();

    /*Risposta non valida o assente: rileggo subito lo stesso parametro, una volta sola, senza avanzare*/
    if(m_readRetryPending)
    {
        m_readRetryPending = false;
        m_readRetried = true;
        readStoveInfo(requestsMap[m_previousState].page, requestsMap[m_previousState].address);
        return;
    }
    m_readRetried = false;

    /*Ultimo address nella mappa mi serve per attendere arrivo ultimi dati prima di emit*/
    if(false == m_chronoSerialGet)
    {
        if(0==m_state)
        {
            m_loopTimer.stop();
            m_loopTimer.start(SERIAL_MESSAGE_DELAY);
        }

        m_nextState       = m_previousState+1;
        m_previousState   = m_state;
        if(m_state < requestsMapSize)
            readStoveInfo( requestsMap[m_state].page,  requestsMap[m_state].address );

        /*Un giro si e uno no controllo lo stato (saltandolo nella sequenza dei parametri)*/
        if(m_state == stoveStateIndex)
            m_state = (m_nextState == stoveStateIndex) ? m_nextState+1 : m_nextState;
        else
            m_state = stoveStateIndex;

        if(m_state > chronoEnableIndex)
        {
            m_state = 0;
            m_loopTimer.stop();

            /*
             * */
            emit updateStoveDateTime(QDateTime( QDate(2000+m_stoveYear, m_stoveMonth, m_stoveDay), QTime(m_stoveHour, m_stoveMinutes, m_stoveSeconds) ));
            m_loopTimer.start(SERIAL_SESSION_DELAY);
        }
    } else {
        /* Leggo solo parte di chrono */
        if(m_state < chronoEnableIndex)
        {
            m_state = chronoEnableIndex;
            m_previousState   = m_state;
            m_nextState       = m_previousState+1;
            m_loopTimer.stop();
            m_loopTimer.start(SERIAL_MESSAGE_DELAY);
        }

        if(m_state < requestsMapSize)
            readStoveInfo( requestsMap[m_state].page,  requestsMap[m_state].address );

        //qDebug() << "***********: " << m_state << "--- " << requestsMap[m_state].address;

        m_previousState   = m_state;
        m_nextState       = m_state+1;
        m_state = m_nextState;

        if(m_state >= LastIndex)
        {
            m_state = 0;
            m_loopTimer.stop();
            m_loopTimer.start(SERIAL_SESSION_DELAY);
        }
    }
}

void SerialProto::readStoveInfo(quint8 page, quint8 address)
{
    m_readSent.clear();
    m_readSent.append((quint8)readCmd + (quint8)page);
    m_readSent.append((quint8)address);
    sendData(m_readSent);
    m_readPending = true;
}

/* The last read got no byte at all (not even the echo) before the next
 * request: counted apart from the bad replies and read again once */
void SerialProto::checkReadTimeout()
{
    if(!m_readPending)
        return;
    m_readPending = false;
    m_rxTimeouts++;
    m_readRetryPending = !m_readRetried;
    qWarning() << "No reply to" << m_readSent.toHex(' ');
    emit updateStats(m_txMessages, m_rxMessages, m_rxErrors, m_rxChecksumErrors, m_rxTimeouts);
}

void SerialProto::writeStoveCmd(RequestsIndex index, quint8 value)
{
    if(!m_serial.isOpen())
    {
        qWarning() << "Write dropped, serial port not open";
        emit writeFailed(requestsMap[index].page, requestsMap[index].address, value);
        return;
    }
    m_loopTimer.stop();
    m_writeQueue.enqueue({index, value});
    //Aspetto eventuali messaggi in coda prima di scrivere
    if(!m_writeInFlight && !m_writeTimer.isActive())
        m_writeTimer.start(SERIAL_WRITE_DELAY);
}

void SerialProto::sendPendingWrite()
{
    if(0 == m_writeAttempts && m_writeQueue.isEmpty())
        return;

    //Risposta di lettura ancora in arrivo: riprovo dopo
    if(m_rxIdleTimer.isActive())
    {
        m_writeTimer.start(SERIAL_WRITE_DELAY);
        return;
    }

    checkReadTimeout();

    //Retry: same write again; otherwise the next one in the queue
    if(0 == m_writeAttempts)
        m_writeCurrent = m_writeQueue.dequeue();
    sendWrite();
}

void SerialProto::sendWrite()
{
    const quint8 page    = requestsMap[m_writeCurrent.index].page;
    const quint8 address = requestsMap[m_writeCurrent.index].address;
    const quint8 value   = m_writeCurrent.value;

    m_writeSent.clear();
    m_writeSent.append((quint8)writeCmd + (quint8)page);
    m_writeSent.append((quint8)address);
    m_writeSent.append((quint8)value);
    m_writeSent.append((quint8)(writeCmd+page+address+value));

    m_rxBuffer.clear();
    m_readRetryPending = false;               //The polling restarts after the write
    m_previousState = m_writeCurrent.index;
    m_writeInFlight = true;
    m_writeAttempts++;
    sendData(m_writeSent);
    //Se la stufa non risponde processStoveReply chiude comunque la scrittura
    m_rxIdleTimer.start(SERIAL_WRITE_TIMEOUT);
}

/* Write reply: the echo of the 4 bytes sent, then 0x80 + bank + address +
 * value and the value written (issue 9) */
bool SerialProto::writeConfirmed(const QByteArray &reply) const
{
    if(reply.length() != 6 || !reply.startsWith(m_writeSent))
        return false;
    return (quint8)reply.at(4) == (quint8)m_writeSent.at(3)
        && (quint8)reply.at(5) == (quint8)m_writeSent.at(2);
}

void SerialProto::writeStoveStateOn()
{
    writeStoveCmd(stoveStateIndex, StoveState::Starting);
}

void SerialProto::writeStoveStateOff()
{
    writeStoveCmd(stoveStateIndex, StoveState::FinalCleaning);
}

void SerialProto::writeStoveStateOffForce()
{
    writeStoveCmd(stoveStateIndex, StoveState::Off);
}

/* -/+ step from the value read from the stove: ignored before the first
 * read, kept inside POWER_MIN-POWER_MAX and SETPOINT_MIN_RAW-SETPOINT_MAX_RAW
 * (a value outside the range moves to the nearest limit) */
void SerialProto::writeStoveDecPower()
{
    if(!m_setPowerRead || m_setPower<=POWER_MIN)
        return;
    writeStoveCmd(powerSetIndex, qMin(m_setPower-1, POWER_MAX));
}

void SerialProto::writeStoveIncPower()
{
    if(!m_setPowerRead || m_setPower>=POWER_MAX)
        return;
    writeStoveCmd(powerSetIndex, qMax(m_setPower+1, POWER_MIN));
}

void SerialProto::writeStoveDecSetPoint()
{
    if(!m_setTempRead || m_setTempRaw<=SETPOINT_MIN_RAW)
        return;
    writeStoveCmd(tempSetIndex, qMin(m_setTempRaw-1, SETPOINT_MAX_RAW));
}

void SerialProto::writeStoveIncSetPoint()
{
    if(!m_setTempRead || m_setTempRaw>=SETPOINT_MAX_RAW)
        return;
    writeStoveCmd(tempSetIndex, qMax(m_setTempRaw+1, SETPOINT_MIN_RAW));
}

void SerialProto::writeChronoEnable(bool val)
{
    writeStoveCmd(chronoEnableIndex, val);
}

void SerialProto::writeChronoWkeEnable(bool val)
{
    writeStoveCmd(chronoWkE_EnableIndex, val);
}

void SerialProto::writeChronoWke1On(quint8 val)
{
    writeStoveCmd(chronoWkE_1_OnIndex, val);
}

void SerialProto::writeChronoWke1Off(quint8 val)
{
    writeStoveCmd(chronoWkE_1_OffIndex, val);
}
