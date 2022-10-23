#ifndef NINEBATTERY_MQTT_MAIN_H
#define NINEBATTERY_MQTT_MAIN_H

#include <WiFiServer.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include "battery.h"

bool debug = false;

WiFiServer server(80);
WiFiClient wifiClient;
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
battery batteryClient(batteryTxPin, batteryRxPin, debug);

String serverResponse;
char   payloadBytes[500];
String uptime;
String req;

void respond(const String& text);


#endif //NINEBATTERY_MQTT_MAIN_H
