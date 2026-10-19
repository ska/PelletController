#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

class ChronoPage;
class HomeAssistant;
class DiagPage;
class MainPage;
class QStackedWidget;
class QTimer;
class SerialProto;

/* The pages: main page, chrono page, diagnostic page. Back to the main
 * page after a while without touches on the other pages. */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(HomeAssistant *ha = nullptr, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showMain();
    void showChrono();
    void showDiag();
    void setStoveClock();
    void showWriteFailed(quint8 page, quint8 address, quint8 value);

    SerialProto *m_proto;
    QStackedWidget *m_stack;
    MainPage *m_mainPage;
    ChronoPage *m_chronoPage;
    DiagPage *m_diagPage;
    QTimer *m_idleTimer;
    QPointer<QWidget> m_edgeTarget;         //Widget pressed through the screen edge, until the release
};
#endif // MAINWINDOW_H
