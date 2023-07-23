#include <WiFi.h>
#include <WifiClient.h>
#include "Config.h"
#include "MqttHandler.h"
#include "SensorHandler.h"
#include "CommunicationManager.h"
#include "ESPCamHandler.h"
#include "WebStreamer.h"

#define TEN_MIN 600000

Config config;
CamStreamConfig camConfig;
MqttHandler *mqttHandler;
TempSensorCommunicationManager *tempCm;
SensorHandler *sensorHandler;
ESPCamHandler *camHandler;
audp::WebStreamer *webStreamer;

bool setupComplete = false;

// functions declaration
void blinkLED(void *parameter);
void startWebStream(void *parameter);
void connectToWifi();
void initSensorHandler();

void setup()
{
  pinMode(camConfig.LEDPin, OUTPUT);
  pinMode(camConfig.SCLPin, INPUT);
  pinMode(camConfig.SDAPin, INPUT);

  xTaskCreate(
      blinkLED,
      "Blink LED",
      1024,
      (void *)&setupComplete,
      10,
      NULL);

  connectToWifi();
  delay(500);

  camHandler = new ESPCamHandler();
  webStreamer = new audp::WebStreamer(camHandler);

  xTaskCreatePinnedToCore(
      startWebStream,
      "Start web stream",
      8192,
      NULL,
      4,
      NULL,
      0);
  delay(500);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  mqttHandler->connect();
  delay(500);

  tempCm = new TempSensorCommunicationManager(mqttHandler, camConfig.MqttTopicSensorTemperature);
  delay(500);

  initSensorHandler();
  delay(500);

  setupComplete = true;
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
  }

  sensorHandler->publishAll();
  delay(TEN_MIN);
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
}

void onPictureRequest(const char *payload)
{
  camHandler->takePicAndSave();
}

void blinkLED(void *parameter)
{
  bool *setupComplete = (bool *)parameter;
  while (*setupComplete == false)
  {
    digitalWrite(camConfig.LEDPin, LOW); // LOW turns on the LED
    delay(250);
    digitalWrite(camConfig.LEDPin, HIGH);
    delay(250);
  }

  // turn on the LED so we know system is working
  digitalWrite(camConfig.LEDPin, LOW);
  vTaskDelete(NULL);
}

void startWebStream(void *parameter)
{
  webStreamer->begin();
  for (;;)
  {
    webStreamer->loop();
    delay(1000);
  }
}

void initSensorHandler()
{
  TemperatureSensorConfig tempSensorConfig = TemperatureSensorConfig(AHT21);
  tempSensorConfig.SCLPin = camConfig.SCLPin;
  tempSensorConfig.SDAPin = camConfig.SDAPin;
  sensorHandler = new SensorHandler(tempSensorConfig, tempCm);
}