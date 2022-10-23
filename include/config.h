#ifndef ESP8266_AIR_CONFIG_H
#define ESP8266_AIR_CONFIG_H

#define wifi_ssid "ssid"
#define wifi_password "password"
#define mqtt_server "192.168.1.1"
#define mqtt_port 1883

#define batteryTxPin D1
#define batteryRxPin D2

#define mqttTopic "wifi2mqtt/%s"
#define mqttClientId "ninebattery-mqtt"
#define mqttEnabled true

#endif //ESP8266_AIR_CONFIG_H
