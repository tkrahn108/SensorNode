#include "sensornode_gui.h"
#include "ble_connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SensorNode_GUI w;
    w.show();
    return a.exec();
}
