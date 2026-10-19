#include "singleinstance.h"

#include <QDebug>

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

SingleInstance::SingleInstance(const QString &name)
    : m_name(name.toUtf8())
{
}

SingleInstance::~SingleInstance()
{
    if(m_fd >= 0)
        ::close(m_fd);
}

/* true if this is the only instance on the target */
bool SingleInstance::tryLock()
{
    if(m_fd >= 0)
        return true;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    //sun_path[0] = '\0': abstract namespace, no file on disk
    const int len = qMin<int>(m_name.size(), sizeof(addr.sun_path) - 1);
    memcpy(addr.sun_path + 1, m_name.constData(), len);
    const socklen_t addrLen = offsetof(struct sockaddr_un, sun_path) + 1 + len;

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(fd < 0)
    {
        qCritical() << "SingleInstance: socket() failed:" << strerror(errno);
        return false;
    }
    if(::bind(fd, reinterpret_cast<struct sockaddr *>(&addr), addrLen) != 0)
    {
        if(errno == EADDRINUSE)
            qCritical() << "Another instance of" << m_name << "is already running";
        else
            qCritical() << "SingleInstance: bind() failed:" << strerror(errno);
        ::close(fd);
        return false;
    }
    m_fd = fd;
    return true;
}
