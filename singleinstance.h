#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QByteArray>
#include <QString>

/*
 * One instance per target: binds a Linux abstract unix socket named
 * after the application. The kernel allows one owner per name, for any
 * user, and frees it when the process ends for any reason (exit, crash,
 * SIGKILL): no stale lock to clean, no file that can be removed.
 */
class SingleInstance
{
public:
    explicit SingleInstance(const QString &name);
    ~SingleInstance();

    bool tryLock();                         //Keep the object alive: the lock lasts as long as it

private:
    QByteArray m_name;
    int m_fd = -1;

    Q_DISABLE_COPY(SingleInstance)
};

#endif // SINGLEINSTANCE_H
