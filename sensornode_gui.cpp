#include "sensornode_gui.h"
#include "ui_sensornode_gui.h"

SensorNode_GUI::SensorNode_GUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SensorNode_GUI)
{
    ui->setupUi(this);
}

SensorNode_GUI::~SensorNode_GUI()
{
    delete ui;
}

void SensorNode_GUI::printText(std::string s){
    ui->textEdit->append(QString::fromStdString(s));
}

void SensorNode_GUI::on_pushButton_clicked()
{
    printText("Test");
}
