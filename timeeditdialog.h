#ifndef TIMEEDITDIALOG_H
#define TIMEEDITDIALOG_H

#include <QDialog>

namespace Ui {
class TimeEditDialog;
}

class TimeEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimeEditDialog(QWidget *parent = nullptr);
    ~TimeEditDialog();

    quint32 tensOfMins() const;
    void setTensOfMins(quint32 newTensOfMins);

protected:
    bool event(QEvent *event) override;

signals:
    void tensOfMinsUpdated(quint8);
    void touchMouseEvent();

private slots:
    void on_saveBtn_clicked();

    void on_cancelBtn_clicked();

private:
    Ui::TimeEditDialog *ui;
    quint32 m_tensOfMins;
};

#endif // TIMEEDITDIALOG_H
