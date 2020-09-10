#include "threaddrawcv.h"
#include <mainwindow.h>

ThreadDrawCV::ThreadDrawCV(QObject *parent) : QThread(parent)
{

}



void ThreadDrawCV::run()
{
    qDebug() << "0S draw" << endl;
    //emit add();
//    signal_data[0][cnt] = cnt-30;
//    series->append(signal_data[0][cnt]*10, signal_data[0][cnt]);
//    qDebug() << "drawing" << signal_data[0][cnt] << endl;
//    cnt += 1;
//    if(cnt == 60) Ended = 1;
//    qDebug() << "Ended" << Ended << endl;


    while(MainWindow::reading);
    for (int i = 0; i <= 60; i++)
    {
        //series->append((i-30)*10, s[i]);
        qDebug() << "drawing" << s[i] << endl;

    }
    //qDebug() << "Ended" << Ended << endl;
}


void ThreadDrawCV::getvector(QVector<float> vect)
{
    s = vect;
}
