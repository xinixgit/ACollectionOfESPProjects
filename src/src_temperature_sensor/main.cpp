#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "MqttHandler.h"
#include "CommunicationManager.h"
#include "SensorHandler.h"

#define TEN_MIN 600000

Config config;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
MqttHandler mqttHandler;
TempSensorCommunicationManager tempCommMgr;
SensorHandler sensorHandler;

// functions declaration
void connectToWifi();
void initSensors();
void onWifiConnect(const WiFiEventStationModeGotIP &);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &);

void setup()
{
  Serial.begin(9600);

  mqttHandler.init(config.mqtt_config);

  tempCommMgr.init(&mqttHandler, MQTT_TOPIC_SENSOR_TEMPERATURE);

  initSensors();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
}

void loop()
{
  connectToWifi();
  delay(500);

  mqttHandler.connect();
  delay(500);

  sensorHandler.publishAll();
  delay(500);

  mqttHandler.disconnect();
  delay(500);

  WiFi.mode(WIFI_OFF);
  delay(500);

  WiFi.forceSleepBegin();
  delay(500);

  delay(30000);

  WiFi.forceSleepWake();
  delay(500);
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi network ");
  WiFi.begin(config.wifi_ssid, config.wifi_password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
}

void initSensors()
{
  TemperatureSensorConfig tempConfig = TemperatureSensorConfig(AHT21);
  sensorHandler.init(tempConfig, &tempCommMgr);
}