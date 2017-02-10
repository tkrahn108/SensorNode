#include "sensornode_gui.h"
#include "ui_sensornode_gui.h"


SensorNode_GUI::SensorNode_GUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SensorNode_GUI)
{
    ui->setupUi(this);

    //register own struct to use it in signal slot mechanism
    qRegisterMetaType<ble_message>("ble_message");

    thread = new QThread();
    ble_worker = new BLE_Connection();
    if(ble_worker->init() != 0){
        printText("Error opening serialport!");
    }

    ble_worker->moveToThread(thread);

    connect(ble_worker, SIGNAL(scanRequested()), thread, SLOT(start()));
    connect(thread, SIGNAL(started()), ble_worker, SLOT(doScan()));
    connect(ble_worker, SIGNAL(valueChanged(ble_message)), this, SLOT(setNewMessage(ble_message)), Qt::ConnectionType::QueuedConnection);
    connect(ble_worker, SIGNAL(aborted()), thread, SLOT(quit()));
}

SensorNode_GUI::~SensorNode_GUI()
{
    ble_worker->requestMethod(BLE_Connection::Disconnect);
    ble_worker->abort();
    thread->wait();
    qDebug()<<"Deleting thread and worker in Thread "<<this->QObject::thread()->currentThreadId();
    delete thread;
    delete ble_worker;

    delete ui;
}

void SensorNode_GUI::printText(std::string s){
    ui->textEdit->append(QString::fromStdString(s));
}

void SensorNode_GUI::on_pushButtonStartScanning_clicked()
{
    ble_worker->requestScan();
}

void SensorNode_GUI::scanForDevices()
{

}

void SensorNode_GUI::on_pushButtonStopScanning_clicked()
{
    ble_worker->abort();
}

void SensorNode_GUI::on_pushButtonConnect_clicked()
{
    ble_worker->requestMethod(BLE_Connection::Connect);
}

void SensorNode_GUI::on_pushButtonDisconnect_clicked()
{
    ble_worker->requestMethod(BLE_Connection::Disconnect);
}

void SensorNode_GUI::setNewMessage(ble_message msg)
{
    std::string s = msg.name;
    if(msg.rssi != NULL){
        s += '\t';
        s += std::to_string(msg.rssi);
        s += " dBm";
    }
    ui->textEdit->append(QString::fromStdString(s));
}

void SensorNode_GUI::on_pushButtonServiceDiscover_clicked()
{
    ble_worker->requestMethod(BLE_Connection::PrimaryServiceDiscovery);
}

void SensorNode_GUI::on_pushButtonCharacterisitcDiscover_clicked()
{
    ble_worker->requestMethod(BLE_Connection::CharacterisiticDiscovery);
}

void SensorNode_GUI::on_pushButtonDescriptorDiscover_clicked()
{
    ble_worker->requestMethod(BLE_Connection::DescriptorsDiscovery);
}

void SensorNode_GUI::on_pushButtonRead_clicked()
{
    ble_worker->requestMethod(BLE_Connection::ReadValueByHandle);
}
