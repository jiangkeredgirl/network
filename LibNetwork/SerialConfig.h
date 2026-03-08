#pragma once

//struct SerialConfig
//{
//    bool enable_crc = true;
//    bool enable_heartbeat = false;
//    int heartbeat_interval_ms = 3000;
//    bool enable_frame = true; // 帧头帧尾
//    char frame_header = 0xAA;
//    char frame_tail = 0x55;
//    bool enable_auto_reconnect_ = true;
//    int reconnect_interval_ms = 2000;
//    int max_reconnect_count = 10;    
//};

#pragma once

struct SerialConfig
{
    int baudrate = 115200;

    int data_bits = 8;
    int stop_bits = 1;
    int parity = 0;

    bool auto_reconnect = true;

    int reconnect_interval_ms = 3000;

    int request_timeout_ms = 2000;

    int request_retry = 2;
};
