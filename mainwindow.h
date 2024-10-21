#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
#include <QMainWindow>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QTimeEdit>
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

protected:
    bool event(QEvent *event) override;

private slots:
    void showMainStack();
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
    void updateChronoEnable(bool b);
    void updateChronoWkeEnable(bool b);
    void updateChronoWkE1On(quint8 u);
    void updateChronoWkE1Off(quint8 u);
    void updateChronoWkE2On(quint8 u);
    void updateChronoWkE2Off(quint8 u);
    void updateChronoDayEnable(bool b);
    void updateChronoDay1On(quint8 u);
    void updateChronoDay1Off(quint8 u);
    void updateChronoDay2On(quint8 u);
    void updateChronoDay2Off(quint8 u);

    void handleClickableLabel();

    void on_chronoEnableCB_stateChanged(int arg1);

    void on_chronoWkEStart1CB_stateChanged(int arg1);

    void on_chronoWkEStop1CB_stateChanged(int arg1);

    void on_chronoWkEStart2CB_stateChanged(int arg1);

    void on_chronoWkEStop2CB_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
    SerialProto *m_serProto;
    QTimer *m_chronoShowTimer = nullptr;
    void updateChronoWdg(QCheckBox *cb, QTimeEdit *te, quint8 u);
};
#endif // MAINWINDOW_H
