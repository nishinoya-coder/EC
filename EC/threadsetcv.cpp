#include "threadsetcv.h"
#include <mainwindow.h>

ThreadSetCV::ThreadSetCV(QObject *parent) : QThread(parent)
{

}


void ThreadSetCV::run()
{
    unsigned long Tw = MainWindow::CV_Tw;
    int E0 = MainWindow::CV_E0;
    int E1 = MainWindow::CV_E1;
    int E2 = MainWindow::CV_E2;
    unsigned long dE = MainWindow::CV_dE;
    int Cycle = MainWindow::CV_Cycle;
    int i = 0;

    msleep(Tw*1000);
    MainWindow::CV_E = E0;
//    qDebug() << "CV_E is " << MainWindow::CV_E << endl;
    emit send();
    MainWindow::sendvoltstart = 1;
//    qDebug() << "tw get" << MainWindow::sendvoltstart << endl;

    //第0循环，由E0转为E1
    while(MainWindow::CV_E != E1)
    {
        if(MainWindow::CV_E > E1)
        {
            msleep(200);
            MainWindow::CV_E -= (dE/5);
//            qDebug() << "CV_E is " << MainWindow::CV_E << endl;
            emit send();
        }
        else
        {
            msleep(200);
            MainWindow::CV_E += (dE/5);
//            qDebug() << "CV_E is " << MainWindow::CV_E << endl;
            emit send();
        }
    }
//    qDebug() << "CY0 end " << endl;
    //正式循环开始
    while(i != Cycle)
    {
        while(MainWindow::CV_E < E2)
        {
            msleep(200);
            MainWindow::CV_E += (dE/5);
//            qDebug() << "CV_E is " << MainWindow::CV_E << endl;
            emit send();
            //qDebug() << "E2 is " << E2 << endl;
        }

        while(MainWindow::CV_E > E1)
        {
            msleep(200);
            MainWindow::CV_E -= (dE/5);
//            qDebug() << "CV_E is " << MainWindow::CV_E << endl;
            emit send();
            //if(MainWindow::CV_E <= E1)  break;
        }
        i +=1;
    }
    msleep(200);
    MainWindow::CV_E = 0;
//    qDebug() << "CV_E is " << MainWindow::CV_E << endl;
    emit send();
//    qDebug() << "cycles end " << endl;
    emit isDone();//处理完后发送信号
}
