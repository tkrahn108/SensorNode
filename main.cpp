#include "sensornode_gui.h"
#include "ble_connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SensorNode_GUI w;
    w.show();
    BLE_Connection ble_connect(&w);
    w.setBLEConnection(&ble_connect);
    return a.exec();
}
