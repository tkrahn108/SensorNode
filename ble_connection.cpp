#include "ble_connection.h"

volatile HANDLE serial_handle;
SensorNode_GUI *gui;

ble_connection::ble_connection(SensorNode_GUI *g)
{
    gui = g;

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
        printf("Error opening serialport %s. %d\n","COM3",(int)GetLastError());
    }

    bglib_output = output;

    //stop previous operation
    ble_cmd_gap_end_procedure();
    //get connection status,current command will be handled in response
    ble_cmd_connection_get_status(0);
    for(int i = 0; i < 10; i++){
        read_message();
        printf("Readed message");
    }

    //Message loop
//    while(1)
//    {
//        if(read_message())
//        {
//            printf("Error reading message\n");
//            break;
//        }
//    }
}

ble_connection::~ble_connection()
{

}

void ble_connection::output(uint8 len1,uint8* data1,uint16 len2,uint8* data2)
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

int ble_connection::read_message()
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

void ble_connection::print_help()
{
    printf("Demo application to scan devices\n");
    printf("\tscan_example\tCOM-port\n");
}


//====================FUNCTIONS FROM stubs.c======================================


void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg)
{
    int i;
    for(i=0;i<6;i++)
        printf("%02x%s",msg->sender.addr[5-i],i<5?":":"");
    printf("\t%d\n",msg->rssi);
    gui->printText("Läuft");
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(msg->flags&connection_connected)
    {
        printf("#connected -> disconnect\n");
        ble_cmd_connection_disconnect(msg->connection);
    }else
    {
        printf("#Not connected -> Scan\n");
        ble_cmd_gap_discover(1);
    }
}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    ble_cmd_connection_get_status(0);
}
