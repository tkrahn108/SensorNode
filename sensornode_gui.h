#ifndef SENSORNODE_GUI_H
#define SENSORNODE_GUI_H

#include <QMainWindow>
#include <QTimer>
#include "ble_connection.h"

namespace Ui {
class SensorNode_GUI;
}

class BLE_Connection;

class SensorNode_GUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit SensorNode_GUI(QWidget *parent = 0);
    ~SensorNode_GUI();
    void printText(std::string s);
    void setBLEConnection(BLE_Connection *ble_connect);

    BLE_Connection *ble;
    QTimer *ScanTimer;

private slots:
    void on_pushButton_clicked();
    void scanForDevices();

    void on_pushButton_2_clicked();

private:
    Ui::SensorNode_GUI *ui;

};

#endif // SENSORNODE_GUI_H
