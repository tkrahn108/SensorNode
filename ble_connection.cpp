#include "ble_connection.h"

volatile HANDLE serial_handle;
//maximum number of devices we can connect to; limited
#define MAX_CONNECTED_DEVICES 8
//maximum number of device we want to scan
#define MAX_SCANNED_DEVICES 50

bd_addr connected_devices[MAX_CONNECTED_DEVICES];
bd_addr scanned_devices[MAX_SCANNED_DEVICES];
uint8 primary_service_uuid[] = {0x00, 0x28};
uint8 connection_handle;

BLE_Connection::BLE_Connection(QObject *parent) :
    QObject(parent)
{

}

int BLE_Connection::init()
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

    COMMTIMEOUTS cto;
    // Set the new timeouts
    cto.ReadIntervalTimeout = MAXDWORD;
    cto.ReadTotalTimeoutConstant = 0;
    cto.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(serial_handle,&cto);

    if (serial_handle == INVALID_HANDLE_VALUE)
    {
        char buff[100];
        snprintf(buff, sizeof(buff), "Error opening serialport %s. %d\n","COM3",(int)GetLastError());
        return -1;
    } else {
        bglib_output = output;

        messageCaptured = false;
        connected_devices_count = 0;
        scanned_devices_count = 0;
        newAdvertisingDevice = false;

        //stop previous operation
        ble_cmd_gap_end_procedure();
        //get connection status,current command will be handled in response
        ble_cmd_connection_get_status(0);

        return 0;
    }
}

void BLE_Connection::requestMethod(BLE_Connection::Method method)
{
    if(method != DoNothing){
        qDebug()<<"Request worker Method"<<method<<"in Thread "<<thread()->currentThreadId();
    }
    QMutexLocker locker(&mutex);
    _method = method;
}

void BLE_Connection::setAddr(bd_addr addr)
{
    mutex.lock();
    _addr = addr;
    mutex.unlock();
}


void BLE_Connection::requestScan()
{
    QMutexLocker locker(&mutex);
    _abort = false;
    qDebug()<<"requestScan in Thread "<<thread()->currentThreadId();

    emit scanRequested();
}

void BLE_Connection::abort()
{
    qDebug()<<"Request worker aborting in Thread "<<thread()->currentThreadId();
    QMutexLocker locker(&mutex);
    _abort = true;
    emit aborted();
}

void BLE_Connection::doScan()
{
    while(!_abort){
        if( messageCaptured ){
            emit valueChanged(message);
            messageCaptured = false;
            message.name = "";
            message.rssi = NULL;
        }

        mutex.lock();
        Method method = _method;
        mutex.unlock();

        switch(method) {
        case DoNothing:
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
        case CharacterisiticDiscovery:
            characterisiticDiscovery();
            break;
        case DescriptorsDiscovery:
            descriptorsDiscovery();
            break;
        case ReadValueByHandle:
            readValueByHandle();
            break;
        case NotificationOn:
            notificationOn();
            break;
        case NotificationOff:
            notificationOff();
            break;
        case DisconnectAll:
            disconnectAll();
            break;
        default:
            qDebug() << "Default in switch case";
            break;
        }

        requestMethod(DoNothing);

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
    qDebug() << "Before:";
    for(int j = 0; j < MAX_CONNECTED_DEVICES-1; j++){
        print_bdaddr(connected_devices[j]);
    }

    mutex.lock();
    bd_addr temp = _addr;
    mutex.unlock();
    ble_cmd_gap_connect_direct(temp.addr, 1, 50, 3200, 400, 0);
}

void BLE_Connection::disconnect()
{
    mutex.lock();
    bd_addr temp = _addr;
    mutex.unlock();
//    ble_cmd_connection_disconnect(1);
    int tempConnectionHandle = getConnectionHandle(temp);
    if(tempConnectionHandle != -1){
        ble_cmd_connection_disconnect(tempConnectionHandle);
        qDebug() << "Try to disconnect!";
        //ble_cmd_gap_end_procedure();
        qDebug() << "disconnect requested";
     } else {
        qDebug() << "No Connection Handle for this address";
    }

}

void BLE_Connection::disconnectAll()
{
    //TODO do not loop over all possible connection handles
    for(int i = 0; i <= MAX_CONNECTED_DEVICES; i++){
        ble_cmd_connection_disconnect(i);
    }
}

void BLE_Connection::primaryServiceDiscovery()
{
    uint8 uuid_len = sizeof(primary_service_uuid);
    ble_cmd_attclient_read_by_group_type(connection_handle, 0x0001, 0xFFFF, uuid_len, primary_service_uuid);
}

void BLE_Connection::characterisiticDiscovery()
{
    uint8 uuid[] = {0x03, 0x28};
    ble_cmd_attclient_read_by_type(connection_handle, 0x000c, 0xFFFF, sizeof(uuid), uuid);
}

void BLE_Connection::descriptorsDiscovery()
{
    ble_cmd_attclient_find_information(connection_handle, 0x0001, 0xFFFF);
}

void BLE_Connection::readValueByHandle()
{
    ble_cmd_attclient_read_by_handle(connection_handle, 0x000e);
}

void BLE_Connection::notificationOn()
{
    uint8 data[] = {0x01, 0x00};
    mutex.lock();
    bd_addr temp = _addr;
    mutex.unlock();
    ble_cmd_attclient_attribute_write(getConnectionHandle(temp), 0x000f, sizeof(data), data);
}

void BLE_Connection::notificationOff()
{
    uint8 data[] = {0x00, 0x00};
    mutex.lock();
    bd_addr temp = _addr;
    mutex.unlock();
    ble_cmd_attclient_attribute_write(getConnectionHandle(temp), 0x000f, sizeof(data), data);
}

int BLE_Connection::getConnectionHandle(bd_addr adr)
{
    for (int i = 0; i < MAX_CONNECTED_DEVICES-1; ++i) {
        if(cmp_bdaddr(adr,connected_devices[i]) == 0){
            return i;
        }
    }
    return -1;
}

/**
 * Check whether address is found in scanned_devices.
 *
 * @brief checkScannedDevices
 * @param bdaddr Address to compare
 * @return true if address is found in scanned_devices
 */
bool checkScannedDevices(bd_addr bdaddr)
{
    for(int i = 0; i < scanned_devices_count; i++){
        if(cmp_bdaddr(bdaddr,scanned_devices[i]) == 0) return true;
    }

    return false;
}

/**
 * Print address.
 * @brief print_bdaddr
 * @param bdaddr Address to print
 */
void print_bdaddr(bd_addr bdaddr)
{
    char str[20];
    snprintf(str, sizeof(str)-1, "%02x:%02x:%02x:%02x:%02x:%02x",
            bdaddr.addr[5],
            bdaddr.addr[4],
            bdaddr.addr[3],
            bdaddr.addr[2],
            bdaddr.addr[1],
            bdaddr.addr[0]);

    qDebug() << str;
}

/**
 * Compare Bluetooth addresses
 *
 * @param first First address
 * @param second Second address
 * @return Zero if addresses are equal
 */
int cmp_bdaddr(bd_addr first, bd_addr second)
{
    int i;
    for (i = 0; i < sizeof(bd_addr); i++) {
        if (first.addr[i] != second.addr[i]) return 1;
    }
    return 0;
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

//    if(!checkScannedDevices(msg->sender)){
//        memcpy(scanned_devices[scanned_devices_count].addr, msg->sender.addr, sizeof(bd_addr));
//        scanned_devices_count++;
//    }

    free(name);
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(msg->flags&connection_completed)
    {
        message.name = "New connection established";
        messageCaptured = true;

        // Check if this device already found
//        int i;
//        for (i = 0; i < connected_devices_count; i++) {
//            if (!cmp_bdaddr(msg->address, connected_devices[i])) return;
//        }
        connected_devices_count++;
        memcpy(connected_devices[msg->connection].addr, msg->address.addr, sizeof(bd_addr));

        qDebug() << "Connected:";
        for(int j = 0; j < MAX_CONNECTED_DEVICES-1; j++){
            print_bdaddr(connected_devices[j]);
        }

        connection_handle = msg->connection;
        qDebug() << "Added Device: " << connected_devices_count;
        qDebug() << "Connection Handle: " << connection_handle;
    }else
    {
        ble_cmd_gap_discover(1);
    }
}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    ble_cmd_connection_get_status(0);

    connected_devices_count--;
    uint8 addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(connected_devices[msg->connection].addr, addr, sizeof(bd_addr));

    qDebug() << "Disconnected:";
    for(int j = 0; j < MAX_CONNECTED_DEVICES-1; j++){
        print_bdaddr(connected_devices[j]);
    }

    qDebug() << "Removed Device: " << connected_devices_count;
}

void ble_rsp_connection_disconnect(const struct ble_msg_connection_disconnect_rsp_t * msg)
{
    if(msg->result == 0){
        qDebug() << "Connection handle " << msg->connection << ": Disconnection procedure successfully started. ";
    } else {
        qDebug() << "Connection handle " << msg->connection << "Disconnection procedure failed. " << msg->result;
    }
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

void ble_evt_attclient_find_information_found(const ble_msg_attclient_find_information_found_evt_t *msg)
{
    message.name = std::string(msg->uuid.data, msg->uuid.data+msg->uuid.len);
    qDebug() << "Uuid->data: " << msg->uuid.data << " Uuid.len: " << msg->uuid.len;
    messageCaptured = true;
}

void ble_evt_attclient_attribute_value(const ble_msg_attclient_attribute_value_evt_t *msg)
{
    //conversion of lux value
    uint16_t lux = (msg->value.data[1] << 8) | msg->value.data[0];
    uint8_t k;
    uint8_t exp = lux >> 12;
    uint16_t mantisse = lux;
    for(k = 12; k<=16; k++){
        mantisse &= ~(1 << k);
    }
    double res = 0.01 * pow(2.0,(double)exp) * mantisse;
    message.name = std::to_string(res);

    messageCaptured = true;
}
