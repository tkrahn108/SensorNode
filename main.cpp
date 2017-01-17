#include "sensornode_gui.h"
#include "ble_connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SensorNode_GUI w;
    w.show();
    ble_connection ble_connect(&w);
    return a.exec();
}
