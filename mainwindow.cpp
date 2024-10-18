#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug("%s tid=%#llx ", __PRETTY_FUNCTION__, (quintptr) QThread::currentThreadId() );
    /* Load Stylesheet */
    QFile file(":/qss/default.qss");
    file.open( QFile::ReadOnly );
    qApp->setStyleSheet( QString::fromLatin1( file.readAll() ));
    ui->setupUi(this);

    /* Init Gui */
    ui->ambTempCSym->setText("C");
    ui->ambTempDegSym->setText("°");
    ui->ambTemp->setText("xx.x");

    ui->setTempCSym->setText("C");
    ui->setTempDegSym->setText("°");
    ui->setTemp->setText("xx.x");


    m_serProto = SerialProto::getInstance();
    connect(m_serProto, SIGNAL(serialError(QString)), this, SLOT(updateStatusBar(QString)));
    connect(m_serProto, SIGNAL(updateAmbTemp(float)), this, SLOT(updateAmbTemp(float)));
    connect(m_serProto, SIGNAL(updateSetTemp(float)), this, SLOT(updateSetTemp(float)));



    m_serProto->openSerPort();
    m_serProto->startSerLoop();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateStatusBar(QString s)
{

    qDebug() << Q_FUNC_INFO;
    ui->statusbar->showMessage(s);
}

void MainWindow::updateAmbTemp(float f)
{
    qDebug() << Q_FUNC_INFO << f;

    ui->ambTemp->setText(QString::number(f, 'f', 1) );
}

void MainWindow::updateSetTemp(float f)
{
    qDebug() << Q_FUNC_INFO << f;

    ui->setTemp->setText(QString::number(f, 'f', 1) );
}
