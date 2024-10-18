#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QFile>
#include "serialproto.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateStatusBar(QString s);
    void updateAmbTemp(float f);
    void updateSetTemp(float f);
private:
    Ui::MainWindow *ui;
    SerialProto *m_serProto;
};
#endif // MAINWINDOW_H
