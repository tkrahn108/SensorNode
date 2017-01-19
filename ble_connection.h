#ifndef BLE_CONNECTION_H
#define BLE_CONNECTION_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "cmd_def.h"
#include "apitypes.h"
#include "sensornode_gui.h"

class SensorNode_GUI;

class BLE_Connection
{
public:
    BLE_Connection(SensorNode_GUI*);
    ~BLE_Connection();
    static void output(uint8,uint8*,uint16,uint8*);
    int read_message();

private:
    void print_help();
};

#endif // BLE_CONNECTION_H
