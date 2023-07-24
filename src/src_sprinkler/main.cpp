#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"

#define TEN_MIN_IN_US 600000000

Config config;
SprinklerConfig sprinklerConfig;
MqttHandler mqttHandler;
SprinklerCommunicationManager communicationManager;
SensorHandler sensorHandler;
bool doWater = false;
int waterOnDurationInMs = 0;
bool doFan = false;
int fanOnDurationInMs = 0;

// functions declaration
void connectToWifi();
void initMqttHandler();
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

  initMqttHandler();
  delay(500);

  initCommunicationManager();

  initSensorHandler();

  mqttHandler.connect();
  delay(500);

  communicationManager.publishState("esp_board", "on");
  delay(500);

  sensorHandler.publishAll();
  delay(500);

  // wait 10s for program upload and watering requests
  unsigned long tStart = millis();
  while (millis() - tStart < 10000)
  {
    delay(1000);
  }

  // turn water on based on for requested duration
  if (doWater && waterOnDurationInMs > 0)
  {
    turnOnWater();
    tStart = millis();
    while (millis() - tStart < waterOnDurationInMs)
    {
      delay(1000);
    }
    waterOnDurationInMs = 0;
    turnOffWater();
    doWater = false;
  }

  delay(1000);

  // turn fan on based on temperature (detected in HA)
  if (doFan && fanOnDurationInMs > 0)
  {
    turnOnFan();
    tStart = millis();
    while (millis() - tStart < fanOnDurationInMs)
    {
      delay(1000);
    }
    fanOnDurationInMs = 0;
    turnOffFan();
    doFan = false;
  }

  communicationManager.publishState("esp_board", "off");
  delay(500);

  ESP.deepSleep(TEN_MIN_IN_US);
}

void loop()
{
  delay(1000);
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

  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());
}

void turnOnFan()
{
  digitalWrite(sprinklerConfig.FanPin, HIGH);
  delay(250);
  communicationManager.publishState("fan", "on");
  delay(250);
}

void turnOffFan()
{
  digitalWrite(sprinklerConfig.FanPin, LOW);
  delay(250);
  communicationManager.publishState("fan", "off");
  delay(250);
}

void onFanRequest(std::string payload)
{
  doFan = true;
  fanOnDurationInMs = std::stoi(payload) * 1000;
}

void turnOnWater()
{
  digitalWrite(sprinklerConfig.WaterPumpPin, HIGH);
  delay(250);
  communicationManager.publishState("water_pump", "on");
  delay(250);
}

void turnOffWater()
{
  digitalWrite(sprinklerConfig.WaterPumpPin, LOW);
  delay(250);
  communicationManager.publishState("water_pump", "off");
  delay(250);
}

void onWaterRequest(std::string payload)
{
  doWater = true;
  waterOnDurationInMs = std::stoi(payload) * 1000;
}

void initMqttHandler()
{
  config.mqtt_config.cleanSession = false;
  mqttHandler.init(config.mqtt_config);
}

void initCommunicationManager()
{
  communicationManager.init(
      &mqttHandler,
      sprinklerConfig.MqttTopicSensorTemperature,
      onWaterRequest,
      onFanRequest);
}

void initSensorHandler()
{
  TemperatureSensorConfig sensorConfig = TemperatureSensorConfig(BME280Sensor);
  sensorConfig.SCLPin = sprinklerConfig.BMESCLPin;
  sensorConfig.SDAPin = sprinklerConfig.BMESDAPin;
  sensorHandler.init(sensorConfig, &communicationManager);
}
