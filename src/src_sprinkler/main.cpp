#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"

#define TEN_MIN_IN_US 600000000

Config config;
SprinkerConfig spConfig;
MqttHandler *mqttHandler;
SprinklerCommunicationManager *communicationManager;
TemperatureSensor *sysTempSensor;
SensorHandler *sensorHandler;
bool isWatering = false;
int waterDurationInMs = 0;

// functions declaration
void connectToWifi();
void initSensorHandler();
void initCommunicationManager();
void turnOffWater();

void setup()
{
  Serial.begin(9600);
  pinMode(spConfig.WaterPumpPin, OUTPUT);
  pinMode(spConfig.FanPin, OUTPUT);
  pinMode(spConfig.AdhocWaterPin, INPUT);

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

  if (isWatering && waterDurationInMs > 0)
  {
    while (waterDurationInMs > 0)
    {
      delay(500);
      waterDurationInMs -= 500;
    }

    turnOffWater();
  }

  // if temperature gets too high, turn fan on for 30s
  if (sysTempSensor->readTemperatureF() > 90.0)
  {
    communicationManager->publishState("fan", "on");
    digitalWrite(spConfig.FanPin, HIGH);
    delay(30000);
    digitalWrite(spConfig.FanPin, LOW);
    communicationManager->publishState("fan", "off");
    delay(500);
  }

  communicationManager->publishState("esp_board", "off");
  delay(1000);

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

void turnOnWater()
{
  if (!isWatering)
  {
    isWatering = true;
    digitalWrite(spConfig.WaterPumpPin, HIGH);
    Serial.println("Sprinkler started watering.");
    communicationManager->publishState("water_pump", "on");
  }
}

void turnOffWater()
{
  isWatering = false;
  digitalWrite(spConfig.WaterPumpPin, LOW);
  Serial.println("Sprinkler stopped watering.");
  communicationManager->publishState("water_pump", "off");
}

void onWaterRequest(const char *payload)
{
  waterDurationInMs = std::stoi(payload) * 1000;
  turnOnWater();
}

void initCommunicationManager()
{
  communicationManager = new SprinklerCommunicationManager(mqttHandler);
  communicationManager->onWaterRequest(onWaterRequest);
  Serial.println("Communication manager initiated.");
}

void initSensorHandler()
{
  TemperatureSensorConfig sensorConfig = TemperatureSensorConfig([](String payload)
                                                                 { communicationManager->publishTemperature(payload, spConfig.MqttTopicSensorTemperature); });

  sensorConfig.type = BME280Sensor;
  sensorConfig.SCLPin = spConfig.BMESCLPin;
  sensorConfig.SDAPin = spConfig.BMESDAPin;
  TemperatureSensor *tempSensor = initTemperatureSensor(sensorConfig);
  sysTempSensor = tempSensor;

  sensorHandler = new SensorHandler(std::list<Sensor *>{tempSensor});
  Serial.println("Sensor handler initiated.");
}
