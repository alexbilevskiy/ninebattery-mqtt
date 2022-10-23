#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#ifndef NINEBATTERY_MQTT_BATTERY_H
#define NINEBATTERY_MQTT_BATTERY_H

class battery {
public:
    bool debug = false;

    battery(int batteryTxPin, int batteryRxPin, bool debugEnabled);

    DynamicJsonDocument readBattery();

    bool sendCommand(const byte cmd[], int cmdLen);

    bool receiving();

private:

    const byte get_status[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x30, 0x02, 0x89, 0xff,};
    const byte get_serial[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x10, 0x0e, 0x9d, 0xff,};
    const byte get_remaining_capacity_perc[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x32, 0x02, 0x87, 0xff,};
    const byte get_remaining_capacity[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x31, 0x02, 0x88, 0xff,};
    const byte get_actual_capacity[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x19, 0x02, 0xa0, 0xff,};
    const byte get_factory_capacity[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x18, 0x02, 0xa1, 0xff,};
    const byte get_current[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x33, 0x02, 0x86, 0xff,};
    const byte get_voltage[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x34, 0x02, 0x85, 0xff,};
    const byte get_cells_voltage[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x40, 0x14, 0x67, 0xff,};
    const byte get_temperature[10] = {0x5a, 0xa5, 0x01, 0x20, 0x22, 0x01, 0x35, 0x02, 0x84, 0xff,};

    enum states {
        STATE_NONE = 0,
        STATE_HEADER1 = 1,
        STATE_HEADER2 = 2,
        STATE_LEN = 3,
        STATE_SRCADDR = 4,
        STATE_DSTADDR = 5,
        STATE_CMD = 6,
        STATE_ARG = 7,
        STATE_PAYLOAD = 8,
        STATE_CRC = 9,
        STATE_READY = 10,
    };
    static const int responseSize = 64;
    byte response[responseSize]{};
    int expectedLen{};
    int payloadPos{};
    int responsePos{};
    states state = STATE_NONE;

    int16_t convertBytesToInt(byte byte1, byte byte2);

    void printBytes(const String &tag, const byte bytes[], int len);

    bool verifyCrc();
};

#endif //NINEBATTERY_MQTT_BATTERY_H
