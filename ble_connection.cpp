#include "ble_connection.h"

volatile HANDLE serial_handle;
#define MAX_DEVICES 64
int found_devices_count;
bd_addr found_devices[MAX_DEVICES];
uint8 primary_service_uuid[] = {0x00, 0x28};
uint8 connection_handle;

BLE_Connection::BLE_Connection(QObject *parent) :
    QObject(parent)
{
    char str[80];
    //TODO getting serialport from GUI
    snprintf(str,sizeof(str)-1,"\\\\.\\%s","COM3");
    //open I/O device
    serial_handle = CreateFileA(str,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);


    if (serial_handle == INVALID_HANDLE_VALUE)
    {
        char buff[100];
        snprintf(buff, sizeof(buff), "Error opening serialport %s. %d\n","COM3",(int)GetLastError());
        //        gui->printText(buff);
    }

    bglib_output = output;

    messageCaptured = false;
    found_devices_count = 0;
    _interrupt = false;

    //stop previous operation
    ble_cmd_gap_end_procedure();
    //get connection status,current command will be handled in response
    ble_cmd_connection_get_status(0);
}

void BLE_Connection::requestMethod(BLE_Connection::Method method)
{
    qDebug()<<"Request worker Method"<<method<<"in Thread "<<thread()->currentThreadId();
//    QMutexLocker locker(&mutex);
    mutex.lock();
    _interrupt = true;
    _method = method;
    mutex.unlock();
//    condition.wakeOne();
}

void BLE_Connection::requestScan()
{
    mutex.lock();
    _working = true;
    _abort = false;
    //    qDebug()<<"Request worker start in Thread "<<thread()->currentThreadId();
    mutex.unlock();

    emit scanRequested();
}

void BLE_Connection::abort()
{
//    mutex.lock();
//    if (_working) {
//        _abort = true;
//        _working = false;
//        //        qDebug()<<"Request worker aborting in Thread "<<thread()->currentThreadId();
//    }
//    mutex.unlock();
//    emit aborted();
    qDebug()<<"Request worker aborting in Thread "<<thread()->currentThreadId();
//    QMutexLocker locker(&mutex);
    mutex.lock();
    _abort = true;
    mutex.unlock();
//    condition.wakeOne();
}

void BLE_Connection::doScan()
{
    while(!_abort){
        qDebug() << "Within doSan";
        if( messageCaptured ){
            emit valueChanged(message);
            messageCaptured = false;
        }

        mutex.lock();
//        if (!_interrupt && !_abort) {
//            condition.wait(&mutex);
//        }
        _interrupt = false;

        Method method = _method;
        mutex.unlock();

        switch(method) {
        case DoNothing:
            qDebug() << "Do nothing";
            break;
        case Connect:
            connect();
            break;
        case Disconnect:
            disconnect();
            break;
        case PrimaryServiceDiscovery:
            primaryServiceDiscovery();
            break;
         default:
            qDebug() << "Default in switch case";
            break;
        }

        requestMethod(DoNothing);

        read_message();
    }

    if (_abort) {
        qDebug()<<"Aborting worker mainLoop in Thread "<<thread()->currentThreadId();
        mutex.unlock();
        emit aborted();
    }
}

void BLE_Connection::output(uint8 len1,uint8* data1,uint16 len2,uint8* data2)
{
    DWORD written;

    if(!WriteFile (serial_handle,
                   data1,
                   len1,
                   &written,
                   NULL
                   ))
    {
        printf("ERROR: Writing data. %d\n",(int)GetLastError());
        exit(-1);
    }

    if(!WriteFile (serial_handle,
                   data2,
                   len2,
                   &written,
                   NULL
                   ))
    {
        printf("ERROR: Writing data. %d\n",(int)GetLastError());
        exit(-1);
    }
}

int BLE_Connection::read_message()
{
    DWORD rread;
    const struct ble_msg *apimsg;
    struct ble_header apihdr;
    unsigned char data[256];//enough for BLE
    //read header

    if(!ReadFile(serial_handle,
                 (unsigned char*)&apihdr,
                 4,
                 &rread,
                 NULL))
    {
        return GetLastError();
    }
    if(!rread)return 0;
    //read rest if needed
    if(apihdr.lolen)
    {
        if(!ReadFile(serial_handle,
                     data,
                     apihdr.lolen,
                     &rread,
                     NULL))
        {
            return GetLastError();
        }
    }
    apimsg=ble_get_msg_hdr(apihdr);
    if(!apimsg)
    {
        printf("ERROR: Message not found:%d:%d\n",(int)apihdr.cls,(int)apihdr.command);
        return -1;
    }
    apimsg->handler(data);

    return 0;
}

void BLE_Connection::connect()
{
    uint8 addr[6] = {0xe6, 0x27, 0x6b, 0xc2, 0x37, 0xe0};
    ble_cmd_gap_connect_direct(addr, 1, 50, 3200, 400, 0);
}

void BLE_Connection::disconnect()
{
    //TODO change argument to connection_handle
    ble_cmd_connection_disconnect(0);
}

void BLE_Connection::primaryServiceDiscovery()
{
    uint8 uuid_len = sizeof(primary_service_uuid);
    ble_cmd_attclient_read_by_group_type(connection_handle, 0x0001, 0xFFFF, uuid_len, primary_service_uuid);
}

//=============FUNCTIONS FOR HANDELING EVENTS AND RESPONSES====================


void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg)
{
    // Parse data to get the name
    int i;
    char *name = NULL;
    for (i = 0; i < msg->data.len; ) {
        int8 len = msg->data.data[i++];
        if (!len) continue;
        if (i + len > msg->data.len) break; // not enough data
        uint8 type = msg->data.data[i++];
        switch (type) {
        case 0x09:
            name = (char*) malloc(len);
            memcpy(name, msg->data.data + i, len - 1);
            name[len - 1] = '\0';
        }

        i += len - 1;
    }

    message.ble_adress = msg->sender;
    if(name) message.name = name;
    else message.name = "Unknown";
    message.rssi = msg->rssi;
    messageCaptured = true;

    free(name);
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(msg->flags&connection_connected)
    {
        //        gui->printText("Connection established");
        message.name = "Connection established";
        messageCaptured = true;
        connection_handle = msg->connection;
    }else
    {
        //        gui->printText("#Not connected -> Scan");
        ble_cmd_gap_discover(1);
    }
}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    ble_cmd_connection_get_status(0);
}

void ble_rsp_gap_discover(const struct ble_msg_gap_discover_rsp_t *msg)
{
    //    gui->printText("Within method ble_rsp_gap_discover");
    //    printf("%u\n", msg->result);
    //    gui->printText((std::to_string(msg->result)));
}

void ble_rsp_gap_connect_direct(const ble_msg_gap_connect_direct_rsp_t *msg)
{
    if(msg->result == 0){
        //        gui->printText("Try to connect");
        message.name = "Try to connect";
        messageCaptured = true;
    } else {
        //        gui->printText("An error occured during connection establishment!");
    }
}

void ble_evt_attclient_group_found(const struct ble_msg_attclient_group_found_evt_t * msg)
{
    message.name = "ble_evt_attclient_group_found";
    messageCaptured = true;
}
