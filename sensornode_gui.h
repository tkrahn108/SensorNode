#ifndef SENSORNODE_GUI_H
#define SENSORNODE_GUI_H

#include <QMainWindow>
#include <QThread>
#include "ble_connection.h"

namespace Ui {
class SensorNode_GUI;
}

struct ble_message;
class BLE_Connection;

class SensorNode_GUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit SensorNode_GUI(QWidget *parent = 0);
    ~SensorNode_GUI();
    void printText(std::string s);

private slots:
    void on_pushButtonStartScanning_clicked();
    void scanForDevices();
    void on_pushButtonStopScanning_clicked();
    void on_pushButtonConnect_clicked();
    void on_pushButtonDisconnect_clicked();
    void setNewMessage(ble_message);

    void on_pushButtonServiceDiscover_clicked();

    void on_pushButtonCharacterisitcDiscover_clicked();

    void on_pushButtonDescriptorDiscover_clicked();

    void on_pushButtonRead_clicked();

    void on_checkBoxConnected_toggled(bool checked);

    void on_checkBoxNotificationAL_toggled(bool checked);

private:
    Ui::SensorNode_GUI *ui;
    QThread *thread;
    BLE_Connection *ble_worker;

};

#endif // SENSORNODE_GUI_H
