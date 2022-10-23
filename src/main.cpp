#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "config.h"
#include "main.h"
#include "battery.h"

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    server.begin();
    delay(500);
    WiFi.begin(wifi_ssid, wifi_password);
    delay(10000);

    if(mqttEnabled) {
        mqttClient.setServer(mqtt_server, mqtt_port);
        mqttClient.setBufferSize(500);
    }
}

void loop() {
    mqttEnabled && mqttClient.loop();

    int s = WiFi.status();
    if (s == WL_DISCONNECTED || s == WL_IDLE_STATUS || s == WL_CONNECT_FAILED || s == WL_CONNECTION_LOST) {
        Serial.println("Disconnected, reconnecting!");
        WiFi.disconnect();
        ESP.restart();

        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_ssid, wifi_password);
        delay(10000);
        return;
    }
    if (s != WL_CONNECTED) {
        Serial.println("not connected: " + String(s));
        delay(1000);
        return;
    }

    if (mqttEnabled && !mqttClient.connected()) {
        Serial.println("Disconnected mqtt, reconnecting!");
        mqttClient.disconnect();
        mqttClient.connect(mqttClientId);
        return;
    }
    if(mqttEnabled) mqttClient.loop();

    uptime = String(long(millis() / 1000));

    DynamicJsonDocument info = batteryClient.readBattery();
    if(!info.containsKey("error")) {
        char topic[] = mqttTopic;
        sprintf(topic, mqttTopic, info["serial"].as<const char*>());
        info.remove("serial");
        serializeJson(info, payloadBytes);
        Serial.println(String(payloadBytes));
        Serial.printf("publish %s\n", topic);
        if(mqttEnabled) mqttClient.publish(topic, payloadBytes);
        if(mqttEnabled) mqttClient.loop();
    } else {
        Serial.printf("ERROR: %s\n", info["error"].as<const char*>());
    }

//    if(debug) {
//        Serial.println();
//    } else {
//        Serial.print(".");
//    }
    serverResponse.toCharArray(payloadBytes, 500);

    wifiClient = server.available();
    if (!wifiClient) {
        debug && Serial.println("no client...");
        delay(100);

        return;
    }
    req = wifiClient.readStringUntil('\r');
    Serial.println("request: " + req);
    if (req.indexOf("/metrics") >= 0) {
        respond(serverResponse);

        return;
    }

    if (req.indexOf("/debug") >= 0) {
        debug = !debug;
        batteryClient.debug = debug;
        respond(String(debug));

        return;
    }

    respond("My MAC: " + WiFi.macAddress() + ", my IP: " + WiFi.localIP().toString());
}

void respond(const String &text) {
    wifiClient.println("HTTP/1.1 200 OK");
    wifiClient.println("Content-Type: text/html");
    wifiClient.println("Connection: close");
    wifiClient.println("");
    wifiClient.println(text);
    wifiClient.flush();
}
