#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "battery.h"

SoftwareSerial *batterySerial;

battery::battery(int batteryTxPin, int batteryRxPin, bool debugEnabled) {
    batterySerial = new SoftwareSerial(batteryTxPin, batteryRxPin);
    batterySerial->begin(115200);
    batterySerial->setTimeout(2000);
    debug = debugEnabled;
}


bool battery::sendCommand(const byte *cmd, int cmdLen) {
    if (debug) printBytes("battery request", cmd, cmdLen);

    int maxAtt = 1;
    for(int att = 1; att <=maxAtt; att++) {
        batterySerial->write(cmd, cmdLen);
        if(receiving()) {

            return true;
        }
        delay(500);
        if(debug) Serial.printf("failed (%d/%d)...\n", att, maxAtt);
    }

    return false;
}

bool battery::receiving() {
    unsigned long started = millis();
    while(!batterySerial->available()) {
        if(millis() - started >= 500) {
            if(debug) Serial.println("receive timeout");

            return false;
        }
    }
    memset(response, 0, responseSize);
    responsePos = 0;
    state = STATE_NONE;

    while (batterySerial->available()) {
        int currentByte = batterySerial->read();
        response[responsePos] = currentByte;

        bool save = true;
        switch (state) {
            case STATE_NONE:
                if (currentByte == 0x5A) {
                    state = STATE_HEADER2;
                    break;
                }

                if(debug) Serial.println("invalid state at header 1");
                if(debug) printBytes("bytes", response, 10);

                //hack to fix not receiving last byte
                return receiving();
            case STATE_HEADER2:
                if (currentByte != 0xA5) {
                    if(debug) Serial.println("invalid state at header 2");
                    if (debug) printBytes("bytes", response, 10);
                    save = false;
                    break;
                }

                state = STATE_LEN;
                break;
            case STATE_LEN:
                expectedLen = currentByte;
                state = STATE_SRCADDR;
                break;
            case STATE_SRCADDR:
                state = STATE_DSTADDR;
                break;
            case STATE_DSTADDR:
                state = STATE_CMD;
                break;
            case STATE_CMD:
                state = STATE_ARG;
                break;
            case STATE_ARG:
                //arg = currentByte
                state = STATE_PAYLOAD;
                payloadPos = 0;
                break;
            case STATE_PAYLOAD:
                state = STATE_PAYLOAD;
                payloadPos++;
                if (payloadPos >= expectedLen) {
                    state = STATE_CRC;
                    break;
                }
                break;
            case STATE_CRC:
                if (responsePos + 1 >= expectedLen + 9) {
                    if (debug) printBytes("stop by len " + String(expectedLen + 9), response, 40);

                    state = STATE_READY;
                    break;
                }
                break;
            default:
                Serial.printf("unknown state %d\n", state);
                panic();
        }
        if (!save) {
            memset(response, 0, responseSize);
            responsePos = 0;
            state = STATE_NONE;
            continue;
        }
        if (state == STATE_READY) {

            return verifyCrc();
        }

        responsePos++;
        if (responsePos >= responseSize) {
            if (debug) printBytes("invalid response size", response, 11);
            state = STATE_NONE;
            memset(response, 0, responseSize);
            responsePos = 0;

            return false;
        }
    }

    //hack to fix not receiving last byte
    if (state == STATE_CRC) {
        response[expectedLen + 9 - 1] = 0xFF;

        return verifyCrc();
    }

    if (debug) printBytes("not processing", response, 11);

    return false;
}

DynamicJsonDocument battery::readBattery() {
    //status resp (11):    5aa50222200430010086ff          0x5a 0xa5 0x02 0x22 0x20 0x04 0x30 0x01 0x00 0x86 0xff
    DynamicJsonDocument doc(1024);

    if(!sendCommand(get_status, sizeof(get_status))) {
        doc["error"] = "status";
        return doc;
    } else{
        doc["status"] = convertBytesToInt(response[8], response[7]);
    }

    if(!sendCommand(get_serial, sizeof(get_serial))) {
        doc["error"] = "serial";
        return doc;
    } else {
        byte serial[15] = {};
        memcpy(&serial, &response[7], 14 * sizeof(response[0])); serial[14] = 0x00;
        doc["serial"] = String((char*)serial);
    }

    if(!sendCommand(get_remaining_capacity_perc, sizeof(get_remaining_capacity_perc))) {
        doc["error"] = "get_remaining_capacity_perc";
        return doc;
    } else {
        doc["remaining_capacity_perc"] = convertBytesToInt(response[8], response[7]);
    }

    if(!sendCommand(get_remaining_capacity, sizeof(get_remaining_capacity))) {
        doc["error"] = "get_remaining_capacity";
        return doc;
    } else {
        doc["remaining_capacity"] = convertBytesToInt(response[8], response[7]);
    }

    if(!sendCommand(get_factory_capacity, sizeof(get_factory_capacity))) {
        doc["error"] = "get_factory_capacity";
        return doc;
    } else {
        doc["factory_capacity"] = convertBytesToInt(response[8], response[7]);
    }

    if(!sendCommand(get_actual_capacity, sizeof(get_actual_capacity))) {
        doc["error"] = "get_actual_capacity";
        return doc;
    } else {
        doc["actual_capacity"] = convertBytesToInt(response[8], response[7]);
    }

    if(!sendCommand(get_current, sizeof(get_current))) {
        doc["error"] = "get_current";
        return doc;
    } else {
        doc["current"] = double(convertBytesToInt(response[8], response[7])) * 10 / 1000;
    }

    if(!sendCommand(get_voltage, sizeof(get_voltage))) {
        doc["error"] = "get_voltage";
        return doc;
    } else {
        doc["voltage"] = double(convertBytesToInt(response[8], response[7])) * 10 / 1000;
    }
    doc["power"] = double (doc["current"].as<double>() / doc["voltage"].as<double>());

    if(!sendCommand(get_temperature, sizeof(get_temperature))) {
        doc["error"] = "get_temperature";
        return doc;
    } else {
        doc["temperature"]["zone_0"] = response[7] - 20;
        doc["temperature"]["zone_1"] = response[8] - 20;
    }

    if(!sendCommand(get_cells_voltage, sizeof(get_cells_voltage))) {
        doc["error"] = "get_cells_voltage";
        return doc;
    } else {
        doc["cell_voltage"]["cell_0"] = convertBytesToInt(response[8], response[7]);
        doc["cell_voltage"]["cell_1"] = convertBytesToInt(response[10], response[9]);
        doc["cell_voltage"]["cell_2"] = convertBytesToInt(response[12], response[11]);
        doc["cell_voltage"]["cell_3"] = convertBytesToInt(response[14], response[13]);
        doc["cell_voltage"]["cell_4"] = convertBytesToInt(response[16], response[15]);
        doc["cell_voltage"]["cell_5"] = convertBytesToInt(response[18], response[17]);
        doc["cell_voltage"]["cell_6"] = convertBytesToInt(response[20], response[19]);
        doc["cell_voltage"]["cell_7"] = convertBytesToInt(response[22], response[21]);
        doc["cell_voltage"]["cell_8"] = convertBytesToInt(response[24], response[23]);
        doc["cell_voltage"]["cell_9"] = convertBytesToInt(response[26], response[25]);
    }
    doc["uptime"] = long(millis() / 1000);

    return doc;
}

int battery::convertBytesToInt(byte byte1, byte byte2) {
    int result = (byte1 << 8) | byte2;

    return result;
}

void battery::printBytes(const String &tag, const byte bytes[], int len) {
    Serial.print(tag + ": ");
    for (int i = 0; i < len; i++) {
        Serial.print("0x");
        if (bytes[i] <= 0xE) {
            Serial.print("0");
        }
        Serial.print(bytes[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
}

bool battery::verifyCrc() {
    unsigned int cs = 0;
    for (int i = 2; i <= expectedLen + 6; i++) {
        cs = cs + response[i];
    }
    if (((response[expectedLen + 9 - 1] << 8) | response[expectedLen + 9 - 2]) == 0xFFFF - cs) {
        if (debug) printBytes("would parse", response, expectedLen + 9);

        return true;
    }
    if (debug) printBytes("invalid crc", response, expectedLen + 9);

    return false;
}
