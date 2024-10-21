#ifndef WCLICKABLELABEL_H
#define WCLICKABLELABEL_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class ClickableLabel;
}
QT_END_NAMESPACE


class wClickableLabel : public QWidget
{
    Q_OBJECT

public:
    wClickableLabel(QWidget *parent = nullptr);
    ~wClickableLabel();

    QString text() const;
    void setText(const QString &newText);

    QPixmap pixmap() const;
    void setPixmap(const QPixmap &newPixmap);
    void setPixmap(const QString pixmapPath);

signals:
    void clicked();
    void textChanged(QString);
    void pixmapChanged();

protected:
    void mousePressEvent(QMouseEvent* event);

private:
    Ui::ClickableLabel *ui;
    QPixmap _pixmap;

    Q_PROPERTY(QString text   READ text   WRITE setText   NOTIFY textChanged(QString))
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged FINAL)
};

#endif // WCLICKABLELABEL_H
