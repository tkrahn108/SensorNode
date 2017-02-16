#ifndef BLE_CONNECTION_H
#define BLE_CONNECTION_H

#include <QObject>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <QWaitCondition>
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
static int connected_devices_count;
static int scanned_devices_count;
static bool newAdvertisingDevice;

int cmp_bdaddr(bd_addr first, bd_addr second);
void print_bdaddr(bd_addr bdaddr);
bool checkScannedDevices(bd_addr bdaddr);

class BLE_Connection : public QObject
{
    Q_OBJECT

public:
    explicit BLE_Connection(QObject *parent = 0);
    static void output(uint8,uint8*,uint16,uint8*);
    int read_message();

    int init();
    void requestScan();
    void abort();

    /**
     * @brief This enum describes the various available methods
     */
    enum Method {
        DoNothing = 0,
        Connect = 1,
        Disconnect = 2,
        PrimaryServiceDiscovery = 3,
        CharacterisiticDiscovery = 4,
        DescriptorsDiscovery = 5,
        ReadValueByHandle = 6,
        NotificationOn = 7,
        NotificationOff = 8,
        DisconnectAll = 9
    };

    /**
     * @brief Requests for the method @em method to be executed
     *
     * This method will defines #_method and set #_abort to interrupt current method.
     * It is thread safe as it uses #mutex to protect access to #_method and #_abort variable.
     */
    void requestMethod(Method method);

    void setAddr(bd_addr addr);


private:
    /**
     * @brief Currently requested method
     */
    Method _method;
    bd_addr _addr;

    bool _abort;
    QMutex mutex;

    void primaryServiceDiscovery();
    void characterisiticDiscovery();
    void descriptorsDiscovery();
    void connect();
    void disconnect();
    void readValueByHandle();
    void notificationOn();
    void notificationOff();
    int getConnectionHandle(bd_addr adr);
    void disconnectAll();

signals:
    void scanRequested();
    void valueChanged(const ble_message &someMessage);
    void aborted();

public slots:
    void doScan();
};

#endif // BLE_CONNECTION_H
