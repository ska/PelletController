#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
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
    void updateAmbTemp(float f);
    void updateSetTemp(float f);
    void updateStoveState(quint8 u, QString s);
    void updatePower(quint8 set, quint8 flame);
    void updateStoveDateTime(QDateTime dt);
    void updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors);
    void handleStatteBtn();
    void handleStateForceBtn();
    void handlePowerMinBtn();
    void handlePowerPlusBtn();
    void handleSetPointMinBtn();
    void handleSetPointPlusBtn();
private:
    Ui::MainWindow *ui;
    SerialProto *m_serProto;
};
#endif // MAINWINDOW_H
