#include "sensornode_gui.h"
#include "ui_sensornode_gui.h"


SensorNode_GUI::SensorNode_GUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SensorNode_GUI)
{
    ui->setupUi(this);

    thread = new QThread();
    ble_worker = new BLE_Connection();

    ble_worker->moveToThread(thread);

    connect(ble_worker, SIGNAL(workRequested()), thread, SLOT(start()));
    connect(thread, SIGNAL(started()), ble_worker, SLOT(doWork()));
    connect(ble_worker, SIGNAL(valueChanged(QString)), ui->textEdit, SLOT(append(QString)), Qt::ConnectionType::QueuedConnection);
}

SensorNode_GUI::~SensorNode_GUI()
{
    //TODO Why do I get the messasge: Destroyed while thread is still running
    delete thread;
    delete ble_worker;

    delete ui;
}

void SensorNode_GUI::printText(std::string s){
    ui->textEdit->append(QString::fromStdString(s));
}

void SensorNode_GUI::on_pushButtonStartScanning_clicked()
{
    ble_worker->requestWork();
}

void SensorNode_GUI::scanForDevices()
{
//    ble->read_message();
}

void SensorNode_GUI::on_pushButtonStopScanning_clicked()
{
//    ScanTimer->stop();
}

void SensorNode_GUI::on_pushButtonConnect_clicked()
{
//    ble->connect();
//    ScanTimer->stop();
}

void SensorNode_GUI::on_pushButtonDisconnect_clicked()
{
//    ble->disconnect();
}
