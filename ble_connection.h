#ifndef BLE_CONNECTION_H
#define BLE_CONNECTION_H

#include <QObject>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "cmd_def.h"
#include "apitypes.h"
#include "sensornode_gui.h"

class SensorNode_GUI;

struct ble_message {
    bd_addr ble_adress;
    std::string name;
    int8 rssi;
};

Q_DECLARE_METATYPE(ble_message)

static bool messageCaptured;
static ble_message message;

class BLE_Connection : public QObject
{
    Q_OBJECT

public:
    explicit BLE_Connection(QObject *parent = 0);
    static void output(uint8,uint8*,uint16,uint8*);
    int read_message();
    void connect();
    void disconnect();

    void requestScan();
    void abort();

private:
    bool _abort;
    bool _working;
    QMutex mutex;

signals:
    void scanRequested();
    void valueChanged(const ble_message &someMessage);
    void aborted();

public slots:
    void doScan();
};

#endif // BLE_CONNECTION_H
