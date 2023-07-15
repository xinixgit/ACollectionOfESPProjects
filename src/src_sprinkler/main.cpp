#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"

#define TEN_MIN_IN_US 600000000

Config config;
SprinklerConfig sprinklerConfig;
MqttHandler *mqttHandler;
SprinklerCommunicationManager *communicationManager;
SensorHandler *sensorHandler;
bool doWater = false;
int waterOnDurationInMs = 0;
bool doFan = false;
int fanOnDurationInMs = 0;

// functions declaration
void connectToWifi();
void initSensorHandler();
void initCommunicationManager();
void turnOnWater();
void turnOffWater();
void turnOnFan();
void turnOffFan();

void setup()
{
  Serial.begin(9600);
  pinMode(sprinklerConfig.WaterPumpPin, OUTPUT);
  pinMode(sprinklerConfig.FanPin, OUTPUT);

  connectToWifi();
  delay(500);

  MqttConfig *mqttConfig = &config.mqtt_config;
  mqttConfig->cleanSession = false;
  mqttHandler = new MqttHandler(mqttConfig);

  initSensorHandler();
  initCommunicationManager();

  mqttHandler->connect();
  delay(500);

  sensorHandler->publishAll();

  communicationManager->publishState("esp_board", "on");

  // leave 10s for program upload and watering requests
  delay(10000);

  // turn water on based on for requested duration
  if (doWater && waterOnDurationInMs > 0)
  {
    turnOnWater();
    while (waterOnDurationInMs > 0)
    {
      delay(500);
      waterOnDurationInMs -= 500;
    }
    turnOffWater();
    doWater = false;
  }

  // turn fan on based on temperature (detected in HA)
  if (doFan && fanOnDurationInMs > 0)
  {
    turnOnFan();
    while (fanOnDurationInMs > 0)
    {
      delay(500);
      fanOnDurationInMs -= 500;
    }
    turnOffFan();
    doFan = false;
  }

  communicationManager->publishState("esp_board", "off");
  delay(500);

  ESP.deepSleep(TEN_MIN_IN_US);
}

void loop()
{
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi network ");
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void turnOnFan()
{
  digitalWrite(sprinklerConfig.FanPin, HIGH);
  communicationManager->publishState("fan", "on");
  delay(500);
}

void turnOffFan()
{
  digitalWrite(sprinklerConfig.FanPin, LOW);
  communicationManager->publishState("fan", "off");
  delay(500);
}

void onFanRequest(std::string payload)
{
  doFan = true;
  fanOnDurationInMs = std::stoi(payload) * 1000;
}

void turnOnWater()
{
  digitalWrite(sprinklerConfig.WaterPumpPin, HIGH);
  communicationManager->publishState("water_pump", "on");
  delay(500);
}

void turnOffWater()
{
  digitalWrite(sprinklerConfig.WaterPumpPin, LOW);
  communicationManager->publishState("water_pump", "off");
  delay(500);
}

void onWaterRequest(std::string payload)
{
  doWater = true;
  waterOnDurationInMs = std::stoi(payload) * 1000;
}

void initCommunicationManager()
{
  communicationManager = new SprinklerCommunicationManager(mqttHandler);
  communicationManager->onWaterRequest(onWaterRequest);
  communicationManager->onFanRequest(onFanRequest);
}

void initSensorHandler()
{
  TemperatureSensorConfig sensorConfig = TemperatureSensorConfig([](String payload)
                                                                 { communicationManager->publishTemperature(payload, sprinklerConfig.MqttTopicSensorTemperature); });

  sensorConfig.type = BME280Sensor;
  sensorConfig.SCLPin = sprinklerConfig.BMESCLPin;
  sensorConfig.SDAPin = sprinklerConfig.BMESDAPin;
  sensorHandler = new SensorHandler(sensorConfig);
}
