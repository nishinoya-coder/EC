#ifndef THREADSETCV_H
#define THREADSETCV_H

#include <QThread>
#include <QDebug>

class ThreadSetCV : public QThread
{
    Q_OBJECT
public:
    explicit ThreadSetCV(QObject *parent = nullptr);

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

#endif // THREADSETCV_H
