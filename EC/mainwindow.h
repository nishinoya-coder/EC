#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <dialog_cv.h>
#include <dialog_dpv.h>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QMessageBox>
#include <QtNetwork/QTcpServer>                 //监听套接字
#include <QtNetwork/QTcpSocket>                 //通信套接字//对方的(客户端的)套接字(通信套接字)
#include <vector>
#include <QVector>
#include <QStringListModel>
#include <QStringList>

#include <QDebug>
#include <thread>
#include <mutex>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
//#include <QReadWriteLock>
#include <threadsetdpv.h>
#include <threadsetcv.h>
#include <threaddrawcv.h>
//#include <threaddrawdpv.h>

#define TIME_CYC 20
#define TIME_SEND 100                   //发送端发送时间间隔（单位：ms）
#define TIME_1S 1000/TIME_SEND          //每 TIME_CYC 毫秒采集一次
#define MINUTE_60S 60*TIME_1S
#define BUFFERSIZE 30*MINUTE_60S//队列寸30分钟内的数据
#define	ARRAY 2//阵列信号数
#define SERIES_NUM 1
#define THREAD_NUM 2//线程数

QT_CHARTS_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static unsigned long CV_Tw;                                   //延迟测试时间（s）
    static int CV_E0, CV_E1, CV_E2;     //初始电压；最小电压；最大电压（mV）
    static unsigned long CV_dE;                                  //电压梯度（mV/s）
    static int CV_Cycle;                                //循环次数
    static int CV_E;

    static unsigned long DPV_Tw, DPV_Ti;       //延迟测试时间；脉冲延迟时间（s）
    static int DPV_E1, DPV_E2;                 //初始电压；结束电压（mV）
    static int DPV_PH, DPV_SH;                 //脉冲高度；脉冲阶差（mV）
    static unsigned long DPV_PW, DPV_ST;                 //脉冲宽度；周期宽度（ms）
    static int DPV_E;

    static int Ended;
    static int reading;
    static int sendvoltstart;

    QValueAxis *axisX;
    QValueAxis *axisY;
    QChart *drawing;
    QLineSeries* series;

signals:
    void sendvector(QVector<float>);

private:
    Ui::MainWindow *ui;
    //子窗口
    Dialog *Dia_CV;
    Dialog_DPV *Dia_DPV;
    //图像
    QChartView *drawview;
    QTcpSocket* tcpClient;

    ThreadSetCV *threadsetcv;
    ThreadSetDPV *threadsetdpv;
    ThreadDrawCV *threaddrawcv;
    //ThreadDrawDPV *threaddrawdpv;

    void threaddraw();
    void lastdraw();

private slots:
    void CV_dealDone();   //线程槽函数
    void DPV_dealDone();

    void Model_check();
    void CV_start();
    void DPV_start();
    void Func_stop();
    void Redraw();
    void Clear_data();


    void save_excel();
    void save_txt();
    void save_graph();
    void storeMessage();

    void CV_sendVolt();
    void DPV_sendVolt();
    void CV_adddraw();

};

#endif // MAINWINDOW_H
