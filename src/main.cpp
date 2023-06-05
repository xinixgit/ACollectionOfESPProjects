#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "AudioPlayer.h"
#include "MqttHandler.h"
#include "SensorHandler.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define ONE_MIN 60000

// Temperature Sensor
#define DHTPIN 13 // GPIO 13
#define DHTTYPE DHT11

Config config;
AudioPlayer *audioPlayer;
MqttHandler *mqttHandler;
SensorHandler *sensorHandler;
DHT dht(DHTPIN, DHTTYPE);

void connectToWifi();
void connectToMqtt();
void startAudioPlayer(void *);
void startSensor(void *);

void setup()
{
  Serial.begin(9600);

  connectToWifi();
  delay(500);

  mqttHandler = new MqttHandler(&config.mqtt_config);
  audioPlayer = new AudioPlayer();

  connectToMqtt();
  delay(500);

  xTaskCreatePinnedToCore(
      startAudioPlayer,
      "Play audio",
      4096,
      NULL,
      10,
      NULL,
      0);
  delay(500);

  xTaskCreatePinnedToCore(
      startSensor,
      "Start publishing all sensor metrics",
      4096,
      NULL,
      10,
      NULL,
      1);
  delay(500);
}

void loop()
{
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

void connectToMqtt()
{
  mqttHandler->onAudioPlayerVolumeChangeRequest([](const char *payload)
                                                { audioPlayer->onVolumeChangeRequested(payload); });
  mqttHandler->onAudioPlayerStateChangeRequest([](const char *payload)
                                               { audioPlayer->onStateChangeRequested(payload); });
  mqttHandler->connect();
}

void startAudioPlayer(void *parameter)
{
  audioPlayer->setPublishStateFn([](String payload)
                                 { mqttHandler->publishAudioPlayerState(payload); });
  audioPlayer->play();
  for (;;)
  {
    audioPlayer->loop();
  }
}

void startSensor(void *parameter)
{
  dht.begin();
  std::list<Sensor *> sensors = {
      new TemperatureSensor{
          &dht,
          [](String payload)
          { mqttHandler->publishTemperature(payload); }}};
  sensorHandler = new SensorHandler(sensors);

  const TickType_t xDelay = ONE_MIN / portTICK_PERIOD_MS;
  for (;;)
  {
    sensorHandler->publishAll();
    vTaskDelay(xDelay);
  }
}
