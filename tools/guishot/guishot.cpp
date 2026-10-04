// Renders screenshots of the PelletController GUI with sample stove data,
// without a stove: the data is injected through the SerialProto signals.
//
// Usage: QT_QPA_PLATFORM=offscreen ./guishot <output dir>

#include "mainwindow.h"
#include "serialproto.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QDir>

static void settle()
{
    for (int i = 0; i < 5; i++)
        QApplication::processEvents();
}

static bool save(const QPixmap &pix, const QString &dir, const QString &name)
{
    const QString path = QDir(dir).filePath(name);
    const bool ok = pix.save(path);
    qInfo() << (ok ? "Saved" : "ERROR saving") << path;
    return ok;
}

static void sampleData(SerialProto *s, quint8 state, quint8 setPower, quint8 flamePower)
{
    emit s->updateAmbTemp(20.5f);
    emit s->updateSetTemp(21.5f);
    emit s->updateStoveState(state, s->m_stoveStateStr.at(state));
    emit s->updatePower(setPower, flamePower);
    emit s->updateStoveDateTime(QDateTime(QDate(2026, 10, 4), QTime(18, 30)));
    emit s->updateStats(1520, 1498, 3);
    emit s->updateChronoEnable(true);
    emit s->updateChronoWkEEnable(true);
    emit s->updateChronoWkE1On(32);
    settle();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const QString outDir = argc > 1 ? argv[1] : ".";
    QDir().mkpath(outDir);

    QFontDatabase::addApplicationFont(":/fonts/AdvancedDotDigital7ls.ttf");

    MainWindow w;
    w.setGeometry(0, 0, 800, 480);
    w.show();

    // No stove: stop polling so only the sample data is shown
    SerialProto *s = SerialProto::getInstance();
    s->closeSerPort();

    bool ok = true;

    sampleData(s, SerialProto::Working, 3, 66);
    ok &= save(w.grab(), outDir, "main-working.png");

    sampleData(s, SerialProto::Alarm, 3, 0);
    ok &= save(w.grab(), outDir, "main-alarm.png");

    sampleData(s, SerialProto::Off, 3, 0);
    ok &= save(w.grab(), outDir, "main-off.png");

    sampleData(s, SerialProto::Working, 3, 66);
    QMetaObject::invokeMethod(&w, "on_chronoClbl_clicked");
    settle();
    ok &= save(w.grab(), outDir, "chrono-weekend.png");

    // Time picker dialog drawn over the chrono page
    QWidget *lcd = w.findChild<QWidget *>("chronoWkEStart1LCD");
    QMetaObject::invokeMethod(&w, "slot_chronoWkE_LCD_clicked", Q_ARG(QWidget *, lcd));
    settle();
    QWidget *dlg = w.findChild<TimeEditDialog *>();
    QPixmap shot = w.grab();
    if (dlg && dlg->isVisible())
    {
        QPainter p(&shot);
        p.fillRect(shot.rect(), QColor(0, 0, 0, 100));
        const QPixmap d = dlg->grab();
        p.drawPixmap((shot.width() - d.width()) / 2, (shot.height() - d.height()) / 2, d);
    } else {
        qWarning() << "Time edit dialog not shown";
        ok = false;
    }
    ok &= save(shot, outDir, "chrono-time-edit.png");

    return ok ? 0 : 1;
}
