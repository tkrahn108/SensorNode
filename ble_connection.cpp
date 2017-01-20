#include "ble_connection.h"

volatile HANDLE serial_handle;
uint8 connection_handle;

BLE_Connection::BLE_Connection(QObject *parent) :
    QObject(parent)
{
    char str[80];
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

    //stop previous operation
    ble_cmd_gap_end_procedure();
    //get connection status,current command will be handled in response
    ble_cmd_connection_get_status(0);
}

//BLE_Connection::~BLE_Connection()
//{
//    ble_cmd_connection_disconnect(0);
//    ble_cmd_connection_disconnect(1);
//    CloseHandle(serial_handle);
//}

void BLE_Connection::requestWork()
{
    mutex.lock();
    _working = true;
    _abort = false;
    //    qDebug()<<"Request worker start in Thread "<<thread()->currentThreadId();
    mutex.unlock();

    emit workRequested();
}

void BLE_Connection::abort()
{
    mutex.lock();
    if (_working) {
        _abort = true;
        //        qDebug()<<"Request worker aborting in Thread "<<thread()->currentThreadId();
    }
    mutex.unlock();
}

void BLE_Connection::doWork()
{
    while(true){
        if( messageCaptured ){
            emit valueChanged(QString::fromStdString(message));
            messageCaptured = false;
        }
        read_message();
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
    ble_cmd_connection_disconnect(1);
}

//=============FUNCTIONS FOR HANDELING EVENTS AND RESPONSES====================


void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg)
{
    int i;
    std::string buffAsStdStr;
    char buff[100];
    char *name = NULL;
    for(i=0;i<6;i++){
        snprintf(buff, sizeof(buff), "%02x%s",msg->sender.addr[5-i],i<5?":":"");
        buffAsStdStr += buff;
    }

    // Parse data to get the name
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

    if (name) snprintf(buff, sizeof(buff), "\t%s", name);
    else snprintf(buff, sizeof(buff), "\t%s", "Unknown");
    buffAsStdStr += buff;

    snprintf(buff, sizeof(buff), "\t%d",msg->rssi);
    buffAsStdStr += buff;
    message = buffAsStdStr;
    messageCaptured = true;

    free(name);
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(msg->flags&connection_connected)
    {
        //        gui->printText("Connection established");
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
    } else {
        //        gui->printText("An error occured during connection establishment!");
    }
}
