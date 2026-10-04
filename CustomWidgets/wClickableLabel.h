#ifndef WCLICKABLELABEL_H
#define WCLICKABLELABEL_H

#include <QWidget>
#include <QLabel>

class wClickableLabel : public QLabel
{
    Q_OBJECT

public:
    wClickableLabel(QWidget *parent = nullptr);
    ~wClickableLabel();
    void setMyPixmap(const QString pixmapPath);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event);

private:
};

#endif // WCLICKABLELABEL_H
