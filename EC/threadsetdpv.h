#ifndef THREADSETDPV_H
#define THREADSETDPV_H

#include <QThread>
#include <QDebug>

class ThreadSetDPV : public QThread
{
    Q_OBJECT
public:
    explicit ThreadSetDPV(QObject *parent = nullptr);

protected:
    //QThread类的虚函数
    //线程处理函数
    //不能直接调用，通过start()间接调用
    void run();

signals:
    void isDone();
    void send();

public slots:
};
#endif // THREADSETDPV_H
