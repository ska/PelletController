#include "serialproto.h"

SerialProto *SerialProto::instance = nullptr;

SerialProto::SerialProto(QObject *parent)
    : QObject{parent}
{
    m_selSerial = SERIAL_PORT_DEFAULT;
    m_loopEnabled = false;
    m_linkState = LinkOk;
    m_state = m_previousState = m_nextState = 0;
    m_stoveState = m_flamePower = m_setPower = 0;
    m_smokeFanRpm = 0;
    m_setTempRaw = 0;
    m_setTempRead = m_setPowerRead = false;
    m_txMessages = m_rxMessages = m_rxErrors = m_rxChecksumErrors = m_rxTimeouts = 0;
    m_readPending = false;
    m_writeAttempts = 0;
    m_writeCurrent = {};
    m_readRetryPending = m_readRetried = false;
    m_stoveYear = m_stoveMonth = m_stoveDay = m_stoveDoW = m_stoveHour = m_stoveMinutes = m_stoveSeconds = 0;
    m_chronoSerialGet = false;
    m_writeInFlight = false;
    m_rxIdleTimer.setSingleShot(true);
    m_writeTimer.setSingleShot(true);
    m_reopenTimer.setSingleShot(true);
    m_noReplyTimer.setSingleShot(true);

    connect(&m_serial, &QSerialPort::readyRead, this, &SerialProto::onReadyRead);
    connect(&m_serial, &QSerialPort::bytesWritten, this, &SerialProto::bytesWritten);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &SerialProto::onSerialError);
    connect(&m_reopenTimer, &QTimer::timeout, this, &SerialProto::openSerPort);
    connect(&m_noReplyTimer, &QTimer::timeout, this, [this]{ setLinkState(LinkNoReply); });
    connect(&m_loopTimer, &QTimer::timeout, this, &SerialProto::getStoveInfos);
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


void SerialProto::setSerPort(const QString &serPortName)
{
    m_selSerial = serPortName;
}

/* Opens the port; if it fails, tries again every SERIAL_REOPEN_DELAY until
 * closeSerPort (USB adapter missing or unplugged). The polling starts here
 * when already wanted. */
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
        //Risposta a lettura: eco uguale alla richiesta, poi la tolgo (una risposta di solo eco non e' un valore)
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

        storeValue(RequestsIndex(m_previousState), val);
    } else {
        //Lunghezza sbagliata o eco diversa dalla richiesta (solo eco = stufa muta)
        qWarning() << "Invalid reply:" << stoveRxData.toHex(' ') << "to" << m_readSent.toHex(' ');
        m_rxErrors++;
        m_readRetryPending = !m_readRetried;
    }
    emit updateStats(m_txMessages, m_rxMessages, m_rxErrors, m_rxChecksumErrors, m_rxTimeouts);
}

/* A valid value read (or confirmed by a write): kept and emitted. Every
 * parameter counts in Rx, also the ones the panel does not use, so that
 * Tx = Rx + Err + Chk + Tmo */
void SerialProto::storeValue(RequestsIndex index, quint8 val)
{
    switch (index)
    {
    case ambTempIndex:
        emit updateAmbTemp(val / 2.0f);
        break;
    case stoveStateIndex:
        if(val != m_stoveState)
            qInfo() << "Stove state:" << m_stoveState << "->" << val;
        m_stoveState = val;
        //Stato fuori tabella (scheda stufa piu' recente o risposta rumorosa): niente at() fuori range
        emit updateStoveState(m_stoveState, (m_stoveState < m_stoveStateStr.size())
                                                ? m_stoveStateStr.at(m_stoveState)
                                                : QString("Unknown (%1)").arg(m_stoveState));
        break;
    case flamePowerIndex:
        //From 0-16 to 10-100%, 0 when Off or from Final Cleaning on (no stale value)
        if (m_stoveState > Off && m_stoveState < FinalCleaning)
            m_flamePower = (((quint16)val*90)/16)+10;
        else
            m_flamePower = 0;
        //Shown at once, not only with the set power later in the round
        if(m_setPowerRead)
            emit updatePower(m_setPower, m_flamePower);
        break;
    case smokeFanSpeedIndex:
        m_smokeFanRpm = val ? (quint16(val)*10)+250 : 0;
        break;
    case smokeTempIndex:
        emit updateSmoke(val, m_smokeFanRpm);
        break;
    case powerInUseIndex:
        emit updatePowerInUse(val);
        break;
    case secondsCurrentIndex:       m_stoveSeconds = val;   break;
    case dayOfWeekIndex:            m_stoveDoW = val;       break;
    case hoursCurrentIndex:         m_stoveHour = val;      break;
    case minutesCurrentIndex:       m_stoveMinutes = val;   break;
    case dayOfMonthCurrentIndex:    m_stoveDay = val;       break;
    case monthCurrentIndex:         m_stoveMonth = val;     break;
    case yearCurrentIndex:          m_stoveYear = val;      break;
    case tempSetIndex:
        m_setTempRaw = val;
        m_setTempRead = true;
        emit updateSetTemp(val / 2.0f);
        break;
    case powerSetIndex:
        m_setPower = val;
        m_setPowerRead = true;
        emit updatePower(m_setPower, m_flamePower);
        break;
    default:
        if(index >= chronoEnableIndex && index <= chronoSet_4_DomEnabIndex)
        {
            emit chronoRead(index, val);
            if(index == chronoEnableIndex)
                emit updateChronoEnable(val);
        }
        break;
    }
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
            /* The board takes a new time at once, but the RAM clock copy
             * keeps the old one for a minute or two (checked on the stove) */
            QDateTime stove(QDate(2000+m_stoveYear, m_stoveMonth, m_stoveDay), QTime(m_stoveHour, m_stoveMinutes, m_stoveSeconds));
            if(m_clockSet.isValid() && m_clockSetAge.isValid() && m_clockSetAge.elapsed() < CLOCK_SET_HOLD_MS)
                stove = m_clockSet.addMSecs(m_clockSetAge.elapsed());
            emit updateStoveDateTime(stove);
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

        if(m_state > chronoSet_4_DomEnabIndex)
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
 * value and the value written: anything else is not a confirmed write */
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

/* Stove clock: written to EEPROM F8-FD in BCD (0x13 = 13), day of week 1 =
 * Monday, year - 2000; the board shows it in the RAM clock (65-6B) */
bool SerialProto::writeClock(const QDateTime &dt)
{
    if(!dt.isValid() || dt.date().year() < 2000 || dt.date().year() > 2099)
        return false;
    auto bcd = [](int v) { return quint8(((v / 10) << 4) | (v % 10)); };
    const QDate d = dt.date();
    const QTime t = dt.time();
    qInfo() << "Stove clock set to" << dt.toString("yyyy-MM-dd HH:mm");
    m_clockSet = QDateTime(d, QTime(t.hour(), t.minute()));
    m_clockSetAge.start();
    emit updateStoveDateTime(m_clockSet);
    writeStoveCmd(clockSet_DoWIndex, bcd(d.dayOfWeek()));
    writeStoveCmd(clockSet_HourIndex, bcd(t.hour()));
    writeStoveCmd(clockSet_MinuteIndex, bcd(t.minute()));
    writeStoveCmd(clockSet_DayIndex, bcd(d.day()));
    writeStoveCmd(clockSet_MonthIndex, bcd(d.month()));
    writeStoveCmd(clockSet_YearIndex, bcd(d.year() - 2000));
    return true;
}

/* Any chrono parameter: on/off and day flags 0/1, times 0-143 (tens of
 * minutes from 00:00) or 144 = slot off. Anything else is refused. */
bool SerialProto::writeChrono(RequestsIndex index, quint8 value)
{
    if(index < chronoEnableIndex || index > chronoSet_4_DomEnabIndex)
        return false;
    const bool time = requestsMap[index].scale == 10;
    if(value > (time ? 144 : 1))
    {
        qWarning() << "Chrono write refused:" << requestsMap[index].description << value;
        return false;
    }
    writeStoveCmd(index, value);
    return true;
}
