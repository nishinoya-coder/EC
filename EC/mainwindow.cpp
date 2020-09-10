#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <dialog_cv.h>
#include <ui_dialog.h>
#include <dialog_dpv.h>
#include <ui_dialog_dpv.h>

#include <math.h>


using namespace std;

//设置socket信息
static int port = 8090;
static char* ServerIP = "192.168.4.1";

//设置坐标轴信息
static int Vc_min = -1000, Vc_max = 1000;
static int Ac_min = -100, Ac_max = 100;
static double Vc_range;

//设置模式信息
unsigned long MainWindow::CV_Tw = 2;            //延迟测试时间（s）
int MainWindow::CV_E0 = -600;
int MainWindow::CV_E1 = -600;
int MainWindow::CV_E2 = 800;                    //初始电压；最小电压；最大电压（mV）
unsigned long MainWindow::CV_dE = 50;                     //电压梯度（mV/s）
int MainWindow::CV_Cycle = 1;                   //循环次数
int MainWindow::CV_E = 0;

unsigned long MainWindow::DPV_Tw = 0;           //延迟测试时间（s）
unsigned long MainWindow::DPV_Ti = 1;           //脉冲延迟时间（s）
unsigned long MainWindow::DPV_PW = 50;          //脉冲宽度（ms）
unsigned long MainWindow::DPV_ST = 100;         //周期宽度（ms）
int MainWindow::DPV_E1 = -600;                  //初始电压（mV）
int MainWindow::DPV_E2 = 600;                   //结束电压（mV）
int MainWindow::DPV_PH = 50;                    //脉冲高度（mV）
int MainWindow::DPV_SH = 5;                     //脉冲阶差（mV）
int MainWindow::DPV_E = 0;

//设置数据信息
vector<float> signal_data;
QVector<signed short> signal_CVE;
static QByteArray buffer;

static int Started = 0;     //标志是否已开始采集
static int CV_MODE = 0;
static int DPV_MODE = 0;
int MainWindow::Ended = 0;     //标志扫描电压是否结束
static int index = -1;          //数据量计数
QString volt;        //发送电压
QString bitover = "/";
QString tranbit;

static int cnt = 0;
int MainWindow::sendvoltstart = 0;
int A=1;


//static QStringList text_content[ARRAY+2];//记录文本的变量
//static QStringListModel *model_text[ARRAY+2];

static QStringList text_content;//记录文本的变量
static QStringListModel *model_text;
static QColor Graph_color = {QColor(47, 79, 79)};//DarkSlateGray

static QTimer *timer;
static thread* threadcv;


//线程同步所需的信号量
static std::mutex bMutex;//互斥锁，防止添加数据或显示数据的时候被打断
int MainWindow::reading = 0;





/************************************************画图线程函数**********************************************/
//每个线程画step条曲线
void MainWindow::threaddraw() {

    while(reading);//等待读结束
    series->setPen(QPen(Graph_color, 1, Qt::SolidLine));

//    qDebug() << "cnt" << signal_CVE.count() << endl;
    for (int i = 0; i < signal_CVE.count(); i++)
    {
        series->append(signal_CVE[i], signal_data[i]);
//        qDebug() << "drawing" << signal_CVE[i] << signal_data[i] << endl;

    }
//    qDebug() << "Ended" << Ended << endl;
    return;
}


void MainWindow::lastdraw() {

    while(reading);//等待读结束
    series->setPen(QPen(Graph_color, 1, Qt::SolidLine));
    for (int i = 1; i < signal_CVE.count(); i++)
    {
        series->append(signal_CVE[i], signal_data[i]);
    }
    return;
}


void idle_thread() {}//空线程函数













/************************************************窗口启动函数**********************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //实例化tcpClient
    tcpClient = new QTcpSocket(this);

    //上位机的显示位，以后可能要用
    //for (int i = 0; i < ARRAY+2; i++)
    model_text = new QStringListModel(text_content);
    //for (int i = 0; i < ARRAY; i++)
    series = new QLineSeries;


        /*****************设置标题和图标***********************/
        setWindowTitle(u8"电化学测试平台");
        //setWindowIcon(QIcon("temp.ico"));
        /***********************设置QSS**************************/
//        QFile qssfile("style/base.qss");
//        qssfile.open(QFile::ReadOnly);
//        QString qss;
//        qss = qssfile.readAll();
//        this->setStyleSheet(qss);


/**********************新建子窗口************************/
    Dia_CV = new Dialog(this);
    Dia_CV->setModal(true);
    Dia_DPV = new Dialog_DPV(this);
    Dia_DPV->setModal(true);

/**********************连接槽函数************************/
    connect(ui->pushVoltage, SIGNAL(clicked()), this, SLOT(Model_check()));
    connect(Dia_CV->ui->pushButton_Start, SIGNAL(clicked()), this, SLOT(CV_start()));
    connect(Dia_DPV->ui->pushButton_Start, SIGNAL(clicked()), this, SLOT(DPV_start()));
    connect(ui->pushRedraw, SIGNAL(clicked()), this, SLOT(Redraw()));

    connect(ui->pushStop, SIGNAL(clicked()), this, SLOT(Func_stop()));
//    connect(ui->pushClear, SIGNAL(clicked()), this, SLOT(Clear_data()));
    connect(ui->action_txt, SIGNAL(triggered()), this, SLOT(save_txt()));
//    connect(ui->action_excel, SIGNAL(triggered()), this, SLOT(save_excel()));
    connect(ui->action_graph, SIGNAL(triggered()), this, SLOT(save_graph()));





/***********************画图****************************/
    drawing = new QChart;
    drawing->setTitle(u8"电化学测试曲线");
    drawview = new QChartView(drawing);
    axisX = new QValueAxis;
    axisY = new QValueAxis;
    //建立数据源队列

    //建立坐标轴
    QBrush AxisColor;
    AxisColor.setColor(Qt::black);
    axisX->setRange(Vc_min, Vc_max);                        // 设置范围
    axisX->setLabelFormat("%d");                            // 设置刻度格式
    axisX->setGridLineVisible(true);                        // 网格线可见
    axisX->setTickCount(6);                                 // 设置多少个大格
    axisX->setMinorTickCount(1);                            // 设置每个大格里面小刻度线的数目
    axisX->setTitleText(u8"E(mV)");                        // 设置刻度描述

    axisY->setRange(Ac_min, Ac_max);
    axisY->setLabelFormat("%.1f");
    axisY->setGridLineVisible(true);
    axisY->setTickCount(8);
    axisY->setMinorTickCount(1);
    axisY->setTitleText(u8"I(uA)");
    //为曲线添加坐标轴
    drawing->addAxis(axisX, Qt::AlignBottom);               // 下：Qt::AlignBottom  上：Qt::AlignTop
    drawing->addAxis(axisY, Qt::AlignLeft);                 // 左：Qt::AlignLeft    右：Qt::AlignRight
    drawing->legend()->hide();                              //隐藏图例
    //chart放入chartview内
    drawview->setRenderHint(QPainter::Antialiasing);        //防止图形走样
    ui->verticalLayout_5->addWidget(drawview);
    //为图表添加曲线
    drawing->addSeries(series);
    drawing->setAxisX(axisX, series);
    drawing->setAxisY(axisY, series);
}

MainWindow::~MainWindow()
{
    delete ui;
}















/*******************************************槽函数************************************************/
/*************功能：确认工作模式**********************/
void MainWindow::Model_check()
{
    switch (ui->comboVolt1->currentIndex())
    {
        case 0: Dia_CV->show(); break;
        case 1: Dia_DPV->show(); break;
        default: break;
    }
}


/*************功能：CV模式开始测试**********************/
void MainWindow::CV_start()
{
    switch (QMessageBox::question(this, u8"提示", u8"确认开始？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
    {
    case QMessageBox::Yes:
    if (Started == 1);          //do nothing
    else
    {
        //提取模式参数
        CV_Tw = Dia_CV->ui->lineEdit_Tw->text().toInt();
        CV_E0 = Dia_CV->ui->lineEdit_E0->text().toInt();
        CV_E1 = Dia_CV->ui->lineEdit_E1->text().toInt();
        CV_E2 = Dia_CV->ui->lineEdit_E2->text().toInt();
        CV_dE = Dia_CV->ui->lineEdit_dE->text().toInt();
        CV_Cycle = Dia_CV->ui->lineEdit_Cycle->text().toInt();
        switch (Dia_CV->ui->Box_Range->currentIndex())
        {
            case 0: axisY->setRange(-100, 100); break;
            case 1: axisY->setRange(-1000, 1000);break;
            default: break;
        }
        Dia_CV->accept();

//        qDebug() << "CV_Tw is " << CV_Tw << endl;
//        qDebug() << "CV_E0 is " << CV_E0 << endl;
//        qDebug() << "CV_E1 is " << CV_E1 << endl;
//        qDebug() << "CV_E2 is " << CV_E2 << endl;
//        qDebug() << "CV_dE is " << CV_dE << endl;
//        qDebug() << "CV_Cycle is " << CV_Cycle << endl;

//        int i;
//        for(i=0;i!=35;i++)  signal_data[i] = i*20/17-20;
//        for(i=35;i!=69;i++)  signal_data[i] = -(i*40/34)+60;
//        for(i=69;i!=86;i++)  signal_data[i] = (i-69)*20/17-20;



        //设置定时器
        timer = new QTimer;
        timer->start(100);

        //设置输出电压线程
        threadsetcv = new ThreadSetCV(this);
        connect(threadsetcv, SIGNAL(isDone()), this, SLOT(CV_dealDone()));
        connect(threadsetcv, SIGNAL(send()), this, SLOT(CV_sendVolt()));

        //设置画图线程
        //threaddrawcv = new ThreadDrawCV(this);
        //connect(threaddrawcv, SIGNAL(add()), this, SLOT(CV_adddraw()));
        //connect(this, SIGNAL(sendvector(QVector<float>)), threaddrawcv, SLOT(getvector(QVector<float>)));
        //emit sendvector(signal_data);
        threadcv = new std::thread(idle_thread);//创建空闲线程
        threadcv->join();

        qDebug() << "start" << endl;
        Started = 1;
        CV_MODE = 1;

        //connect(timer0, SIGNAL(timeout()), this, SLOT(storeMessage()));
        connect(timer, SIGNAL(timeout()), this, SLOT(CV_adddraw()));
        //threadsetcv->start();     //测试电压输出
        //threaddrawcv->start();



        //初始化TCP客户端
        delete tcpClient;
        tcpClient = new QTcpSocket(this);   //实例化tcpClient
        tcpClient->abort();                 //取消原有连接
        tcpClient->connectToHost(ServerIP, port);
        connect(tcpClient, SIGNAL(readyRead()), this, SLOT(storeMessage()));
        if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
        {
            threadsetcv->start();
            //connect(timer, SIGNAL(timeout()), this, SLOT(CV_adddraw()));
            //tcpClient->write("handshake");//先发一个才有回应
            Started = 1;
        }
        else
        {
            QMessageBox::warning(this, "Error", u8"服务器连接失败");
            break;
        }
    }
        break;
    case QMessageBox::No:
        break;
    default:
        break;
    }
}

void MainWindow::DPV_start()
{
    switch (QMessageBox::question(this, u8"提示", u8"确认开始？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
    {
    case QMessageBox::Yes:
    if (Started == 1);          //do nothing
    else
    {
        //提取模式参数
        DPV_Tw = Dia_DPV->ui->lineEdit_Tw->text().toInt();
        DPV_Ti = Dia_DPV->ui->lineEdit_Ti->text().toInt();
        DPV_E1 = Dia_DPV->ui->lineEdit_E1->text().toInt();
        DPV_E2 = Dia_DPV->ui->lineEdit_E2->text().toInt();
        DPV_PH = Dia_DPV->ui->lineEdit_PH->text().toInt();
        DPV_SH = Dia_DPV->ui->lineEdit_SH->text().toInt();
        DPV_PW = Dia_DPV->ui->lineEdit_PW->text().toInt();
        DPV_ST = Dia_DPV->ui->lineEdit_ST->text().toInt();
        switch (Dia_DPV->ui->Box_Range->currentIndex())
        {
            case 0: axisY->setRange(-100, 100); break;
            case 1: axisY->setRange(-1000, 1000);break;
            default: break;
        }
        Dia_DPV->accept();


//        qDebug() << "DPV_Tw is " << DPV_Tw << endl;
//        qDebug() << "DPV_Ti is " << DPV_Ti << endl;
//        qDebug() << "DPV_E1 is " << DPV_E1 << endl;
//        qDebug() << "DPV_E2 is " << DPV_E2 << endl;
//        qDebug() << "DPV_PH is " << DPV_PH << endl;
//        qDebug() << "DPV_SH is " << DPV_SH << endl;
//        qDebug() << "DPV_PW is " << DPV_PW << endl;
//        qDebug() << "DPV_ST is " << DPV_ST << endl;

        //设置定时器和线程
        timer = new QTimer;
        timer->start(100);

        threadsetdpv = new ThreadSetDPV(this);
        connect(threadsetdpv, SIGNAL(isDone()), this, SLOT(DPV_dealDone()));
        connect(threadsetdpv, SIGNAL(send()), this, SLOT(DPV_sendVolt()));
        //threadsetdpv->start();

        threadcv = new std::thread(idle_thread);//创建空闲线程
        threadcv->join();

        qDebug() << "start" << endl;
        Started = 1;
        DPV_MODE = 1;

        //connect(timer0, SIGNAL(timeout()), this, SLOT(storeMessage()));
        connect(timer, SIGNAL(timeout()), this, SLOT(CV_adddraw()));
        //threadsetcv->start();     //测试电压输出
        //threaddrawcv->start();


        //初始化TCP客户端
        delete tcpClient;
        tcpClient = new QTcpSocket(this);   //实例化tcpClient
        tcpClient->abort();                 //取消原有连接
        tcpClient->connectToHost(ServerIP, port);
        connect(tcpClient, SIGNAL(readyRead()), this, SLOT(storeMessage()));
        if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
        {
            threadsetdpv->start();
            Started = 1;
        }
        else
        {
            QMessageBox::warning(this, "Error", u8"服务器连接失败");
            break;
        }
    }
        break;
    case QMessageBox::No:
        break;
    default:
        break;
    }
}




void MainWindow::Func_stop()
{
    switch (QMessageBox::question(this, u8"提示", u8"确认停止采集？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
    {
    case QMessageBox::Yes:
        if (Started == 0);//do nothing
        else
        {
            Started = 0;
            delete threadcv;//删除线程
            timer->stop();
            delete timer;//删除采集线程

            if(CV_MODE)
            {
                threadsetcv->quit();        //停止线程
                qDebug() << "x4" << endl;
                qDebug() << "x5" << endl;
                CV_MODE = 0;
            }
            else if(DPV_MODE)
            {
                threadsetdpv->quit();        //停止线程
                DPV_MODE = 0;
            }
        }
        break;
    case QMessageBox::No:
        //do nothing
        break;
    default:
        break;
    }
}


void MainWindow::Redraw()
{
    if(Ended==1)
    {
        axisX->setRange(ui->lineEdit_Xmin->text().toInt(), ui->lineEdit_Xmax->text().toInt());
        axisY->setRange(ui->lineEdit_Ymin->text().toInt(), ui->lineEdit_Ymax->text().toInt());

        delete series;
        series = new QLineSeries;
        drawing->addSeries(series);
        drawing->setAxisX(axisX, series);
        drawing->setAxisY(axisY, series);

        threadcv = new std::thread(&MainWindow::lastdraw, this);
        threadcv->join();
        delete threadcv;    //可以直接删吗？ join是等待新线程结束，所以应该可以直接删
    }
}

/*************功能：清空数据区和曲线******************/
//只有在不采集的情况下才能清空
//不仅清空显示，缓存里已有的数据也会清空
void MainWindow::Clear_data()
{
//    if (Started) {
//        //do nothing
//    }
//    else {
//        for (int i = 0; i < ARRAY+2; i++) {
//            text_content[i].clear();
//            series->clear();
//        }
//        index = -1;
//        signal_data.clear();
//        signal_data.resize(ARRAY, QVector<float>(BUFFERSIZE));
//    }
}




/*************功能：读取数据（可读时刻）**************/
void MainWindow::storeMessage()
{
    reading = 1;
//    qDebug() << "read" << endl;

    //读取缓冲区数据
    QByteArray buffer = tcpClient->readAll();         //ARRAY*4个字节
    qDebug() << "size:" << buffer.size() << endl;
//    qDebug() << buffer.toHex() << endl;

    uint buf_0,buf_1,buf_2,buf_3,buf;
    uint buf_volt1,buf_volt2,buf_volt;
    signed short bufvolt;
    int i;
    //if (index < BUFFERSIZE) index++;                    //用来判断数据是否超过了30分钟的数据量？

    for(i=0; i<(buffer.size()/6); i++)
    {
        if (index < BUFFERSIZE) index++;
        if (index >= BUFFERSIZE)  signal_data.pop_back();
        buf_0 = ((uint)buffer.at(0))&0x000000ff;
        buf_1 = ((uint)(buffer.at(1)) << 8) & 0x0000ff00;
        buf_2 = ((uint)(buffer.at(2)) << 16) & 0x00ff0000;
        buf_3 = ((uint)(buffer.at(3)) << 24) & 0xff000000;
        buf = buf_0 + buf_1 + buf_2 + buf_3;
        signal_data.push_back(*(float*)&buf);

        buf_volt1 = ((uint)buffer.at(4))&0x000000ff;
        buf_volt2 = ((uint)buffer.at(5) << 8)&0x0000ff00;
        buf_volt = buf_volt1 + buf_volt2;
        bufvolt = *(signed short*)&buf_volt;
        signal_CVE.append(bufvolt);
        qDebug() << index << bufvolt << *(float*)&buf << endl;
    }














/*
    switch(buffer.size())
    {
    case 4:
        if (index < BUFFERSIZE) index++;
        if (index >= BUFFERSIZE)  signal_data.pop_back();
        buf_0 = ((uint)buffer.at(0))&0x000000ff;
        buf_1 = ((uint)(buffer.at(1)) << 8) & 0x0000ff00;
        buf_2 = ((uint)(buffer.at(2)) << 16) & 0x00ff0000;
        buf_3 = ((uint)(buffer.at(3)) << 24) & 0xff000000;
        buf = buf_0 + buf_1 + buf_2 + buf_3;
    //        qDebug() << hex << buf_0 << buf_1 << buf_2 << buf_3 << endl;
    //        qDebug() << hex << buf << endl;

        signal_data.push_back(*(float*)&buf);
        qDebug() << index << *(float*)&buf << endl;
    //        qDebug() << *(float*)&buf << endl;
    //        qDebug() << signal_data.capacity() << endl;
    //        qDebug() << signal_data[signal_data.capacity()-1] << endl;
        break;
    case 2:
        buf_volt1 = ((uint)buffer.at(0))&0x000000ff;
        buf_volt2 = ((uint)buffer.at(1) << 8)&0x0000ff00;
        buf_volt = buf_volt1 + buf_volt2;
//        qDebug() << hex << buf_volt1 << buf_volt2 << endl;
//        qDebug() << hex << buf_volt << endl;
        bufvolt = *(signed short*)&buf_volt;
//        qDebug() << bufvolt << endl;
        signal_CVE.append(bufvolt);
//        qDebug() << signal_CVE[signal_CVE.count()-1] << endl;
        break;
    case 6:
        if (index < BUFFERSIZE) index++;
        if (index >= BUFFERSIZE)  signal_data.pop_back();
        buf_0 = ((uint)buffer.at(0))&0x000000ff;
        buf_1 = ((uint)(buffer.at(1)) << 8) & 0x0000ff00;
        buf_2 = ((uint)(buffer.at(2)) << 16) & 0x00ff0000;
        buf_3 = ((uint)(buffer.at(3)) << 24) & 0xff000000;
        buf = buf_0 + buf_1 + buf_2 + buf_3;
        signal_data.push_back(*(float*)&buf);
        qDebug() << index << *(float*)&buf << endl;

        buf_volt1 = ((uint)buffer.at(4))&0x000000ff;
        buf_volt2 = ((uint)buffer.at(5) << 8)&0x0000ff00;
        buf_volt = buf_volt1 + buf_volt2;
        bufvolt = *(signed short*)&buf_volt;
        signal_CVE.append(bufvolt);
        break;
    default:    break;
    }*/

//    if(buffer.size() == 4)
//    {
//        if (index < BUFFERSIZE) index++;
//        if (index >= BUFFERSIZE)  signal_data.pop_back();
//        uint buf_0 = ((uint)buffer.at(0))&0x000000ff;
//        uint buf_1 = ((uint)(buffer.at(1)) << 8) & 0x0000ff00;
//        uint buf_2 = ((uint)(buffer.at(2)) << 16) & 0x00ff0000;
//        uint buf_3 = ((uint)(buffer.at(3)) << 24) & 0xff000000;
//        uint buf = buf_0 + buf_1 + buf_2 + buf_3;
//        qDebug() << hex << buf_0 << buf_1 << buf_2 << buf_3 << endl;
//        qDebug() << hex << buf << endl;

//        signal_data.push_back(*(float*)&buf);
//        qDebug() << index << *(float*)&buf << endl;
//        qDebug() << *(float*)&buf << endl;
//        qDebug() << signal_data.capacity() << endl;
//        qDebug() << signal_data[signal_data.capacity()-1] << endl;
//    }
//    else if(buffer.size() == 2)
//    {
//        uint buf_volt1 = ((uint)buffer.at(0))&0x000000ff;
//        uint buf_volt2 = ((uint)buffer.at(1) << 8)&0x0000ff00;
//        uint buf_volt = buf_volt1 + buf_volt2;
//        qDebug() << hex << buf_volt1 << buf_volt2 << endl;
//        qDebug() << hex << buf_volt << endl;
//        signed short bufvolt = *(signed short*)&buf_volt;
//        qDebug() << bufvolt << endl;
//        signal_CVE.append(bufvolt);
//        qDebug() << signal_CVE[signal_CVE.count()-1] << endl;
//    }
//    else if(buffer.size() == 6)
//    {
//        if (index < BUFFERSIZE) index++;
//        if (index >= BUFFERSIZE)  signal_data.pop_back();
//        uint buf_0 = ((uint)buffer.at(0))&0x000000ff;
//        uint buf_1 = ((uint)(buffer.at(1)) << 8) & 0x0000ff00;
//        uint buf_2 = ((uint)(buffer.at(2)) << 16) & 0x00ff0000;
//        uint buf_3 = ((uint)(buffer.at(3)) << 24) & 0xff000000;
//        uint buf = buf_0 + buf_1 + buf_2 + buf_3;
//        signal_data.push_back(*(float*)&buf);
//        qDebug() << index << *(float*)&buf << endl;

//        uint buf_volt1 = ((uint)buffer.at(4))&0x000000ff;
//        uint buf_volt2 = ((uint)buffer.at(5) << 8)&0x0000ff00;
//        uint buf_volt = buf_volt1 + buf_volt2;
//        signed short bufvolt = *(signed short*)&buf_volt;
//        signal_CVE.append(bufvolt);
//    }




    reading = 0;
//    qDebug() << "readover" << endl;
    //if(CV_MODE = 1) CV_adddraw();
    //if(DPV_MODE = 1) DPV_adddraw();

}




/*************功能：发送电压**********************/
void MainWindow::CV_sendVolt()
{
    volt = QString::number(CV_E, 10);
    //volt = QString::number(-600, 10);

//    qDebug() << "volt is " << volt << endl;

    tcpClient->write(volt.toUtf8().data(), strlen(volt.toUtf8().data()));        //发电压
    tcpClient->write(bitover.toUtf8().data(), strlen(bitover.toUtf8().data()));
}

void MainWindow::DPV_sendVolt()
{
    volt = QString::number(DPV_E, 10);
//    qDebug() << "volt is " << volt << endl;

    tcpClient->write(volt.toUtf8().data(), strlen(volt.toUtf8().data()));        //发电压
    tcpClient->flush();
    tcpClient->write(bitover.toUtf8().data(), strlen(bitover.toUtf8().data()));
    tcpClient->flush();
}

void MainWindow::CV_dealDone()
{
    qDebug() << "it is over";  //打印线程结束信息
    Ended = 1;

    delete threadcv;    //删除画图线程
    timer->stop();
    delete timer;       //删除画图计时器

    threadsetcv->quit();        //停止线程
    threadsetcv->wait();        //等待线程处理完手头工作
}

void MainWindow::DPV_dealDone()
{
    qDebug() << "it is over";  //打印线程结束信息
    Ended = 1;

    delete threadcv;    //删除画图线程
    timer->stop();
    delete timer;       //删除画图计时器

    threadsetdpv->quit();       //停止线程
    threadsetdpv->wait();       //等待线程处理完手头工作
}

void MainWindow::CV_adddraw()
{
    //为可变坐标轴做准备
//    delete series[j];
//    series[j] = new QLineSeries;
//    drawing->addSeries(series[j]);
//    drawing->setAxisX(axisX, series[j]);
//    drawing->setAxisY(axisY, series[j]);
//    model_text[j] = new QStringListModel(text_content[j]);
//    model_text[j]->setStringList(text_content[j]);
//    RT_list[j]->setModel(model_text[j]);

    if(sendvoltstart==1)
    {
        delete threadcv;
        delete series;
        series = new QLineSeries;
        drawing->addSeries(series);
        drawing->setAxisX(axisX, series);
        drawing->setAxisY(axisY, series);

        threadcv = new std::thread(&MainWindow::threaddraw, this);
        threadcv->join();
    }
}






/*************功能：数据保存成excel*******************/
void MainWindow::save_excel()
{
    //行数列数
//    int row_num = index;
//    int i = 0;

//    QXlsx::Document xlsx;
//    xlsx.write("A1", u8"通道一"); xlsx.write("B1", u8"通道二"); xlsx.write("C1", u8"通道三");
//    xlsx.write("D1", u8"通道四"); xlsx.write("E1", u8"通道五"); xlsx.write("F1", u8"通道六");

//    for (int i = 0; i <= index; i++) {
//        for(int j = 0; j< ARRAY; j++)
//            xlsx.write(i + 2, j + 1, signal_data[j][i]);//row col
//    }

//    QString fileName = QFileDialog::getSaveFileName(this, "Save",
//        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
//        "Excel files(*.xlsx *.xls)");
//    xlsx.saveAs(QDir::toNativeSeparators(fileName));
}

/*************功能：数据保存成txt*********************/
void MainWindow::save_txt()
{
    QString save_fileName = QFileDialog::getSaveFileName(this, "Save",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "text files(*.txt)");

    //对文件进行操作
    QFile file(save_fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    {
        //QMessageBox::warning(this, "Error", u8"打开失败", QMessageBox::Yes);
        return;
    }
    QTextStream in(&file);

    for (int i=1; i < signal_CVE.count(); i++)
    {
        in << signal_CVE[i] << "	" << signal_data[i];
        in << "\n";
    }

    file.close();
}

/*************功能：保存图像**************************/
void MainWindow::save_graph()
{
    QPixmap save_P = QPixmap::grabWidget(drawview);
    QString save_fileName = QFileDialog::getSaveFileName(this, "Save",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "graph files(*.png *.jpg *.bmp)");

    save_P.save(save_fileName);
}








