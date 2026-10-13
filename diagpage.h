#ifndef DIAGPAGE_H
#define DIAGPAGE_H

#include <QWidget>

class QDateTime;
class QLabel;
class SerialProto;

/*
 * Diagnostic page: serial port and link state, the Tx / Rx / Err / Chk /
 * Tmo counters, the stove values the main page does not show (power in
 * use, smoke fan, clock), the panel version. Opened from "Stove" on the
 * main page.
 */
class DiagPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiagPage(SerialProto *proto, QWidget *parent = nullptr);

signals:
    void backRequested();

private:
    QLabel *addRow(class QGridLayout *grid, const QString &name);
    void setLink(quint8 linkState);
    void setStats(quint32 tx, quint32 rx, quint32 err, quint32 chk, quint32 tmo);

    SerialProto *m_proto;
    QLabel *m_port;
    QLabel *m_link;
    QLabel *m_tx;
    QLabel *m_rx;
    QLabel *m_err;
    QLabel *m_chk;
    QLabel *m_tmo;
    QLabel *m_rate;
    QLabel *m_state;
    QLabel *m_powerInUse;
    QLabel *m_smoke;
    QLabel *m_fan;
    QLabel *m_clock;
};

#endif // DIAGPAGE_H
