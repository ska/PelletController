#include "uiicons.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

QIcon tintedIcon(const QString &name, const QColor &color, int size)
{
    static QHash<QString, QIcon> cache;
    const QString key = QString("%1/%2/%3").arg(name, color.name()).arg(size);
    if(cache.contains(key))
        return cache.value(key);

    QFile f(QString(":/icons/%1.svg").arg(name));
    if(!f.open(QIODevice::ReadOnly))
        return QIcon();
    QByteArray svg = f.readAll();
    svg.replace("currentColor", color.name().toLatin1());

    QSvgRenderer renderer(svg);
    QIcon icon;
    for(int scale : { 1, 2 })
    {
        QPixmap pm(size * scale, size * scale);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        renderer.render(&p);
        p.end();
        icon.addPixmap(pm);
    }
    cache.insert(key, icon);
    return icon;
}
