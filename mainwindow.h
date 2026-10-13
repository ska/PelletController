#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSignalMapper>
#include <QMessageBox>
#include <QMainWindow>
#include <QTouchEvent>
#include <QLCDNumber>
#include <QKeyEvent>
#include <QTimeEdit>
#include <QThread>
#include <QFile>
#include "serialproto.h"
#include "timeeditdialog.h"
#include "wClickableLCDNumber.h"

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

protected:
    bool event(QEvent *event) override;

private slots:
    void showMainStack();
    void on_updateAmbTemp(float f);
    void on_updateSetTemp(float f);
    void on_updateStoveState(quint8 u, QString s);
    void on_updatePower(quint8 set, quint8 flame);
    void on_updateStoveDateTime(QDateTime dt);
    void on_updateStats(quint32 txMessages, quint32 rxMessages, quint32 rxErrors, quint32 rxChecksumErrors, quint32 rxTimeouts);
    void on_updateChronoEnable(bool b);
    void on_updateChronoWkeEnable(bool b);
    void on_updateChronoWkE1On(quint8 u);
    void on_updateChronoWkE1Off(quint8 u);
    void on_writeFailed(quint8 page, quint8 address, quint8 value);
    void on_linkStateChanged(quint8 linkState);

    void restartTimer();

    void on_chronoEnableCB_stateChanged(int arg1);
    void on_chronoWkEEnableCB_stateChanged(int arg1);
    void on_chronoWkEStart1CB_stateChanged(int arg1);
    void on_chronoWkEStop1CB_stateChanged(int arg1);
    //void on_chronoWkEStart1LCD_clicked();
    void on_stateBtn_released();
    void on_stateOffForceBtn_released();
    void on_setPointMinBtn_released();
    void on_setPointPlusBtn_released();
    void on_powerMinBtn_released();
    void on_powerPlusBtn_released();
    void on_chronoClbl_clicked();

    void chronoWkE_LCD_update(quint8);
    void slot_chronoWkE_LCD_clicked(QWidget *w);

private:
    Ui::MainWindow *ui;
    SerialProto *m_serProto;
    QTimer *m_chronoShowTimer = nullptr;
    TimeEditDialog *tm;
    wClickableLCDNumber *m_editLcd = nullptr;   //LCD that opened the time picker
    QSignalMapper *mapper;

    void updateUiData_ChronoWdg(QCheckBox *cb, wClickableLCDNumber *lcd, quint8 u);
};
#endif // MAINWINDOW_H
