#include "wClickableLabel.h"
#include "ui_wClickableLabel.h"

wClickableLabel::wClickableLabel(QWidget *parent)
    : QLabel(parent)
{
}

wClickableLabel::~wClickableLabel()
{
}

void wClickableLabel::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit clicked();
}

void wClickableLabel::setMyPixmap(const QString pixmapPath)
{
    QPixmap pm;
    if(pm.load(pixmapPath))
    {
        pm = pm.scaled(this->size(),Qt::KeepAspectRatio);
        this->setPixmap(pm);
    }
}

