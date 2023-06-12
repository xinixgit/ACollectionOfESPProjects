#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "MqttHandler.h"
#include "CommunicationManager.h"
#include "SensorHandler.h"

#define TEN_MIN_IN_US 10 * 60 * 1e6

Config config;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
MqttHandler *mqttHandler = nullptr;
TemperatureSensorCommunicationManager *communicationManager = nullptr;
SensorHandler *sensorHandler = nullptr;

// functions declaration
void connectToWifi();
void onWifiConnect(const WiFiEventStationModeGotIP &);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &);

void setup()
{
  Serial.begin(9600);

  pinMode(SPRINKLER_WATER, OUTPUT);

  if (mqttHandler == nullptr)
  {
    mqttHandler = new MqttHandler(&config.mqtt_config);
  }
  if (communicationManager == nullptr)
  {
    communicationManager = new TemperatureSensorCommunicationManager(mqttHandler);
  }
  if (sensorHandler == nullptr)
  {
    sensorHandler = new SensorHandler([](String payload)
                                      { communicationManager->publishTemperature(payload); });
  }

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  connectToWifi();
  delay(500);

  mqttHandler->connect();
  delay(500);

  sensorHandler->publishAll();
  delay(5000); // provide active CPU for new code upload if any

  mqttHandler->disconnect();

  WiFi.mode(WIFI_OFF);

  ESP.deepSleep(TEN_MIN_IN_US);
}

void loop()
{
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
