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

static bool messageCaptured;
static std::string message;

class BLE_Connection : public QObject
{
    Q_OBJECT

public:
    explicit BLE_Connection(QObject *parent = 0);
//    ~BLE_Connection();
    static void output(uint8,uint8*,uint16,uint8*);
    int read_message();
    void connect();
    void disconnect();

    void requestWork();
    void abort();

private:
    bool _abort;
    bool _working;
    QMutex mutex;

signals:
    void workRequested();
    void valueChanged(const QString &value);

public slots:
    void doWork();
};

#endif // BLE_CONNECTION_H
