#ifndef THREADDRAWCV_H
#define THREADDRAWCV_H

#include <QThread>
#include <QDebug>
#include <Qvector>


class ThreadDrawCV : public QThread
{
    Q_OBJECT
public:
    explicit ThreadDrawCV(QObject *parent = nullptr);


protected:
    //QThread类的虚函数
    //线程处理函数
    //不能直接调用，通过start()间接调用
    void run();

signals:
    void isDone();
    void add();

public slots:
    void getvector(QVector<float>);

private:
    QVector<float> s;
};

#endif // THREADDRAWCV_H
