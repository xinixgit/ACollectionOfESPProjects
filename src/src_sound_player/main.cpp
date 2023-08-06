#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "Config.h"
#include "AudioPlayer.h"
#include "MqttHandler.h"
#include "CommunicationManager.h"
#include "SensorHandler.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define TEN_MIN 600000

Config config;
AudioPlayer audioPlayer;
MqttHandler mqttHandler;
SoundPlayerCommunicationManager communicationManager;
SensorHandler sensorHandler;
AsyncWebServer server(80);

void connectToWifi();
void initCommunicationManager();
void initSensors();
void startWebServer();
void startAudioPlayer(void *);

void setup()
{
  Serial.begin(9600);

  connectToWifi();
  delay(500);

  mqttHandler.init(config.mqtt_config);

  audioPlayer.init();

  initCommunicationManager();

  initSensors();

  mqttHandler.connect();
  delay(500);

  startWebServer();

  xTaskCreate(
      startAudioPlayer,
      "Play audio",
      4096,
      NULL,
      10,
      NULL);
  delay(500);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED || !mqttHandler.isConnected())
  {
    ESP.restart();
  }

  sensorHandler.publishAll();
  delay(TEN_MIN);
}

void connectToWifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // WiFi Connected, print IP to serial monitor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void startAudioPlayer(void *parameter)
{
  audioPlayer.setPublishStateFn([](String payload)
                                { communicationManager.publishState(payload); });
  audioPlayer.start();
  for (;;)
  {
    audioPlayer.loop();
  }
}

void startWebServer()
{
  AsyncElegantOTA.begin(&server);
  server.begin();
}

void initCommunicationManager()
{
  communicationManager.init(
      &mqttHandler,
      spConfig.MqttTopicSensorTemperature,
      [](string payload)
      { audioPlayer.onVolumeChangeRequested(payload); },
      [](string payload)
      { audioPlayer.onStateChangeRequested(payload); },
      [](string payload)
      { audioPlayer.onGenreChangeRequested(payload); });
}

void initSensors()
{
  TemperatureSensorConfig tempSensorConfig = TemperatureSensorConfig(DHT11Sensor);
  tempSensorConfig.DHTPin = 21;
  sensorHandler.init(tempSensorConfig, &communicationManager);
}
