#include "threadsetdpv.h"
#include <mainwindow.h>

ThreadSetDPV::ThreadSetDPV(QObject *parent) : QThread(parent)
{

}


void ThreadSetDPV::run()
{
    unsigned long Tw = MainWindow::DPV_Tw;
    unsigned long Ti = MainWindow::DPV_Ti;
    unsigned long PW = MainWindow::DPV_PW;
    unsigned long ST = MainWindow::DPV_ST;
    int E1 = MainWindow::DPV_E1;
    int E2 = MainWindow::DPV_E2;
    int PH = MainWindow::DPV_PH;
    int SH = MainWindow::DPV_SH;


    msleep(Tw*1000);
    MainWindow::DPV_E = E1;
//    qDebug() << "DPV_E is " << MainWindow::DPV_E << endl;
    emit send();
    MainWindow::sendvoltstart = 1;

    msleep(Ti*1000);
    while(MainWindow::DPV_E < (E2 - PH/2))
    {
        MainWindow::DPV_E += SH + PH;
//        qDebug() << "DPV_E is " << MainWindow::DPV_E << endl;
        emit send();
        msleep(PW);
        MainWindow::DPV_E -= PH;
//        qDebug() << "DPV_E is " << MainWindow::DPV_E << endl;
        emit send();
        msleep(ST-PW);
    }
    MainWindow::DPV_E = 0;
//    qDebug() << "DPV_E is " << MainWindow::DPV_E << endl;
    emit send();
//    qDebug() << "cycles end " << endl;

    emit isDone();//处理完后发送信号
}
