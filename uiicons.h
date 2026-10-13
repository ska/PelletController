#ifndef UIICONS_H
#define UIICONS_H

#include <QColor>
#include <QIcon>
#include <QString>

/* Icon from :/icons/<name>.svg drawn in one color: the SVG files use
 * "currentColor", replaced here (24 x 24 stroke icons of the mockups) */
QIcon tintedIcon(const QString &name, const QColor &color, int size = 24);

#endif // UIICONS_H
