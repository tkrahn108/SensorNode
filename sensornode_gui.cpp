#include "sensornode_gui.h"
#include "ui_sensornode_gui.h"


SensorNode_GUI::SensorNode_GUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SensorNode_GUI)
{
    ui->setupUi(this);
    ScanTimer = new QTimer(this);
    connect(ScanTimer, SIGNAL(timeout()),this, SLOT(scanForDevices()));
}

SensorNode_GUI::~SensorNode_GUI()
{
    delete ui;
}

void SensorNode_GUI::printText(std::string s){
    ui->textEdit->append(QString::fromStdString(s));
}

void SensorNode_GUI::setBLEConnection(BLE_Connection *ble_connect)
{
    ble = ble_connect;
}

void SensorNode_GUI::on_pushButton_clicked()
{
    ScanTimer->start(1000);
}

void SensorNode_GUI::scanForDevices()
{
    ble->read_message();
}

void SensorNode_GUI::on_pushButton_2_clicked()
{
    ScanTimer->stop();
}
